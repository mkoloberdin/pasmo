; Test of DEFL

    NOP
    JP after

IFDEF df1
    .ERROR "Unnexpected defined"
ENDIF

df1 DEFL 42

df1 DEFL 44

IF df1 != 44
    .ERROR "Wrong DEFL"
ENDIF

    NOP

after:
    ld de, msg
    ld c, 9
    call 5
    jp 0

msg: db 'Hello, world!', 0Dh, 0Ah, '$'

; End
