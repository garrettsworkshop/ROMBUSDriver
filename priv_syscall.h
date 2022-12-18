#ifndef _PRIV_SYSCALL_H
#define _PRIV_SYSCALL_H

#include <Disks.h>
#include <OSUtils.h>

#pragma parameter __D0 PSReadXPRAM(__D0, __D1, __A0)
OSErr PSReadXPRAM(short numBytes, short whichByte, Ptr dest) = {0x4840, 0x3001, 0xA051};

#pragma parameter __D0 PSAddDrive(__D1, __D0, __A0)
OSErr PSAddDrive(short drvrRefNum, short drvNum, DrvQElPtr dq) = {0x4840, 0x3001, 0xA04E};

// Figure out the first available drive number >= 5
static int PSFindDrvNum() {
	DrvQElPtr dq;
	int drvNum = 5;
	for (dq = (DrvQElPtr)(GetDrvQHdr())->qHead; dq; dq = (DrvQElPtr)dq->qLink) {
		if (dq->dQDrive >= drvNum) { drvNum = dq->dQDrive + 1; }
	}
	return drvNum;
}

#endif
