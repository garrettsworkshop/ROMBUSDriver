	.macro  	xfer_in	from, to
	move.w		(%A0), (%A1)+
	nop
	nop
	nop
	.if			\to-\from
	xfer_in	"(\from+1)"
	.else
	
	.endif
	.endm


;pragma parameter xfer_s_256(__A0, __A1)
;void xfer_s_256(Ptr srcreg, Ptr destmem);
xfer_256:
	movem.l		%D0/%A1-%A2, -(%SP)
xfer_256_loop:
	xfer_s_in	0, 255
xfer_256_end:
	movem.l		(%SP)+, %D0/%A1-%A2


;pragma parameter xfer_s(__D0, __A0, __A1)
;void xfer_s(uint8_t numBytes, Ptr srcreg, Ptr destmem);
xfer:
	movem.l		%D0/%A1-%A2, -(%SP)
	andi.l 		#0xFF, %D0
	subi.l 		#256, %D0
	neg.l 		%D0
	lsl.l 		#2, %D0
	addi.l 		#xfer_256, %D0
	movea.l 	%D0, %A2
	jmp 		%A2
