#ifndef ROMBUS_H
#define ROMBUS_H

#define BufPtr ((Ptr*)0x10C)
#define MemTop ((Ptr*)0x108)
#define MMU32bit ((char*)0xCB2)

#pragma parameter __D0 RBReadXPRAM(__D0, __D1, __A0)
OSErr RBReadXPRAM(short numBytes, short whichByte, Ptr dest) = {0x4840, 0x3001, 0xA051};

#pragma parameter __D0 RBAddDrive(__D1, __D0, __A0)
OSErr RBAddDrive(short drvrRefNum, short drvNum, DrvQElPtr dq) = {0x4840, 0x3001, 0xA04E};

static inline char RBIsSPressed() { return *((volatile char*)0x174) & 0x02; }
static inline char RBIsXPressed() { return *((volatile char*)0x174) & 0x80; }
static inline char RBIsRPressed() { return *((volatile char*)0x175) & 0x80; }

#define RDISK_ICON_SIZE (285)
#define SDISK_ICON_SIZE (285)
typedef struct RBStorage_s {
	DrvSts2 rdStatus;
	DrvSts2 sdStatus;
	char initialized;
	char mountROM;
	char mountSD;
	char rdIcon[RDISK_ICON_SIZE+8];
	char sdIcon[SDISK_ICON_SIZE+8];
} RBStorage_t;

typedef void (*RBCopy_t)(Ptr, Ptr, unsigned long);
#define copy24(s, d, b) { RBCopy_t f = C24; f(s, d, b); }

#define PackBits_Repeat(count) (-1 * (count - 1))
#define PackBits_Literal(count) (count - 1)

#endif
