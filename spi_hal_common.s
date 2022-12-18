* spi calling convention
* A0 - ROM register
* A1 - readback address
* A2 - RX buffer
* A3 - TX buffer
* A4 - clobbered
* D0 - length (clobbered)
* D1 - nops (clobbered)
* D2 - clobbered (save SR)
* D3 - clobbered (save CACR)

.macro spi_call table, maxnops
    * Limit %D0 (length) to 1-256
    subq.w #1, %D0
    andi.l #0xFF, %D0
    addq.w #1, %D0
    
    * Limit %D1 (nops) based on maxnops
    .if \maxnops == 1
        andi.l #1, %D1
    .elseif \maxnops == 3
        andi.l #3, %D1
    .elseif \maxnops == 7
        andi.l #7, %D1
    .elseif \maxnops == 15
        andi.l #15, %D1
    .elseif \maxnops != 0
        .error
    .endif

    * Convert length to offset
    * %D0 = -%D0 (-length)
    neg.l %D0
    * %D0 = %D0 + 256 (-length + 256)
    add.l #256, %D0

    .if \maxnops != 0
        * Combine nops with offset to get lookup table index
        * %D1 = %D1 * 4 * 256 (nops * 256)
        lsl.l #8, %D1
        * %D0 = %D0 + %D1 (offset + nops*256)
        or.l %D1, %D0
    .endif
    
    * Get index of entry point from lookup table
    * %D0 = table[%D0] (table[4*(length+nops*256)])
    move.l (\table - ., %PC, %D0.w : 4), %D0

    * Save status register and disable interrupts
    move.w %SR, %D2
    ori.w #0x0700, %SR

    * Save CACR in %D3
    movec %CACR, %D3

    * Copy CACR to %D1
    move.l %D3, %D1
    * Clear CACR bits:
    * DE  ('040 enable data cache)          (31)
    * WA  ('030 write allocate)             (13)
    * DBE ('030 data burst enable)          (12)
    * CD  ('030 clear data cache)           (11)
    * CED ('030 clear entry in data cache)  (10)
    * FD  ('030 freeze data cache)          (9)
    * ED  ('030 enable data cache)          (8)
    andi.l #0x7FFFC0FF, %D1
    * Set '030 CACR bits:
    * FD (freeze data cache) (9)
    ori.w #0x0200, %D1
    * Move back into CACR
    movec.l %D1, %CACR

    * Jump to entry point
    * (table + table[length*4 + nops*4*256])
    jmp (\table - ., %PC, %D0.l)

    * Return statement for use with parameter checking
    ret: rts
.endm

.macro lookup_table base, nbase, stride, n
    dc.l \nbase - \base
    dc.l \nbase + \stride - \base
    dc.l \nbase + \stride+\stride - \base
    dc.l \nbase + \stride+\stride+\stride - \base
    .if \n > 0
    lookup_table \base, \nbase + \stride+\stride+\stride+\stride, \stride, \n-4
    .endif
.endm

.macro unroll_table macro, nops, n
    .rept \n
        \macro \nops
    .endr
    move.w %D2, %SR
    movec.l %D3, %CACR
    rts
.endm
