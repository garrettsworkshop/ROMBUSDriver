#include <Memory.h>
#include <Devices.h>
#include <Files.h>
#include <Disks.h>
#include <Errors.h>
#include <Events.h>
#include <OSUtils.h>
#include <Quickdraw.h>

#include "rombus.h"
#include "rdisk.h"
#include "sdisk.h"

// Decode keyboard/PRAM settings
static void RBDecodeSettings(Ptr unmountROM, Ptr unmountSD, Ptr mountSD) {
	// Sample R, S, X keys repeatedly
	char r = 0, s = 0, x = 0;
	long tmax = TickCount() + 60;
	for (long i = 0; i < 1000000; i++) {
		r |= RBIsRPressed();
		s |= RBIsSPressed();
		x |= RBIsXPressed();
		if (r || s || x) { break; }
		if (TickCount() > tmax) { break; }
	}

	// Read PRAM
	char legacy_startup;
	RBReadXPRAM(1, 4, &legacy_startup);

	// Decode settings: unmount (don't boot), mount (after boot), RAM disk
	if (x) { // X disables SD
		*unmountROM = 1; // Unmount ROM disk volume
		*unmountSD = 1; // Unmount SD volume
		*mountSD = 0; // Don't mount SD later
	} else if (r) { // R boots from ROM disk and mount SD later
		*unmountROM = 0; // Don't unmount so we boot from ROM
		*unmountSD = 1; // Unmount SD volume so we don't boot from it
		*mountSD = 1; // Mount SD later
	} else if (s) { // S boots from SD
		*unmountROM = 1; // Unmount ROM disk volume so we don't boot from it
		*unmountSD = 0; // Don't unmount so we boot from SD
		*mountSD = 0; // No need to mount SD later since already mounted
	} else if (legacy_startup & 0x04) { // Boot from SD
		*unmountROM = 1; // Unmount ROM disk volume so we don't boot from it
		*unmountSD = 0; // Don't unmount so we boot from SD
		*mountSD = 0; // No need to mount later since already mounted
	} else {
		*unmountROM = !(legacy_startup & 0x01); // Unmount ROM disk if saved in PRAM
		*unmountSD = 1; // Unmount SD volume so we don't boot from it
		*mountSD =  !(legacy_startup & 0x08); // Mount SD later if setting
	}
}

// Switch to 32-bit mode and copy
#pragma parameter C24(__A0, __A1, __D0)
void __attribute__ ((noinline)) C24(Ptr sourcePtr, Ptr destPtr, unsigned long byteCount) {
	signed char mode = true32b;
	SwapMMUMode(&mode);
	BlockMove(sourcePtr, destPtr, byteCount);
	SwapMMUMode(&mode);
}

// Figure out the first available drive number >= 5
static int RBFindDrvNum() {
	DrvQElPtr dq;
	int drvNum = 5;
	for (dq = (DrvQElPtr)(GetDrvQHdr())->qHead; dq; dq = (DrvQElPtr)dq->qLink) {
		if (dq->dQDrive >= drvNum) { drvNum = dq->dQDrive + 1; }
	}
	return drvNum;
}

#pragma parameter __D0 RBOpen(__A0, __A1)
OSErr RBOpen(IOParamPtr p, DCtlPtr d) {
	int drvNum;
	RBStorage_t *c;
	char legacy_startup;
	char *src, *dst;
	OSErr ret;

	// Do nothing if already opened
	if (d->dCtlStorage) { return noErr; }

	// Do nothing if inhibited
	RBReadXPRAM(1, 4, &legacy_startup);
	if (legacy_startup & 0x80) { return noErr; } 

	// Allocate storage struct
	d->dCtlStorage = NewHandleSysClear(sizeof(RBStorage_t));
	if (!d->dCtlStorage) { return openErr; }

	// Lock our storage struct and get master pointer
	HLock(d->dCtlStorage);
	c = *(RBStorage_t**)d->dCtlStorage;

	// Populate both drives' DrvSts and add each to drive queue
	if (ret = RDOpen(p, d ,c) != noErr) { return ret; }	
	if (ret = SDOpen(p, d ,c) != noErr) { return ret; }	
	return noErr;
}

// Init is called at beginning of first prime (read/write) call
static void RBInit(IOParamPtr p, DCtlPtr d, RBStorage_t *c) {
	char unmountROM, unmountSD;
	// Mark init done
	c->initialized = 1;
	// Decode settings
	RBDecodeSettings(&unmountROM, &c->mountROM, &unmountSD, &c->mountSD);

	// Unmount if requested
	if (unmountROM) { c->rdStatus.diskInPlace = 0; }
	if (unmountSD) { c->sdStatus.diskInPlace = 0; }

	// If mount enabled, enable accRun to post disk inserted event later
	if (c->mountROM || c->mountSD) { 
		d->dCtlDelay = 150; // Set accRun delay (150 ticks is 2.5 sec.)
		d->dCtlFlags |= dNeedTimeMask; // Enable accRun
	}
}

#pragma parameter __D0 RBPrime(__A0, __A1)
OSErr RBPrime(IOParamPtr p, DCtlPtr d) {
	RBStorage_t *c;
	char cmd;
	Ptr disk;

	// Return disk offline error if dCtlStorage null
	if (!d->dCtlStorage) { return notOpenErr; }
	// Dereference dCtlStorage to get pointer to our context
	c = *(RBStorage_t**)d->dCtlStorage;

	// Initialize if this is the first prime call
	if (!c->initialized) { RBInit(p, d, c); }

	switch (p->ioVRefNum) {
		case c->rdStatus.dQDrive: return RDPrime(p, d, c);
		case c->sdStatus.dQDrive: return SDPrime(p, d, c);
		default: return statusErr;
	}
}

#pragma parameter __D0 RBCtl(__A0, __A1)
OSErr RBCtl(CntrlParamPtr p, DCtlPtr d) {
	RBStorage_t *c;
	// Fail if dCtlStorage null
	if (!d->dCtlStorage) { return notOpenErr; }
	// Dereference dCtlStorage to get pointer to our context
	c = *(RBStorage_t**)d->dCtlStorage;
	// Handle control request csCodes common to both volumes
	switch (p->csCode) {
		case killCode: return noErr;
		case accRun:
			if (c->mountSD) { // Request to mount SD disk
				c->sdStatus.diskInPlace = 8; // 8 is nonejectable disk
				PostEvent(diskEvt, c->sdStatus.dQDrive); // Post disk inserted event
				return noErr;
			} else if (c->mountROM) { // Request to mount ROM disk
				c->rdStatus.diskInPlace = 8; // 8 is nonejectable disk
				PostEvent(diskEvt, c->rdStatus.dQDrive); // Post disk inserted event
				return noErr;
			} else { // Nothing to mount
				d->dCtlFlags &= ~dNeedTimeMask; // Disable accRun
				return noErr;
			}
		case 2351: // Post-boot
			c->initialized = 1; // Skip initialization
			d->dCtlDelay = 30; // Set accRun delay (30 ticks is 0.5 sec.)
			d->dCtlFlags |= dNeedTimeMask; // Enable accRun
			return noErr;
	}
	// Dispatch based on volume reference number
	switch (p->ioVRefNum) {
		case c->rdStatus.dQDrive: return RDCtl(p, d, c);
		case c->sdStatus.dQDrive: return SDCtl(p, d, c);
		default: return controlErr;
	}
}

#pragma parameter __D0 RBStat(__A0, __A1)
OSErr RBStat(CntrlParamPtr p, DCtlPtr d) {
	RBStorage_t *c;
	// Fail if dCtlStorage null
	if (!d->dCtlStorage) { return notOpenErr; }
	// Dereference dCtlStorage to get pointer to our context
	c = *(RBStorage_t**)d->dCtlStorage;
	// Dispatch based on volume reference number
	switch (p->ioVRefNum) {
		case c->rdStatus.dQDrive: return RDStat(p, d, c);
		case c->sdStatus.dQDrive: return SDStat(p, d, c);
		default: return statusErr;
	}
}

#pragma parameter __D0 RBClose(__A0, __A1)
OSErr RBClose(IOParamPtr p, DCtlPtr d) {
	// If dCtlStorage not null, dispose of it
	if (!d->dCtlStorage) { return noErr; }
	RBStorage_t *c = *(RBStorage_t**)d->dCtlStorage;
	RDClose(p, d, c);
	SDClose(p, d, c);
	HUnlock(d->dCtlStorage);
	DisposeHandle(d->dCtlStorage);
	d->dCtlStorage = NULL;
	return noErr;
}
