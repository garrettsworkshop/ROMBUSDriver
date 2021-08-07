#ifndef RDISK_H
#define RDISK_H

#define RDiskBuf ((char*)0x40880000)
#define BufPtr ((Ptr*)0x10C)
#define MemTop ((Ptr*)0x108)
#define MMU32bit ((char*)0xCB2)

#define RDiskDBGDisPos (*(const unsigned long*)0x40851D98)
#define RDiskCDRDisPos (*(const unsigned long*)0x40851D9C)
#define RDiskDBGDisByte (*(const char*)0x40851DA8)
#define RDiskCDRDisByte (*(const char*)0x40851DA9)
#define RDiskSize (*(const unsigned long*)0x40851DAC)

#define RDISK_COMPRESS_ICON_ENABLE

#pragma parameter __D0 RDiskReadXPRAM(__D0, __D1, __A0)
OSErr RDiskReadXPRAM(short numBytes, short whichByte, Ptr dest) = {0x4840, 0x3001, 0xA051};

#pragma parameter __D0 RDiskAddDrive(__D1, __D0, __A0)
OSErr RDiskAddDrive(short drvrRefNum, short drvNum, DrvQElPtr dq) = {0x4840, 0x3001, 0xA04E};

static inline char RDiskIsRPressed() { return *((volatile char*)0x175) & 0x80; }

#define RDISK_ICON_SIZE (285)
typedef struct RDiskStorage_s {
	DrvSts2 status;
	char initialized;
	char dbgDisByte;
	char cdrDisByte;
	#ifdef RDISK_COMPRESS_ICON_ENABLE
	char icon[RDISK_ICON_SIZE+8];
	#endif
} RDiskStorage_t;

typedef void (*RDiskCopy_t)(Ptr, Ptr, unsigned long);
#define copy24(s, d, b) { RDiskCopy_t f = C24; f(s, d, b); }

typedef char (*RDiskPeek_t)(Ptr);
#define peek24(a, d) { RDiskPeek_t f = G24; d = f(a); }

#define PackBits_Repeat(count) (-1 * (count - 1))
#define PackBits_Literal(count) (count - 1)

#define RDISK_COMPRESSED_ICON_SIZE (87)
#ifdef RDISK_COMPRESS_ICON_ENABLE
#include <Quickdraw.h>
const char RDiskIconCompressed[RDISK_COMPRESSED_ICON_SIZE] = {
	PackBits_Repeat(76), 0b00000000, /*
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, */
	PackBits_Repeat(4), 0b11111111, /*
	0b11111111, 0b11111111, 0b11111111, 0b11111111, */
	PackBits_Literal(36),
	0b10000000, 0b00000000, 0b00000000, 0b00000001,
	0b10001111, 0b00011110, 0b00111100, 0b01111001,
	0b10001001, 0b00010010, 0b00100100, 0b01001001,
	0b10001001, 0b00010010, 0b00100100, 0b01001001,
	0b10001001, 0b00010010, 0b00100100, 0b01001001,
	0b10001111, 0b00011110, 0b00111100, 0b01111001,
	0b11000000, 0b00000000, 0b00000000, 0b00000001,
	0b01010101, 0b01010101, 0b11010101, 0b01010101,
	0b01111111, 0b11111111, 0b01111111, 0b11111111,
	PackBits_Repeat(12), 0b00000000, /*
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, */
	
	PackBits_Repeat(76), 0b00000000, /*
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, */
	PackBits_Repeat(32), 0b11111111, /*
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111, */
	PackBits_Literal(1), 0b01111111, 
	PackBits_Repeat(3), 0b11111111, /*
	0b01111111, 0b11111111, 0b11111111, 0b11111111, */
	PackBits_Literal(1), 0b01111111, 
	PackBits_Repeat(3), 0b11111111, /*
	0b01111111, 0b11111111, 0b11111111, 0b11111111, */
	PackBits_Repeat(12), 0b00000000, /*
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, */
	PackBits_Literal(29),
	27, 'G', 'a', 'r', 'r', 'e', 't', 't', '\'', 's', ' ', 
	'W', 'o', 'r', 'k', 's', 'h', 'o', 'p', ' ', 
	'R', 'O', 'M', ' ', 'D', 'i', 's', 'k', 0
};
#else
const char RDiskIcon[RDISK_ICON_SIZE] = {
	// Icon
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b10000000, 0b00000000, 0b00000000, 0b00000001,
	0b10001111, 0b00011110, 0b00111100, 0b01111001,
	0b10001001, 0b00010010, 0b00100100, 0b01001001,
	0b10001001, 0b00010010, 0b00100100, 0b01001001,
	0b10001001, 0b00010010, 0b00100100, 0b01001001,
	0b10001111, 0b00011110, 0b00111100, 0b01111001,
	0b11000000, 0b00000000, 0b00000000, 0b00000001,
	0b01010101, 0b01010101, 0b11010101, 0b01010101,
	0b01111111, 0b11111111, 0b01111111, 0b11111111,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	// Mask
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b11111111, 0b11111111, 0b11111111, 0b11111111,
	0b01111111, 0b11111111, 0b11111111, 0b11111111,
	0b01111111, 0b11111111, 0b11111111, 0b11111111,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000,
	27, 'G', 'a', 'r', 'r', 'e', 't', 't', '\'', 's', ' ', 
	'W', 'o', 'r', 'k', 's', 'h', 'o', 'p', ' ', 
	'R', 'O', 'M', ' ', 'D', 'i', 's', 'k', 0
};
#endif

#endif
