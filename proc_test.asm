PROC
    LOCAL somevar, other
; Redeclared local is a warning
    LOCAL somevar
; Unused local is a warning
    LOCAL unused
somevar:
other   EQU 42
    ret
ENDP

somevar     EQU 1
