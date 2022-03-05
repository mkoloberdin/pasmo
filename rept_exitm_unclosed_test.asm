; Test of REPT unclosed with EXITM condition

REPT 42, value, 100, 2
    db 'Unclosed REPT'
    EXITM

; No ENDM

; End
