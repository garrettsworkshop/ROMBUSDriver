* spi calling convention
* A0 - ROM register
* A1 - readback address
* A2 - RX buffer
* A3 - TX buffer
* A4 - clobbered
* D0 - length (clobbered)
* D1 - nops (clobbered)

.global _spi_hal_rx16

.include "spi_hal_common.s"

.macro _spi_hal_rx16_iteration nops
    move.w (%A0), (%A2)+
    .rept \nops
        nop
    .endr
.endm

.align 16
_spi_hal_rx16:
    spi_call _spi_hal_rx16_lookup, 15
.align 16
_spi_hal_rx16_lookup:
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_0,  2,  256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_1,  4,  256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_2,  6,  256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_3,  8,  256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_4,  10, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_5,  12, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_6,  14, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_7,  16, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_8,  18, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_9,  20, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_10, 22, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_11, 24, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_12, 26, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_13, 28, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_14, 30, 256
    lookup_table _spi_hal_rx16_lookup, _spi_hal_rx16_table_15, 32, 256
.align 16
_spi_hal_rx16_table_0:  unroll_table _spi_hal_rx16_iteration, 0, 256
.align 16
_spi_hal_rx16_table_1:  unroll_table _spi_hal_rx16_iteration, 1, 256
.align 16
_spi_hal_rx16_table_2:  unroll_table _spi_hal_rx16_iteration, 2, 256
.align 16
_spi_hal_rx16_table_3:  unroll_table _spi_hal_rx16_iteration, 3, 256
.align 16
_spi_hal_rx16_table_4:  unroll_table _spi_hal_rx16_iteration, 4, 256
.align 16
_spi_hal_rx16_table_5:  unroll_table _spi_hal_rx16_iteration, 5, 256
.align 16
_spi_hal_rx16_table_6:  unroll_table _spi_hal_rx16_iteration, 6, 256
.align 16
_spi_hal_rx16_table_7:  unroll_table _spi_hal_rx16_iteration, 7, 256
.align 16
_spi_hal_rx16_table_8:  unroll_table _spi_hal_rx16_iteration, 8, 256
.align 16
_spi_hal_rx16_table_9:  unroll_table _spi_hal_rx16_iteration, 9, 256
.align 16
_spi_hal_rx16_table_10: unroll_table _spi_hal_rx16_iteration, 10, 256
.align 16
_spi_hal_rx16_table_11: unroll_table _spi_hal_rx16_iteration, 11, 256
.align 16
_spi_hal_rx16_table_12: unroll_table _spi_hal_rx16_iteration, 12, 256
.align 16
_spi_hal_rx16_table_13: unroll_table _spi_hal_rx16_iteration, 13, 256
.align 16
_spi_hal_rx16_table_14: unroll_table _spi_hal_rx16_iteration, 14, 256
.align 16
_spi_hal_rx16_table_15: unroll_table _spi_hal_rx16_iteration, 15, 256
