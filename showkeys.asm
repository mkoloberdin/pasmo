;	showkeys.asm

bdos	equ 5

	org 100h

sigue:	ld c, 6
	ld e, 0FDh
	call bdos

	cp 20h
	jp z, final

	call showreg
	jp sigue

final	ld c, 2
	ld e, 0Dh
	call bdos
	ld c, 2
	ld e, 0Ah
	call bdos
	call 0

showreg:
	push bc
	push hl

	push af

	push af

	ld e, 20h
	ld c, 2
	call bdos

	pop af

	; Para ver mejor al trazar.
	and 0F0h

	rrca
	rrca
	rrca
	rrca

	call shownibble

	pop af

	call shownibble

	pop hl
	pop bc
	ret	

shownibble:
	and 0Fh
	add a, 30h

	cp 3Ah
	jp c, isdigit

	add a, 7
isdigit:	

	ld e, a
	ld c, 2
	call bdos
	ret

;	End
