;	rept.asm
; Test of rept and irp directives.

; Macro with rept and irp inside.
hola	macro
	local unused, unused2

unused	rept 2
	db 'Rept inside macro', 0
	endm

unused2	irp ?reg, af,bc, de, hl
	push ?reg
	endm

	endm	; hola

;-------------------------------------

	rept 10
	db 'Hello, reptworld'
	endm

	rept 3

; Rept with calculated end condition.
n	defl 1
	rept 0FFFFh

n	defl n + 2

	if n gt 10
	exitm
	endif

	rept 4
	db n
	endm

	endm

	endm

; Macro call inside rept.
	rept 2
	hola
	endm

	end
