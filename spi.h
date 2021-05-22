#ifndef SPI_H
#define SPI_H

void spi_select();
void spi_deselect();

void spi_tx_slow(char d);
char spi_rx_slow();
void spi_skip_slow(int n);

void spi_tx_8(char d);
char spi_rx_8();
void spi_tx_16(int d);
int spi_rx_16();
void spi_skip(int n);

#endif
