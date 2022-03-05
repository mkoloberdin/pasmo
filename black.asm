	; black.asm

	; Fills slowly the spectrum screen.
	; This program test some capabilities of Pasmo.

	; Try for example:
	; pasmo --tapbas black.asm black.tap
	; pasmo --tapbas --equ NODELAY black.asm black.tap
	; pasmo --tapbas --equ DELAYVALUE=0F000h black.asm black.tap

	; To run it in fuse, for example:
	; fuse black.tap

;		Testing ? short-circuit operator
	org defined LOADPOS ? LOADPOS : 30000

	if defined FILLVALUE
value	equ FILLVALUE
	else
value	equ 0FFh
	endif

	if defined WITH_PAD
	defs WITH_PAD, &CD
	endif

start

	; Point to the screen bitmap.

screen		equ defined CPC ? 0C000h : 04000h
	; High word of end of screen
endscreen	equ defined CPC ? 00h : 58h

	ld a, value
	ld (current), a

	if defined LOOP
again:
	endif

	if defined USEHL
	ld hl, screen
	else
	ld ix,screen
	endif

other

	; Fills an octet of the screen with the value specified.

	ld a, (current)
	if defined USEHL
	ld (hl), a
	inc hl
	else
	ld (ix), a
	inc ix
	endif

	; Delay

	;if not defined NODELAY
	;if ~ defined NODELAY
	;if ! defined NODELAY
	; Testing short-circuit evaluation.
	if ! defined NODELAY && (! defined DELAYVALUE || DELAYVALUE != 0)
	;if ! defined NODELAY || ! defined DELAYVALUE || DELAYVALUE != 0

;	if defined DELAYVALUE
;loops	equ DELAYVALUE
;	else
;loops	equ 100h
;	endif
;		Testing ? short-circuit operator
loops	equ defined DELAYVALUE ? DELAYVALUE : 100h

;	ld bc, loops
;		Testing high and low operators
	ld b, high loops
	ld c, low loops

delay	dec bc
	ld a,b
	or c
	if defined USEJP
	jp nz, delay
	else
	jr nz, delay
	endif

	endif

	; Test if reached end of screen.

	ld a, 0

	if defined USEHL
	cp l
	else
	cp ixl
	endif

	if defined USEJP
	jp nz, other
	else
	jr nz, other
	endif
	ld a, endscreen

	if defined USEHL
	cp h
	else
	cp ixh
	endif

	if defined USEJP
	jp nz, other
	else
	jr nz, other
	endif

	if defined LOOP
	ld a, (current)
	inc a
	ld (current), a
	jp again
	else
	; Return to Basic.
	ret
	endif

current	db 0

	; Set the entry point, needed with the --tapbas option to autorun.
	end start
