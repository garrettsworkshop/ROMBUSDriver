#include <Memory.h>
#include <Devices.h>
#include <Files.h>
#include <Disks.h>
#include <Errors.h>
#include <Events.h>
#include <OSUtils.h>

#include "rdisk.h"

// Decode keyboard/PRAM settings
static void RDDecodeSettings(Ptr recoveryEN, Ptr unmountEN, Ptr mountEN) {
	// Read PRAM
	char legacy_startup;
	RBReadXPRAM(1, 4, &legacy_startup);

	// Decode settings: boot from recovery, unmount (don't boot), mount (after boot)
	if (RBIsRPressed()) { // R boots from ROM recovery
		*recoveryEN = 1; // Enable recovery partition
		*unmountEN = 0; // Unmount SD so we don't boot from it
		*mountEN = 1; // Mount SD later
	} else {
		*recoveryEN = 0; // Disable recovery partition
		if (legacy_startup & 0x10) { // Boot from SD disk
			*unmountEN = 0; // Don't unmount so we boot from this drive
			*mountEN = 0; // No need to mount later since we are boot disk
		} else if (legacy_startup & 0x20) { // Mount SD disk under other boot volume
			*unmountEN = 1; // Unmount to not boot from our disk
			*mountEN = 1; // Mount in accRun
		} else {
			*unmountEN = 1; // Unmount
			*mountEN = 0; // Don't mount again
		}
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

// Switch to 32-bit mode and get
#pragma parameter __D0 G24(__A2)
char __attribute__ ((noinline)) G24(Ptr pos) {
	long ret;
	signed char mode = true32b;
	SwapMMUMode(&mode);
	ret = *pos; // Peek
	SwapMMUMode(&mode);
	return ret;
}

// Switch to 32-bit mode and set
#pragma parameter S24(__A2, __D3)
void __attribute__ ((noinline)) S24(Ptr pos, char patch) {
	signed char mode = true32b;
	SwapMMUMode(&mode);
	*pos = patch; // Poke
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

static void RBOpenRDisk(RDiskStorage_t *c) {
	int drvNum;
	
	// Find first available drive number for ROM recovery
	drvNum = RBFindDrvNum();

	// Set ROM recovery drive status
	//c->rStatus.track = 0;
	c->rStatus.writeProt = -1; // nonzero is write protected
	c->rStatus.diskInPlace = 8; // 8 is nonejectable disk
	c->rStatus.installed = 1; // drive installed
	//c->rStatus.sides = 0;
	//c->rStatus.qType = 1;
	c->rStatus.dQDrive = drvNum;
	//c->rStatus.dQFSID = 0;
	c->rStatus.dQRefNum = d->dCtlRefNum;
	c->rStatus.driveSize = RDiskSize / 512;
	//c->rStatus.driveS1 = (RDiskSize / 512) >> 16;

	// Decompress icon
	#ifdef RDISK_COMPRESS_ICON_ENABLE
	char *src = &RDiskIconCompressed[0];
	char *dst = &c->rIcon[0];
	UnpackBits(&src, &dst, RDISK_ICON_SIZE);
	#endif

	// Add RDisk to drive queue and return
	RDiskAddDrive(c->rStatus.dQRefNum, drvNum, (DrvQElPtr)&c->rStatus.qLink);
}

static void RBOpenSDisk(RDiskStorage_t *c) {
	int drvNum;
	
	// Find first available drive number for SD disk
	drvNum = RBFindDrvNum();

	// Set SD disk drive status
	//c->rStatus.track = 0;
	c->rStatus.writeProt = 0; // 0 is writable
	c->rStatus.diskInPlace = 8; // 8 is nonejectable disk
	c->rStatus.installed = 1; // drive installed
	//c->rStatus.sides = 0;
	c->rStatus.qType = 1;
	c->rStatus.dQDrive = drvNum;
	//c->rStatus.dQFSID = 0;
	c->rStatus.dQRefNum = d->dCtlRefNum;
	c->rStatus.driveSize = SDiskSize / 512;
	c->rStatus.driveS1 = (SDiskSize / 512) >> 16;

	// Decompress icon
	#ifdef SDISK_COMPRESS_ICON_ENABLE
	char *src = &SDiskIconCompressed[0];
	char *dst = &c->sIcon[0];
	UnpackBits(&src, &dst, SDISK_ICON_SIZE);
	#endif

	// Add SDisk to drive queue and return
	RDiskAddDrive(c->rstatus.dQRefNum, drvNum, (DrvQElPtr)&c->sStatus.qLink);
}

#pragma parameter __D0 RBOpen(__A0, __A1)
OSErr RBOpen(IOParamPtr p, DCtlPtr d) {
	RDiskStorage_t *c;
	char legacy_startup;

	// Do nothing if already opened
	if (d->dCtlStorage) { return noErr; }

	// Do nothing if inhibited
	RBReadXPRAM(1, 4, &legacy_startup);
	if (legacy_startup & 0x40) { return noErr; } 

	// Allocate storage struct
	d->dCtlStorage = NewHandleSysClear(sizeof(RDiskStorage_t));
	if (!d->dCtlStorage) { return openErr; }

	// Lock our storage struct and get master pointer
	HLock(d->dCtlStorage);
	c = *(RDiskStorage_t**)d->dCtlStorage;

	// Create RDisk and SDisk entries in drive queue, then return
	RBOpenRDisk(c);
	RBOpenSDisk(c);
	return noErr;
}

// Init is called at beginning of first prime (read/write) call
static void RBInit(IOParamPtr p, DCtlPtr d, RDiskStorage_t *c) {
	char recoveryEN, unmountEN, mountEN;
	// Mark init done
	c->initialized = 1;
	// Decode settings
	RDDecodeSettings(&recoveryEN, &unmountEN, &mountEN);

	// Unmount if not booting from ROM disk
	if (!recoveryEN) { c->rStatus.diskInPlace = 0; }

	// Unmount if not booting from ROM disk
	if (unmountEN) { c->SStatus.diskInPlace = 0; }

	// If mount enabled, enable accRun to post disk inserted event later
	if (mountEN) { 
		d->dCtlDelay = 150; // Set accRun delay (150 ticks is 2.5 sec.)
		d->dCtlFlags |= dNeedTimeMask; // Enable accRun
	}
}

static OSErr RDPrime(IOParamPtr p, DCtlPtr d) {
	// Get pointer to correct position in ROM disk buffer
	Ptr disk = RDiskBuf + d->dCtlPosition;

	// Return disk offline error if virtual disk not inserted
	if (!c->rStatus.diskInPlace) { return offLinErr; }

	//  Bounds checking
	if (d->dCtlPosition >= RDiskSize || p->ioReqCount >= RDiskSize || 
		d->dCtlPosition + p->ioReqCount >= RDiskSize) { return paramErr; }

	// Service read or write request
	cmd = p->ioTrap & 0x00FF;
	if (cmd == aRdCmd) {
		if (*MMU32bit) { BlockMove(disk, p->ioBuffer, p->ioReqCount); }
		else { copy24(disk, StripAddress(p->ioBuffer), p->ioReqCount); }
	} else if (cmd == aWrCmd) { return wPrErr;
	} else { return noErr; } //FIXME: Fail if cmd isn't read or write?

	// Update count and position/offset, then return
	d->dCtlPosition += p->ioReqCount;
	p->ioActCount = p->ioReqCount;
	return noErr;
}

static OSErr SDPrime(IOParamPtr p, DCtlPtr d) {
	
}

#pragma parameter __D0 RBPrime(__A0, __A1)
OSErr RBPrime(IOParamPtr p, DCtlPtr d) {
	RDiskStorage_t *c;
	char cmd;

	// Return disk offline error if dCtlStorage null
	if (!d->dCtlStorage) { return notOpenErr; }
	// Dereference dCtlStorage to get pointer to our context
	c = *(RDiskStorage_t**)d->dCtlStorage;

	// Initialize if this is the first prime call
	if (!c->initialized) { RBInit(p, d, c); }

	if (p->ioVRefNum == c->sStatus.dQDrive) {
		return SDPrime(p, d, c);
	} else if (p->ioVRefNum == c->rStatus.dQDrive) {
		return RDPrime(p, d, c);
	} else { return nsvErr; }
}

static OSErr RDCtl(CntrlParamPtr p, DCtlPtr d, RDiskStorage_t *c) {
	// Handle control request based on csCode
	switch (p->csCode) {
		case kFormat: return controlErr;
		case kVerify:
			if (!c->rStatus.diskInPlace) { return controlErr; }
			return noErr;
		case kEject:
			// "Reinsert" disk if ejected illegally
			if (c->rStatus.diskInPlace) { 
				PostEvent(diskEvt, c->status.dQDrive);
			}
			return controlErr; // Eject not allowed so return error
		case kDriveIcon: case kMediaIcon: // Get icon
			#ifdef RDISK_COMPRESS_ICON_ENABLE
			*(Ptr*)p->csParam = (Ptr)c->rIcon;
			#else
			*(Ptr*)p->csParam = (Ptr)RDiskIcon;
			#endif
			return noErr;
		case kDriveInfo:
			//  high word (bytes 2 & 3) clear
			//  byte 1 = primary + fixed media + internal
			//  byte 0 = drive type (0x11 is ROM disk)
			*(long*)p->csParam = 0x00000411;
			return noErr;
		case 24: // Return SCSI partition size
			*(long*)p->csParam = RDiskSize / 512;
			return noErr;
		default: return controlErr;
	}
}

static OSErr SDCtl(CntrlParamPtr p, DCtlPtr d, RDiskStorage_t *c) {
	// Handle control request based on csCode
	switch (p->csCode) {
		case kFormat:
			// FIXME: implement SD format
			return controlErr;
		case kVerify:
			// FIXME: implement SD verify
			return noErr;
		case kEject:
			// "Reinsert" disk if ejected illegally
			if (c->sStatus.diskInPlace) { 
				PostEvent(diskEvt, c->sStatus.dQDrive);
			}
			return controlErr; // Eject not allowed so return error
		case kDriveIcon: case kMediaIcon: // Get icon
			#ifdef SDISK_COMPRESS_ICON_ENABLE
			*(Ptr*)p->csParam = (Ptr)c->sIcon;
			#else
			*(Ptr*)p->csParam = (Ptr)SDiskIcon;
			#endif
			return noErr;
		case kDriveInfo:
			//  high word (bytes 2 & 3) clear
			//  byte 1 = primary + fixed media + internal
			//  byte 0 = drive type (0x01 is unspecified drive)
			*(long*)p->csParam = 0x00000401;
			return noErr;
		case 24: // Return SCSI partition size
			*(long*)p->csParam = SDiskSize / 512;
			return noErr;
		default: return controlErr;
	}
}

#pragma parameter __D0 RBCtl(__A0, __A1)
OSErr RBCtl(CntrlParamPtr p, DCtlPtr d) {
	RDiskStorage_t *c;
	// Fail if dCtlStorage null
	if (!d->dCtlStorage) { return notOpenErr; }
	// Dereference dCtlStorage to get pointer to our context
	c = *(RDiskStorage_t**)d->dCtlStorage;
	// Handle control request based on csCode
	switch (p->csCode) {
		case killCode:
			return noErr;
		case accRun:
			d->dCtlFlags &= ~dNeedTimeMask; // Disable accRun
			c->sStatus.diskInPlace = 8; // 8 is nonejectable disk
			PostEvent(diskEvt, c->sStatus.dQDrive); // Post disk inserted event
			return noErr;
		case 2351: // Post-boot
			c->initialized = 1; // Skip initialization
			d->dCtlDelay = 30; // Set accRun delay (30 ticks is 0.5 sec.)
			d->dCtlFlags |= dNeedTimeMask; // Enable accRun
			return noErr;
	}

	// Otherwise, dispatch to correct drive
	if (p->ioVRefNum == c->sStatus.dQDrive) {
		return SDCtl(p, d, c);
	} else if (p->ioVRefNum == c->rStatus.dQDrive) {
		return RDCtl(p, d, c);
	} else { return nsvErr; }
}

#pragma parameter __D0 RBStat(__A0, __A1)
OSErr RBStat(CntrlParamPtr p, DCtlPtr d) {
	RDiskStorage_t *c;
	// Fail if dCtlStorage null
	if (!d->dCtlStorage) { return notOpenErr; }
	// Dereference dCtlStorage to get pointer to our context
	c = *(RDiskStorage_t**)d->dCtlStorage;
	// Handle status request based on csCode
	switch (p->csCode) {
		case kDriveStatus:
			// Otherwise, copy correct drive status
			if (p->ioVRefNum == c->sStatus.dQDrive) {
				BlockMove(&c->sStatus, &p->csParam, sizeof(DrvSts2));
			} else if (p->ioVRefNum == c->rStatus.dQDrive) {
				BlockMove(&c->rStatus, &p->csParam, sizeof(DrvSts2));
			} else { return nsvErr; }
			return noErr;
		default: return statusErr;
	}
}

#pragma parameter __D0 RBClose(__A0, __A1)
OSErr RBClose(IOParamPtr p, DCtlPtr d) {
	// If dCtlStorage not null, dispose of it
	if (!d->dCtlStorage) { return noErr; }
	//RDiskStorage_t *c = *(RDiskStorage_t**)d->dCtlStorage;
	HUnlock(d->dCtlStorage);
	DisposeHandle(d->dCtlStorage);
	d->dCtlStorage = NULL;
	return noErr;
}
