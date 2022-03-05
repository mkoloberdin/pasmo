; Test of IF ELSE ENDIF

IF 0

; Nested IF in inactive
IF 1
    NOP
; Some label
l1: NOP
ENDIF

; Macro in inactive

some MACRO
    NOP
ENDM

ENDIF

IF 1

ELSE

; Nested IF in inactive ELSE
IF 1
    NOP
; Some label
l1: NOP
ENDIF

; Macro in inactive ELSE

some MACRO
    NOP
ENDM

ENDIF

;---------------------------------------------------------------

some_def EQU 1

IFDEF some_no_def
    .ERROR IFDEF failed when undef
ELSE
    NOP
ENDIF

IFDEF some_def
    NOP
ELSE
    .ERROR IFDEF failed when def
ENDIF

IFNDEF some_no_def
    NOP
ELSE
    .ERROR IFNDEF failed when undef
ENDIF

IFNDEF some_def
    .ERROR IFNDEF failed
ELSE
    NOP
ENDIF

; IF without ENDIF in macro

macrowithif MACRO
IF 0
    NOP
ENDM

    macrowithif

; IF without ENDIF in macro

macrowithelse MACRO
IF 1
ELSE
    NOP
ENDM

    macrowithelse

; End
