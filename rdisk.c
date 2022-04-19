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

OSErr RDOpen(IOParamPtr p, DCtlPtr d, RBStorage_t *c) {
	// Find first available drive number for ROM drive
	drvNum = RBFindDrvNum();
	// Set ROM drive status
	c->rdStatus.track = 0;
	c->rdStatus.writeProt = -1; // nonzero is write protected
	c->rdStatus.diskInPlace = 8; // 8 is nonejectable disk
	c->rdStatus.installed = 1; // drive installed
	c->rdStatus.sides = 0;
	c->rdStatus.qType = 0;
	c->rdStatus.dQDrive = drvNum;
	c->rdStatus.dQFSID = 0;
	c->rdStatus.dQRefNum = d->dCtlRefNum;
	c->rdStatus.driveSize = RDiskSize / 512;
	//c->rdStatus.driveS1 = (RDiskSize / 512) >> 16;
	// Decompress ROM disk icon
	src = &RDiskIconCompressed[0];
	dst = &c->rdIcon[0];
	UnpackBits(&src, &dst, RDISK_ICON_SIZE);
	// Add ROM disk to drive queue and return
	return RBAddDrive(c->rdStatus.dQRefNum, c->rdStatus.dQDrive, (DrvQElPtr)&c->rdStatus.qLink);
}

OSErr RDPrime(IOParamPtr p, DCtlPtr d, RBStorage_t *c) {
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

	// Update count and 1position/offset, then return
	d->dCtlPosition += p->ioReqCount;
	p->ioActCount = p->ioReqCount;
	return noErr;
}

OSErr RDCtl(CntrlParamPtr p, DCtlPtr d, RBStorage_t *c) {
	// Handle control request based on csCode
	switch (p->csCode) {
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
		default: return controlErr;
	}
}

OSErr RDStat(IOParamPtr p, DCtlPtr d, RBStorage_t *c) {
	// Handle status request based on csCode
	switch (p->csCode) {
		case kDriveStatus:
			BlockMove(&c->rdStatus, &p->csParam, sizeof(DrvSts2));
			return noErr;
		default: return statusErr;
	}
}

void RDClose(CntrlParamPtr p, DCtlPtr d, RBStorage_t *c) { }
