;	local.asm
; Test of LOCAL, PROC and ENDP under CP/M.

bdos	equ 5
conout	equ 2

	org 100h

	jp hola

showtext	proc
	local hola, exit

hola	ld a, (hl)
	cp 0
	jp z, exit
	push hl
	ld e, a
	ld c, conout
	call bdos
	pop hl
	inc hl
	jp hola

exit	ret

	endp

hola	ld hl, exit
	call showtext
	jp 0

exit	db "Hello, world\r\n", 0

	end
