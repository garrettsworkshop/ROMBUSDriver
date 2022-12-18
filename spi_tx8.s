* spi calling convention
* A0 - ROM register
* A1 - readback address
* A2 - RX buffer
* A3 - TX buffer
* A4 - clobbered
* D0 - length (clobbered)
* D1 - nops (clobbered)

.global _spi_hal_tx8

.include "spi_hal_common.s"

.macro _spi_hal_tx8_iteration nops
    move.b (%A3)+, %D1
    move.b (%A0, %D1.W), %D1
    .rept \nops
    nop
    .endr
.endm

.align 16
_spi_hal_tx8:
    spi_call _spi_hal_tx8_lookup, 7
.align 16
_spi_hal_tx8_lookup:
    lookup_table _spi_hal_tx8_lookup, _spi_hal_tx8_table_0, 6,  256
    lookup_table _spi_hal_tx8_lookup, _spi_hal_tx8_table_1, 8,  256
    lookup_table _spi_hal_tx8_lookup, _spi_hal_tx8_table_2, 10, 256
    lookup_table _spi_hal_tx8_lookup, _spi_hal_tx8_table_3, 12, 256
    lookup_table _spi_hal_tx8_lookup, _spi_hal_tx8_table_4, 14, 256
    lookup_table _spi_hal_tx8_lookup, _spi_hal_tx8_table_5, 16, 256
    lookup_table _spi_hal_tx8_lookup, _spi_hal_tx8_table_6, 18, 256
    lookup_table _spi_hal_tx8_lookup, _spi_hal_tx8_table_7, 20, 256
.align 16
_spi_hal_tx8_table_0: unroll_table _spi_hal_tx8_iteration, 0, 256
.align 16
_spi_hal_tx8_table_1: unroll_table _spi_hal_tx8_iteration, 1, 256
.align 16
_spi_hal_tx8_table_2: unroll_table _spi_hal_tx8_iteration, 2, 256
.align 16
_spi_hal_tx8_table_3: unroll_table _spi_hal_tx8_iteration, 3, 256
.align 16
_spi_hal_tx8_table_4: unroll_table _spi_hal_tx8_iteration, 4, 256
.align 16
_spi_hal_tx8_table_5: unroll_table _spi_hal_tx8_iteration, 5, 256
.align 16
_spi_hal_tx8_table_6: unroll_table _spi_hal_tx8_iteration, 6, 256
.align 16
_spi_hal_tx8_table_7: unroll_table _spi_hal_tx8_iteration, 7, 256
