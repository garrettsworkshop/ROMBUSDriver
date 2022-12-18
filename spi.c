#include "spi.h"
#include "spi_hal.h"

static int _search_lt(char *buf, int stride, int threshold) {
    for (int i = 1; i < 256; i++) {
        if (buf[i * stride] < threshold) { return 1; }
    }
    return 0;
}

int _spi_hal_rx8_nops, _spi_hal_tx8_nops;
int _spi_hal_rx16_nops, _spi_hal_tx16_nops;
int _spi_hal_rxtx8_nops;

short *_spi_reg_rx16;
char *_spi_reg_tx16;
short *_spi_reg_rd16;

int spi_init(int swap) {
    short buf16[256];
    char *buf8 = (char*)&buf16;

    for (int i = 0; i < 8; i++) {
        spi_hal_rx8(SPI_REG_TIMER8, buf8, 256, i);
        _spi_hal_rx8_nops = i;
        _spi_hal_tx8_nops = i > 0 ? i - 1 : 0;
        if (!_search_lt(buf8, 1, 8)) { break; }
    }

    for (int i = 0; i < 16; i++) {
        spi_hal_rx16(SPI_REG_TIMER16, buf16, 256, i);
        _spi_hal_rx16_nops = i;
        _spi_hal_tx16_nops = i > 0 ? i - 1 : 0;
        if (!_search_lt(buf8, 2, 8)) { break; }
    }
    
    for (int i = 0; i < 8; i++) {
        spi_hal_rxtx8(SPI_REG_EMPTY, SPI_REG_TIMER16, buf8, buf8, 256, i);
        _spi_hal_rxtx8_nops = i;
        if (!_search_lt(buf8, 1, 8)) { break; }
    }

    _spi_reg_rx16 = swap ? SPI_REG_RX16S : SPI_REG_RX16;
    _spi_reg_tx16 = swap ? SPI_REG_TX16S : SPI_REG_TX16;
    _spi_reg_rd16 = swap ? SPI_REG_RD16S : SPI_REG_RD16;
    return 0;
}

void spi_cs(int cs) {
    if (cs) { SPI_REG_CSR_SET_CS(); }
    else { SPI_REG_CSR_CLR_CS(); }
}

char spi_txrx8_slow(char txd) {
    char rxd = 0;
    for (int i = 7; i >= 0; i--) {
        spi_delay(64);
        SPI_REG_CSR_CLR_SCK();
        spi_delay(64);
        SPI_REG_SET_MOSI((txd >> i) & 1);
        rxd = (rxd << 1) | SPI_REG_CSR_GET_MISO();
        SPI_REG_CSR_SET_SCK();
    }
    spi_delay(64);
    SPI_REG_CSR_CLR_SCK();
    return rxd;
}

char spi_txrx8(char txd) {
    spi_hal_tx8(SPI_REG_TX8, &txd, 1, _spi_hal_tx8_nops);
    return *SPI_REG_RD8;
}

char spi_rxtx8(char txd) {
    char rxd = *SPI_REG_RD8;
    spi_hal_tx8(SPI_REG_TX8, &txd, 1, _spi_hal_tx8_nops);
    return rxd;
}

#define UNROLL_LENGTH 5

void spi_rx(char txd, char *rxb, unsigned int length) {
    if (length == 0) { return; } // Return if length 0

    // Word-align rx pointer by transferring 0/1 bytes
    if ((int)rxb & 1) {
        *(rxb++) = spi_rxtx8(txd);
        length--;
    }

    // Set tx pattern
    reg_write16(SPI_REG_ST16, smear8to32(txd));
    
    // Transfer all but 0-65 bytes
    for (int i = length >> (UNROLL_LENGTH +1); i > 0; i--) {
        // Transfer 2^UNROLL_LENGTH words (2 * 2^UNROLL_LENGTH bytes)
        spi_hal_rx16(_spi_reg_rx16, rxb, UNROLL_LENGTH, _spi_hal_rx16_nops);
    }
    
    // Transfer remaining 1-32 words (2-64 bytes)
    spi_hal_rx16(_spi_reg_rx16, rxb, length >> 1, _spi_hal_rx16_nops);

    // Transfer remaining byte if any
    if (length & 1) { *(rxb++) = spi_rxtx8(txd); }
}

void spi_tx(char *txb, unsigned int length) {
    if (length == 0) { return; } // Return if length 0

    // Word-align tx pointer by transferring 0/1 bytes
    if ((int)txb & 1) {
        spi_rxtx8(*(txb++));
        length--;
    }

    // Transfer all but 0-65 bytes
    for (; length > UNROLL_LENGTH; length -= UNROLL_LENGTH) {
        spi_hal_tx16(_spi_reg_tx16, txb, UNROLL_LENGTH, _spi_hal_tx16_nops);
    }
    
    // Transfer remaining 1-32 words (2-64 bytes)
    spi_hal_tx16(_spi_reg_tx16, txb, length >> 1, _spi_hal_tx16_nops);

    // Transfer remaining byte if any
    if (length & 1) { spi_rxtx8(*(txb++)); }
}

char spi_rd8() { return *SPI_REG_RD8; }
short spi_rd16() { return *_spi_reg_rd16; }
