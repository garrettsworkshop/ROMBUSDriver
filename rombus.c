#include <Memory.h>
#include <Devices.h>
#include <Files.h>
#include <Disks.h>
#include <Errors.h>
#include <Events.h>
#include <OSUtils.h>

#include "rombus.h"
#include "spi.h"
#include "priv_syscall.h"

// Decode keyboard settings
static void RBDecodeKeySettings(RBStorage_t *c) {
	// Sample R, S, X keys repeatedly
	char r = 0, s = 0, x = 0;
	long tmax = TickCount() + 60;
	for (long i = 0; i < 1000000; i++) {
		r |= IsRPressed();
		s |= IsSPressed();
		x |= IsXPressed();
		if (r || s || x) { break; }
		if (TickCount() > tmax) { break; }
	}
	// Decode settings
	if (x) { // Unmount everything
		c->unmountROMEN = 1;
		c->unmountSDEN = 1;
		c->mountROMEN = 0;
		c->mountSDEN = 0;
	} else if (s) { // Boot from SD, don't mount ROM
		c->unmountROMEN = 1;
		c->unmountSDEN = 0;
		c->mountROMEN = 1;
		c->mountSDEN = 0;
	} else if (r) { // Boot from ROM, mount SD
		c->unmountROMEN = 0;
		c->unmountSDEN = 1;
		c->mountROMEN = 0;
		c->mountSDEN = 1;
	}
}

// Decode PRAM settings
static OSErr RBDecodePRAMSettings(RBStorage_t *c) {
	// Read PRAM
	char legacy_startup, legacy_ram;
	PSReadXPRAM(1, 4, &legacy_startup);
	PSReadXPRAM(1, 5, &legacy_ram);
	
	// Decoded settings
	const char opt_disable   = legacy_startup & (1<<7);
	const char opt_boot_sd   = legacy_startup & (1<<2);
	const char opt_boot_rom  = legacy_startup & (1<<0);
	const char opt_mount_sd  = legacy_startup & (1<<3);
	const char opt_mount_rom = legacy_startup & (1<<4);

	// Old settings for ROM SIMM (not used here)
	//const char opt_old_mount_rom = !(legacy_startup & (1<<1));
	//const char opt_old_ram       = legacy_ram & (1<<0);

	// Set based on decoded settings
	if (opt_disable) { return -1; }
	else {
		c->unmountROMEN = !opt_boot_rom;
		c->unmountSDEN = !opt_boot_sd;
		c->mountROMEN = opt_mount_rom && !opt_boot_rom;
		c->mountSDEN = opt_mount_sd && !opt_boot_sd;
	}
	return noErr;
}

// Switch to 32-bit mode and copy
#pragma parameter C24(__A0, __A1, __D0)
void __attribute__ ((noinline)) C24(Ptr sourcePtr, Ptr destPtr, unsigned long byteCount) {
	signed char mode = true32b;
	SwapMMUMode(&mode);
	BlockMove(sourcePtr, destPtr, byteCount);
	SwapMMUMode(&mode);
}

#pragma parameter __D0 RBClose(__A0, __A1)
OSErr RBClose(IOParamPtr p, DCtlPtr d) {
	// If dCtlStorage not null, dispose of it
	if (!d->dCtlStorage) { return noErr; }
	HUnlock(d->dCtlStorage);
	DisposeHandle(d->dCtlStorage);
	d->dCtlStorage = NULL;
	return noErr;
}

#pragma parameter __D0 RBOpen(__A0, __A1)
OSErr RBOpen(IOParamPtr p, DCtlPtr d) {
	int drvNum;
	RBStorage_t *c;

	// Do nothing if already opened
	if (d->dCtlStorage) { return noErr; }

	// Allocate storage struct
	d->dCtlStorage = NewHandleSysClear(sizeof(RBStorage_t));
	if (!d->dCtlStorage) { return openErr; }

	// Lock our storage struct and get master pointer
	HLock(d->dCtlStorage);
	c = *(RBStorage_t**)d->dCtlStorage;

	// Do nothing if inhibited
	if (RBDecodePRAMSettings(c) != noErr) {
		RBClose(p, d);
		return openErr;
	}

	// Iff mount enabled, enable accRun to post disk inserted event later
	if (c->mountROMEN || c->mountSDEN) { d->dCtlFlags |= dNeedTimeMask; }
	else { d->dCtlFlags &= ~dNeedTimeMask; }

	// Set accRun delay
	d->dCtlDelay = 150; // (150 ticks is 2.5 sec.)

	// Find first available drive number
	drvNum = PSFindDrvNum();

	//TODO: set c->sdSize
	
	// Set drive status
	c->sdStatus.track = 0;
	c->sdStatus.writeProt = 0; // nonzero is write protected
	c->sdStatus.diskInPlace = 8; // 8 is nonejectable disk
	c->sdStatus.installed = 1; // drive installed
	c->sdStatus.sides = 0;
	c->sdStatus.qType = 1;
	c->sdStatus.dQDrive = drvNum;
	c->sdStatus.dQFSID = 0;
	c->sdStatus.dQRefNum = d->dCtlRefNum;
	c->sdStatus.driveSize = c->sdSize / 512;
	c->sdStatus.driveS1 = (c->sdSize / 512) >> 16;

	// Decompress icon
	#ifdef RB_COMPRESS_ICON_ENABLE
	char *src = &SDIconCompressed[0];
	char *dst = &c->sd[0];
	UnpackBits(&src, &dst, RB_ICON_SIZE);
	#endif

	// Add drive to drive queue and return
	PSAddDrive(c->sdStatus.dQRefNum, drvNum, (DrvQElPtr)&c->sdStatus.qLink);
	return noErr;
}

// Init is called at beginning of first prime (read/write) call
static void RBBootInit(IOParamPtr p, DCtlPtr d, RBStorage_t *c) {
	// Mark init done
	c->initialized = 1;

	// Decode key settings
	RBDecodeKeySettings(c);

	// Unmount if not booting from ROM disk
	if (c->unmountSDEN) { c->sdStatus.diskInPlace = 0; }

	// Iff mount disabled, disable accRun
	if (!c->mountSDEN || !c->mountROMEN) { d->dCtlFlags &= ~dNeedTimeMask; }
}

#pragma parameter __D0 RBPrime(__A0, __A1)
OSErr RBPrime(IOParamPtr p, DCtlPtr d) {
	RBStorage_t *c;

	// Return disk offline error if dCtlStorage null
	if (!d->dCtlStorage) { return notOpenErr; }
	// Dereference dCtlStorage to get pointer to our context
	c = *(RBStorage_t**)d->dCtlStorage;

	// Initialize if this is the first prime call
	if (!c->initialized) { RBBootInit(p, d, c); }

	// Return disk offline error if virtual disk not inserted
	if (!c->sdStatus.diskInPlace) { return offLinErr; }

	//TODO: Read/write

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
		case kFormat:
			if (!c->sdStatus.diskInPlace || c->sdStatus.writeProt) {
				return controlErr;
			}
			//TODO: Format
			return noErr;
		case kVerify:
			if (!c->sdStatus.diskInPlace) { return controlErr; }
			return noErr;
		case accRun:
			c->initialized = 1; // Mark init done
			c->sdStatus.diskInPlace = 8; // 8 is nonejectable disk
			PostEvent(diskEvt, c->sdStatus.dQDrive); // Post disk inserted event
			d->dCtlFlags &= ~dNeedTimeMask; // Disable accRun
			return noErr;
		case kDriveIcon: case kMediaIcon: // Get icon
			#ifdef RB_COMPRESS_ICON_ENABLE
			*(Ptr*)p->csParam = (Ptr)c->sd;
			#else
			*(Ptr*)p->csParam = (Ptr)RDiskIcon;
			#endif
			return noErr;
		case kDriveInfo:
			//  high word (bytes 2 & 3) clear
			//  byte 1 = primary + fixed media + internal
			//  byte 0 = drive type (0x10 is RAM disk) / (0x11 is ROM disk)
			if (c->sdStatus.writeProt) { *(long*)p->csParam = 0x00000400; }
			else { *(long*)p->csParam = 0x00000400; }
			return noErr;
		case 24: // Return SCSI partition size
			*(long*)p->csParam = c->sdSize / 512;
			return noErr;
		case killCode: return noErr;
		case kEject:
			// "Reinsert" disk if ejected illegally
			if (c->sdStatus.diskInPlace) { 
				PostEvent(diskEvt, c->sdStatus.dQDrive);
			}
			return controlErr; // Eject not allowed so return error
		default: return controlErr;
	}
}

#pragma parameter __D0 RBStat(__A0, __A1)
OSErr RBStat(CntrlParamPtr p, DCtlPtr d) {
	// Fail if dCtlStorage null
	if (!d->dCtlStorage) { return notOpenErr; }
	// Handle status request based on csCode
	switch (p->csCode) {
		case kDriveStatus:
			BlockMove(*d->dCtlStorage, &p->csParam, sizeof(DrvSts2));
			return noErr;
		default: return statusErr;
	}
}
