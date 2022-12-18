#ifndef _SPI_H
#define _SPI_H

#include <stddef.h>

int spi_init();

void spi_cs(int cs);

char spi_txrx8_slow(char txd);
char spi_txrx8(char txd);
char spi_rxtx8(char txd);

void spi_tx(char *txb, unsigned int length);
void spi_rx(char txd, char *rxb, unsigned int length);

char spi_rd8();
short spi_rd16();

#endif