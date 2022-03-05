; Test of REPT unclosed condition

REPT 42, value, 100, 2
    db 'Unclosed REPT'

; No ENDM

; End
