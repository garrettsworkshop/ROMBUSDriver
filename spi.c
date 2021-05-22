#include "spi.h"

/*-------------------------------------------------------------------------*/
/* Platform dependent macros and functions needed to be modified           */
/*-------------------------------------------------------------------------*/
#define	CS_H()		bset(P0)	/* Set MMC CS "high" */
#define CS_L()		bclr(P0)	/* Set MMC CS "low" */
#define CK_H()		bset(P1)	/* Set MMC SCLK "high" */
#define	CK_L()		bclr(P1)	/* Set MMC SCLK "low" */
#define DI_H()		bset(P2)	/* Set MMC DI "high" */
#define DI_L()		bclr(P2)	/* Set MMC DI "low" */
#define DO			btest(P3)	/* Test MMC DO (high:true, low:false) */


// Command listing
/* T16: Transfer 16-bit
 * First, the 16-bit value encoded in the address bits A[17:2] is latched.
 * The data output mux is set to the current RXR and the cycle completes.
 * Shortly after /AS rises, the SPI transfer engine begins
 * transferring the latched value.
 */
/* T16S: Transfer 16-bit Swapped
 * Same as T16L but read data is byte-swapped.
 */
/* MERT: Measure Elapsed time and Reset Timer
 * The elapsed time since the last MERT command is returned in D[31:24]
 * and the timer is reset.
 */
/* SKIP1: Skip Clocks with MOSI "1"
 * 
 */
/* SKIP0: Skip Clocks with MOSI "0"
 *
 */
/* T8S: Transfer 8-bit Swapped
 * Same as T8 but read data is byte-swapped.
 */
/* T8: Transfer 8-bit
 * First, the 8-bit value encoded in the address bits A[9:2] is latched.
 * The data output mux is set to the current RXR and the cycle completes.
 * Shortly after /AS rises, the SPI transfer engine begins
 * transferring the latched value.
 */
/* WRC: Write Command
 * The command encoded in address bits A[9:2] is sent to the command target
 * corresponding to the address bits A[15:10].
 * Command targets:
 *	$00 - Set bitbang
 *			A[2] - SCS value
 */
/* RDRXR: Read Receive Data Register
 * The the current RXR is returned in D[31:16] and the cycle completes.
 */
/* RDRXRS: Read Receive Data Register Swapped
 * Same as RDRXR but read data is byte-swapped.
 */
/* MAGIC: Write Command
 * Write sequence $FF $00 $55 $AA $C1 $AD
 * to enable registers at $40890000-$4097FFFF.
 * Write anything else to disable them.
 * Always reads 0xC1AD
 */

// SPI controller address map:
// 40940000-4097FFFF (256 kB, D[31:16]) T16S. Write transfer data in A[17:2].
// 40900000-4093FFFF (256 kB, D[31:16]) T16. Write transfer data in A[17:2].
// 408C0000-408FFFFF (320 kB) reserved
// 408A0000-408AFFFF ( 64 kB, D[31:24]) WRC. Write port address in A[15:10] and data in A[9:2].
// 40891C00-4089FFFF ( 55 kB) reserved
// 40892000-408923FF (  1 kB, D[31:24]) MERT.
// 40891C00-40891FFF (  1 kB) SKIP1. Write bytes to skip in A[9:2].
// 40891800-40891BFF (  1 kB) SKIP0. Write bytes to skip in A[9:2].
// 40891400-408917FF (  1 kB, D[31:16]) T8S. Write transfer data in A[9:2].
// 40891000-408913FF (  1 kB, D[31:16]) T8. Write transfer data in A[9:2].
// 40890C00-40890FFF (  1 kB, D[31:16]) RDRXRS.
// 40890800-40890BFF (  1 kB, D[31:16]) RDRXR.
// 40890400-408907FF (  1 kB) reserved
// 40890000-408903FF (  1 kB, D[31:24]) MAGIC. Write magic numbers in A[9:2].
// 40880000-408FFFFF ( 64 kB, D[31:00]) ROMBUS driver data

#define RB_T16S(x)		(*(volatile int*)	(0x40940000 + ((x && 0xFFFF)<<02)) )
#define RB_T16(x)		(*(volatile int*)	(0x40900000 + ((x && 0xFFFF)<<02)) )
#define RB_RDS(a)		(*(volatile char*)	(0x408B0000 + ((a && 0x003F)<<10)) )
#define RB_WRC(a,d)		(*(volatile char*)	(0x408A0000 + ((a && 0x003F)<<10) 
														+ ((d && 0x00FF)<<02)) )
#define RB_MERT(x)		(*(volatile char*)	(0x40892000 + ((x && 0xFFFF)<<02)) )
#define RB_SKIP1(n)		(*(volatile char*)	(0x40891C00 + ((x && 0xFFFF)<<02)) )
#define RB_SKIP0(n)		(*(volatile char*)	(0x40891800 + ((x && 0xFFFF)<<02)) )
#define RB_T8S(x)		(*(volatile int*)	(0x40891400 + ((x && 0xFFFF)<<02)) )
#define RB_T8(x)		(*(volatile int*)	(0x40891000 + ((x && 0xFFFF)<<02)) )
#define RB_RDRXRS		(*(volatile int*)	(0x40890C00 + ((x && 0xFFFF)<<02)) )
#define RB_RDRXR		(*(volatile int*)	(0x40890800 + ((x && 0xFFFF)<<02)) )
#define RB_WRMOSI(x)	(*(volatile char*)	(0x40890400 + ((x && 0x0001)<<02)) )
#define RB_MAGIC(x)		(*(volatile char*)	(0x40890000 + ((x && 0xFFFF)<<02)) )

#define SPI_GET_MISO(d)	(d & 1)

void spi_select() { 
	ret = *SPI_CMD_SEL0;
	ret = *SPI_CMD_SEL1;
	ret = *SPI_CMD_SEL2;
	ret = *SPI_CMD_SEL3;
}
void spi_deselect() {
	ret = *SPI_CMD_DES0;
	ret = *SPI_CMD_DES1;
	ret = *SPI_CMD_DES2;
	ret = *SPI_CMD_DES3;
}

void spi_tx_slow(char d) {
	for (int i = 0; i < 8; i++) {
		*SPI_CMD_BBA((0 & 0x02) | (d & 0x01));
		*SPI_CMD_BBA((1 & 0x02) | (d & 0x01));
		d >>= 1;
	}
	*SPI_CMD_BBA((0 & 0x02) | (0 & 0x01));
}

char spi_rx_slow() {
	char ret = 0;
	for (int i = 0; i < 8; i++) {
		*SPI_CMD_BBA((0 & 0x02) | (1 & 0x01));
		*SPI_CMD_BBA((1 & 0x02) | (1 & 0x01));
		ret = (ret << 1) + (*SPI_CMD_BBA((1 & 0x02) | (1 & 0x01)) & 1);
	}
	return ret;
}

void spi_skip_slow(int n) {
	while (n-- > 0) {
		for (int i = 0; i < 8; i++) {
			*SPI_CMD_BBA((0 & 0x02) | (1 & 0x01));
			*SPI_CMD_BBA((1 & 0x02) | (1 & 0x01));
		}
	}
	*SPI_CMD_BBA((0 & 0x02) | (1 & 0x01));
}

void spi_tx_8(char d) {
	*SPI_CMD_SH8(d);
}

char spi_rx_8() {
	*SPI_CMD_SH8(0xFF);
	return *SPI_CMD_RD & 0xFF;
}

void spi_tx_16(int d) {
	*SPI_CMD_SH16(d);
}

int spi_rx_16() {
	*SPI_CMD_SH16(0xFFFF);
	return *SPI_CMD_RD & 0xFFFF;
}

void spi_skip(int n) {
	while (n-- > 0) {
		for (int i = 0; i < 8; i++) {
			*SPI_CMD_SH8(0xFF);
		}
	}
}
