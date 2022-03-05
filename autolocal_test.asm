start:
    jp after

_label1: nop

    jp _label1

after:
_label1: nop

    jp _label1
    jp start

_def1   DEFL 42
    ld a, _def1

; DEFL ends autolocal
def2    DEFL 33
