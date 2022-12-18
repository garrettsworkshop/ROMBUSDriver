#ifndef _SPI_HAL_H
#define _SPI_HAL_H

#pragma parameter spi_delay(__D0)
extern void spi_delay(char iterations);

#pragma parameter _reg_write8(__A0, __D0, __D1)
extern void _reg_write8(void *addr, char data, int tmp);
static inline void reg_write8(void *addr, char data) { _reg_write8(addr, data, 0); }

#pragma parameter _reg_write16(__A0, __D0, __D1)
extern void _reg_write16(void *addr, short data, int tmp);
static inline void reg_write16(void *addr, short data) { _reg_write16(addr, data, 0); }

#pragma parameter __D0 smear8to32(__D0)
extern long smear8to32(char data);

// Read transfer registers
#define SPI_REG_RX8     ((char*)    0x00000000) // A[01:00]==2'b00, D[31:24]==ret
#define SPI_REG_RX16    ((short*)   0x00000000) // A[01:00]==2'b00, D[31:16]==ret
#define SPI_REG_RX16S   ((short*)   0x00000000) // A[01:00]==2'b00, D[31:16]==ret
#define SPI_REG_RD8     ((char*)    0x00000000) // A[01:00]==2'b00, D[31:24]==ret
#define SPI_REG_RD16    ((short*)   0x00000000) // A[01:00]==2'b00, D[31:16]==ret
#define SPI_REG_RD16S   ((short*)   0x00000000) // A[01:00]==2'b00, D[31:16]==ret
#define SPI_REG_TIMER8  ((char*)    0x00000000) // A[01:00]==2'b00, D[31:24]==ret
#define SPI_REG_TIMER16 ((short*)   0x00000000) // A[01:00]==2'b00, D[31:16]==ret
// Write transfer registers
#define SPI_REG_TX8     ((char*)    0x00000000) // A[07:00]==arg
#define SPI_REG_TX16    ((char*)    0x00000000) // A[15:00]==arg
#define SPI_REG_TX16S   ((char*)    0x00000000) // A[05:00]==arg
#define SPI_REG_ST16    ((char*)    0x00000000) // A[15:00]==arg
#define SPI_REG_EMPTY   ((char*)    0x00000000)
// Control/status register (read/write)
#define SPI_REG_RD_CSR  ((char*)    0x00000000)
#define SPI_REG_WR_CSR  ((char*)    0x00000000)
#define SPI_REG_CSR_BIT_nDET    (0)
#define SPI_REG_CSR_GET_nDET()  ((*SPI_REG_RD_CSR>>SPI_REG_CSR_BIT_nDET) & 1)
#define SPI_REG_CSR_BIT_MISO    (1)
#define SPI_REG_CSR_GET_MISO()  ((*SPI_REG_RD_CSR>>SPI_REG_CSR_BIT_MISO) & 1)
#define SPI_REG_CSR_BIT_SCK     (2)
#define SPI_REG_CSR_SET_SCK()   reg_write16(SPI_REG_WR_CSR, *SPI_REG_RD_CSR |  (1<<SPI_REG_CSR_BIT_SCK))
#define SPI_REG_CSR_CLR_SCK()   reg_write16(SPI_REG_WR_CSR, *SPI_REG_RD_CSR & ~(1<<SPI_REG_CSR_BIT_SCK))
#define SPI_REG_CSR_BIT_CS      (3)
#define SPI_REG_CSR_SET_CS()    reg_write16(SPI_REG_WR_CSR, *SPI_REG_RD_CSR |  (1<<SPI_REG_CSR_BIT_CS))
#define SPI_REG_CSR_CLR_CS()    reg_write16(SPI_REG_WR_CSR, *SPI_REG_RD_CSR & ~(1<<SPI_REG_CSR_BIT_CS))

#define SPI_REG_SET_MOSI(x)     reg_write16(SPI_REG_ST16, x ? 0xFFFF : 0x0000)

#pragma parameter _spi_hal_rx8(__A0, __A2, __A4, __D0, __D1, __D2)
extern void _spi_hal_rx8(void *reg, void *rx, void *tmp, int length, int nops, int tmp2);
static inline void spi_hal_rx8(void *reg, void *rx, int length, int nops) { 
    _spi_hal_rx8(reg, rx, 0, length, nops, 0);
}

#pragma parameter _spi_hal_rx16(__A0, __A2, __A4, __D0, __D1, __D2)
extern void _spi_hal_rx16(void *reg, void *rx, void *tmp, int length, int nops, int tmp2);
static inline void spi_hal_rx16(void *reg, void *rx, int length, int nops) { 
    _spi_hal_rx16(reg, rx, 0, length, nops, 0);
}

#pragma parameter _spi_hal_tx8(__A0, __A3, __A4, __D0, __D1, __D2)
extern void _spi_hal_tx8(void *reg, void *tx, void *tmp, int length, int nops, int tmp2);
static inline void spi_hal_tx8(void *reg, void *tx, int length, int nops) { 
    _spi_hal_tx8(reg, tx, 0, length, nops, 0);
}

#pragma parameter _spi_hal_tx16(__A0, __A3, __A4, __D0, __D1, __D2)
extern void _spi_hal_tx16(void *reg, void *tx, void *tmp, int length, int nops, int tmp2);
static inline void spi_hal_tx16(void *reg, void *tx, int length, int nops) { 
    _spi_hal_tx16(reg, tx, 0, length, nops, 0);
}

#pragma parameter _spi_hal_rxtx8(__A0, __A1, __A2, __A3, __A4, __D0, __D1, __D2)
extern void _spi_hal_rxtx8(void *reg, void *read, void *rx, void *tx, void *tmp, int length, int nops, int tmp2);
static inline void spi_hal_rxtx8(void *reg, void *read, void *rx, void *tx, int length, int nops) { 
    _spi_hal_rxtx8(reg, read, rx, tx, 0, length, nops, 0);
}

#endif
