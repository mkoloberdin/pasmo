;	align.asm

align	macro n

	org ($ + n - 1) / n * n

	endm

	org 100h
	rst 0
	align 8
	rst 0
	rst 0
	rst 0
	align 64
	align 16
	rst 0

	end
