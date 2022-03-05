; Test of IRP unclosed with EXITM condition

IRP varname, value, 100, 2
    db 'Unclosed IRP'
    EXITM

; No ENDM

; End
