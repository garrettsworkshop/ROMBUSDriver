#include <Memory.h>
#include <Devices.h>
#include <Files.h>
#include <Disks.h>
#include <Errors.h>
#include <Events.h>
#include <OSUtils.h>

#include "rombus.h"

// Decode keyboard/PRAM settings
static void RBDecodeSettings(Ptr unmountROMEN, Ptr mountROMEN, Ptr unmountSDEN, Ptr mountSDEN) {
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
	if (x) { // X disables ROM disk and SD
		*unmountROMEN = 1; // Unmount ROM disk volume
		*mountROMEN = 0; // Don't mount ROM later
		*unmountSDEN = 1; // Unmount SD volume
		*mountSDEN = 0; // Don't mount SD later
	} else if (r) { // R boots from ROM disk and mount SD later
		*unmountROMEN = 0; // Don't unmount so we boot from ROM
		*mountROMEN = 0; // No need to mount ROM later since already mounted
		*unmountSDEN = 1; // Unmount SD volume so we don't boot from it
		*mountSDEN = 1; // Mount SD later
	} else if (s) { // S boots from SD
		*unmountROMEN = 1; // Unmount ROM disk volume so we don't boot from it
		*mountROMEN = 1; // Mount ROM disk later
		*unmountSDEN = 0; // Don't unmount so we boot from SD
		*mountSDEN = 0; // No need to mount SD later since already mounted
	} else if (legacy_startup & 0x04) { // Boot from SD
		*unmountROMEN = 1; // Unmount ROM disk volume so we don't boot from it
		*mountROMEN = !(legacy_startup & 0x02); // Mount ROM disk later if setting
		*unmountSDEN = 0; // Don't unmount so we boot from SD
		*mountSDEN = 0; // No need to mount later since already mounted
	} else if (legacy_startup & 0x01) { // Boot from ROM
		*unmountROMEN = 0; // Don't unmount so we boot from ROM
		*mountROMEN = 0; // No need to mount later since already mounted
		*unmountSDEN = 1; // Unmount SD volume so we don't boot from it
		*mountSDEN =  !(legacy_startup & 0x08); // Mount SD later if setting
	} else {
		*unmountROMEN = 1; // Unmount ROM disk volume so we don't boot from it
		*mountROMEN = !(legacy_startup & 0x02); // Mount ROM disk later if setting
		*unmountSDEN = 1; // Unmount SD volume so we don't boot from it
		*mountSDEN =  !(legacy_startup & 0x08); // Mount SD later if setting
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

	// Find first available drive number
	drvNum = RBFindDrvNum();
	
	// Set drive status
	//c->rdStatus.track = 0;
	c->rdStatus.writeProt = -1; // nonzero is write protected
	c->rdStatus.diskInPlace = 8; // 8 is nonejectable disk
	c->rdStatus.installed = 1; // drive installed
	//c->rdStatus.sides = 0;
	//c->rdStatus.qType = 1;
	c->rdStatus.dQDrive = drvNum;
	//c->rdStatus.dQFSID = 0;
	c->rdStatus.dQRefNum = d->dCtlRefNum;
	c->rdStatus.driveSize = RDiskSize / 512;
	//c->rdStatus.driveS1 = (RDiskSize / 512) >> 16;

	// Decompress icons
	char *src = &RDiskIconCompressed[0];
	char *dst = &c->rdIcon[0];
	UnpackBits(&src, &dst, RDISK_ICON_SIZE);
	src = &SDiskIconCompressed[0];
	dst = &c->sdIcon[0];
	UnpackBits(&src, &dst, SDISK_ICON_SIZE);

	// Add drive to drive queue and return
	RBAddDrive(c->rdStatus.dQRefNum, drvNum, (DrvQElPtr)&c->rdStatus.qLink);
	return noErr;
}

// Init is called at beginning of first prime (read/write) call
static void RBInit(IOParamPtr p, DCtlPtr d, RBStorage_t *c) {
	char unmountEN, mountEN, dbgEN, cdrEN;
	// Mark init done
	c->initialized = 1;
	// Decode settings
	RBDecodeSettings(&unmountEN, &mountEN, &dbgEN, &cdrEN);

	// Unmount if not booting from ROM disk
	if (unmountEN) { c->rdStatus.diskInPlace = 0; }

	// If mount enabled, enable accRun to post disk inserted event later
	if (mountEN) { 
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

	// Return disk offline error if virtual disk not inserted
	if (!c->rdStatus.diskInPlace) { return offLinErr; }

	// Get pointer to ROM disk buffer
	disk = RDiskBuf + d->dCtlPosition;
	//  Bounds checking
	if (d->dCtlPosition >= RDiskSize || p->ioReqCount >= RDiskSize || 
		d->dCtlPosition + p->ioReqCount >= RDiskSize) { return paramErr; }

	// Service read or write request
	cmd = p->ioTrap & 0x00FF;
	if (cmd == aRdCmd) { // Read
		// Read from disk into buffer.
		if (*MMU32bit) { BlockMove(disk, p->ioBuffer, p->ioReqCount); }
		else { copy24(disk, StripAddress(p->ioBuffer), p->ioReqCount); }
	} else if (cmd == aWrCmd) { return wPrErr;
	} else { return noErr; } //FIXME: Fail if cmd isn't read or write?

	// Update count and position/offset, then return
	d->dCtlPosition += p->ioReqCount;
	p->ioActCount = p->ioReqCount;
	return noErr;
}

#pragma parameter __D0 RBCtl(__A0, __A1)
OSErr RBCtl(CntrlParamPtr p, DCtlPtr d) {
	RBStorage_t *c;
	// Fail if dCtlStorage null
	if (!d->dCtlStorage) { return notOpenErr; }
	// Dereference dCtlStorage to get pointer to our context
	c = *(RBStorage_t**)d->dCtlStorage;
	// Handle control request based on csCode
	switch (p->csCode) {
		case killCode: return noErr;
		case kFormat: return controlErr;
		case kVerify:
			if (!c->rdStatus.diskInPlace) { return controlErr; }
			return noErr;
		case kEject:
			// "Reinsert" disk if ejected illegally
			if (c->rdStatus.diskInPlace) { 
				PostEvent(diskEvt, c->rdStatus.dQDrive);
			}
			return controlErr; // Eject not allowed so return error
		case accRun:
			d->dCtlFlags &= ~dNeedTimeMask; // Disable accRun
			c->rdStatus.diskInPlace = 8; // 8 is nonejectable disk
			PostEvent(diskEvt, c->rdStatus.dQDrive); // Post disk inserted event
			return noErr;
		case kDriveIcon: case kMediaIcon: // Get icon
			*(Ptr*)p->csParam = (Ptr)c->rdIcon;
			return noErr;
		case kDriveInfo:
			//  high word (bytes 2 & 3) clear
			//  byte 1 = primary + fixed media + internal
			//  byte 0 = drive type (0x10 is RAM disk) / (0x11 is ROM disk)
			*(long*)p->csParam = 0x00000411;
			return noErr;
		case 24: // Return SCSI partition size
			*(long*)p->csParam = RDiskSize / 512;
			return noErr;
		case 2351: // Post-boot
			c->initialized = 1; // Skip initialization
			d->dCtlDelay = 30; // Set accRun delay (30 ticks is 0.5 sec.)
			d->dCtlFlags |= dNeedTimeMask; // Enable accRun
			return noErr;
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
	// Handle status request based on csCode
	switch (p->csCode) {
		case kDriveStatus:
			BlockMove(&c->rdStatus, &p->csParam, sizeof(DrvSts2));
			return noErr;
		default: return statusErr;
	}
}

#pragma parameter __D0 RBClose(__A0, __A1)
OSErr RBClose(IOParamPtr p, DCtlPtr d) {
	// If dCtlStorage not null, dispose of it
	if (!d->dCtlStorage) { return noErr; }
	//RBStorage_t *c = *(RBStorage_t**)d->dCtlStorage;
	HUnlock(d->dCtlStorage);
	DisposeHandle(d->dCtlStorage);
	d->dCtlStorage = NULL;
	return noErr;
}
