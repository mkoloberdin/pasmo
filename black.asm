	org 30000

	; Fills the spectrum screen.

start
	;ld hl, 4000h
	ld ix,4000h

other
	;ld (hl),0FFh
	;inc hl
	ld (ix),0FFh
	inc ix

	if 1

	; Delay
	ld bc, 100h
delay	dec bc
	ld a,b
	or c
	jr nz, delay

	endif

	ld a, 0
	;cp l
	cp ixl
	jr nz, other
	ld a, 58h
	;cp h
	cp ixh
	jr nz, other

	ret

	end start
