.global spi_delay
.global _reg_write16
.global _reg_write8
.global smear8to32

* delay calling convention
* D0 - iterations

spi_delay:
    subq.w #1, %D0
    bne.b spi_delay
    rts

* reg_write calling convention
* A0 - address
* D0 - data (clobbered)
* D1 - clobbered

_reg_write8:
    move.l %A0, %D1
    andi.l #0xFFFFFF00, %D1
    andi.l #0x000000FF, %D0
    or %D1, %D0
    movea %D0, %A0
    move.l (%A0), %D0
    rts

_reg_write16:
    move.l %A0, %D1
    andi.l #0xFFFF0000, %D1
    andi.l #0x0000FFFF, %D0
    or %D1, %D0
    movea %D0, %A0
    move.l (%A0), %D0
    rts

* smear8to32 calling convention
* D0 - data in/out

smear8to32:
    andi.l #0xFF, %D0
    move.l %D0, %D1
    lsl.w #8, %D1
    or.w %D1, %D0
    lsl.w #8, %D1
    or.w %D1, %D0
    lsl.w #8, %D1
    or.w %D1, %D0
    rts
