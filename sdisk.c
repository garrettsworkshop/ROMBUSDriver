#include <Memory.h>
#include <Devices.h>
#include <Files.h>
#include <Disks.h>
#include <Errors.h>
#include <Events.h>
#include <OSUtils.h>
#include <Quickdraw.h>

#include "rombus.h"
#include "sdisk.h"

static char SPI_SetCS(char cs) {
	
}

static void SPI_TX8(char d) {
	
}

static char SPI_RX8(char d) {
	
}

static void SPI_TX8_Slow(char d) {
	
}

static char SPI_RX8_Slow(char d) {
	
}

OSErr SDOpen(IOParamPtr p, DCtlPtr d, RBStorage_t *c) {
	// Find first available drive number for SD drive
	drvNum = RBFindDrvNum();
	// Set SD drive status
	c->sdStatus.track = 0;
	c->sdStatus.writeProt = 0; // zero is writable
	c->sdStatus.diskInPlace = 8; // 8 is nonejectable disk
	c->sdStatus.installed = 1; // drive installed
	c->sdStatus.sides = 0;
	c->sdStatus.qType = 1;
	c->sdStatus.dQDrive = drvNum;
	c->sdStatus.dQFSID = 0;
	c->sdStatus.dQRefNum = d->dCtlRefNum;
	c->sdStatus.driveSize = SDiskSize / 512;
	c->sdStatus.driveS1 = (SDiskSize / 512) >> 16;
	// Decompress SD disk icon
	src = &SDiskIconCompressed[0];
	dst = &c->sdIcon[0];
	UnpackBits(&src, &dst, SDISK_ICON_SIZE);
	// Add SD disk to drive queue and return
	return RBAddDrive(c->sdStatus.dQRefNum, c->sdStatus.dQDrive, (DrvQElPtr)&c->sdStatus.qLink);
}

OSErr SDPrime(IOParamPtr p, DCtlPtr d, RBStorage_t *c) {
	return controlErr;
}

OSErr SDCtl(CntrlParamPtr p, DCtlPtr d, RBStorage_t *c) {
	// Handle control request based on csCode
	switch (p->csCode) {
		case kFormat: return controlErr;
		case kVerify:
			if (!c->sdStatus.diskInPlace) { return controlErr; }
			return noErr;
		case kEject:
			// "Reinsert" disk if ejected illegally
			if (c->sdStatus.diskInPlace) { 
				PostEvent(diskEvt, c->sdStatus.dQDrive);
			}
			return controlErr; // Eject not allowed so return error
		case kDriveIcon: case kMediaIcon: // Get icon
			*(Ptr*)p->csParam = (Ptr)c->sdIcon;
			return noErr;
		case kDriveInfo:
			//  high word (bytes 2 & 3) clear
			//  byte 1 = primary + fixed media + internal
			//  byte 0 = drive type (0x00 is ??)
			*(long*)p->csParam = 0x00000400;
			return noErr;
		case 24: // Return SCSI partition size
			*(long*)p->csParam = SDiskSize / 512;
			return noErr;
		default: return controlErr;
	}
}

OSErr SDStat(CntrlParamPtr p, DCtlPtr d, RBStorage_t *c) {
	// Handle status request based on csCode
	switch (p->csCode) {
		case kDriveStatus:
			BlockMove(&c->sdStatus, &p->csParam, sizeof(DrvSts2));
			return noErr;
		default: return statusErr;
	}
}

void SDClose(IOParamPtr p, DCtlPtr d, RBStorage_t *c) { }
