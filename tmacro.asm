;	tmacro.asm
;	Some tests of macro usage.

	org 100h	; To run in cp/m

start:

macro	bdos, function

	ld c, function
	call 5

	endm

lineend	macro
	ld e, 0Dh
	bdos 2
	ld e, 0Ah
	bdos 2

	endm

macro	pushall
	push af
	push bc
	push de
	push hl

	endm

popall	macro
	pop hl
	pop de
	pop bc
	pop af

	endm

	pushall

	ld de, hello
i1	bdos 9
i2:	lineend

	popall

	bdos 0

hello	db 'Hello, world.$'

	end start

; End of tmacro.asm
