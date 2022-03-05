#!/bin/sh

BIN="asmtested.bin"
SYM="asmtested.sym"

PASMO=./pasmo

count=0

ok ( )
{
    local r
    if [ $1 -ne 0 ]
    then
        r='not '
    fi
    count=$((count + 1))
    echo $r'ok '$count - $2
}

assemble ()
{
    local prog
    prog=$1
    shift
    ${PASMO} $@ $prog $BIN
    ok $? "Assembled $prog"
}

assemble_failed ()
{
    local prog
    prog=$1
    shift
    ${PASMO} $@ $prog $BIN
    ok $((! $?)) "Assemble failed $prog"
}

echo '1..42'

${PASMO} all.asm $BIN
cmp -s $BIN all.check
ok $? 'Assembled all.asm'

${PASMO} --86 --equ ONLY86 all.asm $BIN
ok $? 'Assembled all 86'

${PASMO} tmacro.asm $BIN
ok $? 'Assembled tmacro.asm'

assemble local.asm

assemble undoc.asm

assemble if_test.asm

assemble defl_test.asm

${PASMO} --prl defl_test.asm $BIN
ok $? 'Assembled defl_test.asm to prl'

#${PASMO} --equ condition=1 if_unclosed_test.asm $BIN
#ok $((! $?)) 'Assemble failed if_unclosed_test.asm active'

assemble_failed if_unclosed_test.asm --equ condition=1

${PASMO} --equ condition=0 if_unclosed_test.asm $BIN
ok $((! $?)) 'Assemble failed if_unclosed_test.asm not active'

${PASMO} --equ condition=1 else_unclosed_test.asm $BIN
ok $((! $?)) 'Assemble failed else_unclosed_test.asm not active'

${PASMO} --equ condition=0 else_unclosed_test.asm $BIN
ok $((! $?)) 'Assemble failed else_unclosed_test.asm active'

assemble proc_test.asm

${PASMO} proc_unclosed_test.asm $BIN $SYM
ok $((! $?)) 'Assemble failed proc_unclosed_test.asm'

${PASMO} rept_test.asm $BIN $SYM
ok $? 'Assembled rept_test.asm'

${PASMO} rept_unclosed_test.asm $BIN $SYM
ok $((! $?)) 'Assemble failed rept_unclosed_test.asm'

${PASMO} rept_exitm_unclosed_test.asm $BIN $SYM
ok $((! $?)) 'Assemble failed rept_exitm_unclosed_test.asm'

${PASMO} irp_unclosed_test.asm $BIN $SYM
ok $((! $?)) 'Assemble failed irp_unclosed_test.asm'

${PASMO} irp_exitm_unclosed_test.asm $BIN $SYM
ok $((! $?)) 'Assemble failed irp_exitm_unclosed_test.asm'

${PASMO} macro_unclosed_test.asm $BIN
ok $((! $?)) 'Assemble failed macro_unclosed_test.asm'

${PASMO} macro_endp_test.asm $BIN
ok $((! $?)) 'Assemble failed macro_endp_test.asm'

${PASMO} --alocal autolocal_test.asm $BIN
ok $? 'Assembled autolocal_test.asm'

${PASMO} -I testaux include_test.asm $BIN
ok $? 'Assembled include_test.asm'

${PASMO} include_bad_test.asm $BIN
ok $((! $?)) 'Assemble failed include_bad_test.asm'

${PASMO} -I ./somedir include_notfound_test.asm $BIN
ok $((! $?)) 'Assemble failed include_notfound_test.asm'

${PASMO} -v -d --bin --public -B -- public_test.asm $BIN $SYM
ok $? 'Assembled public_test.asm'

${PASMO}
ok $((! $?)) 'Usage'

${PASMO} all.asm
ok $((! $?)) 'Output file required'

${PASMO} --notavalidoption
ok $((! $?)) 'Invalid option'

${PASMO} -I
ok $((! $?)) 'Option -I needs argument'

${PASMO} --equ
ok $((! $?)) 'Option --equ needs argument'

${PASMO} --name
ok $((! $?)) 'Option --name needs argument'

${PASMO} --tapbas black.asm black.tap
ok $? 'Generate tapbas'

${PASMO} --tap black.asm black.tap
ok $? 'Generate tap'

${PASMO} --tzx black.asm black.tzx
ok $? 'Generate tzx'

${PASMO} --cdt --name blackisblackisblack --equ CPC --equ NODELAY black.asm black.cdt
ok $? 'Generate cdt'

# Add pad to use more than one block
${PASMO} --cdtbas --equ CPC --equ NODELAY --equ WITH_PAD=3000 black.asm black.cdt
ok $? 'Generate cdtbas'

# Test name longer than max in header
${PASMO} --tzxbas --name blackisblack black.asm black.tzx
ok $? 'Generate tzxbas'

${PASMO} --plus3dos --pass3 black.asm black.p3d
ok $? 'Generate plus3dos'

${PASMO} --amsdos --equ CPC --equ NODELAY black.asm black.ams
ok $? 'Generate amsdos'

# Test name without dot
${PASMO} --amsdos --name blackisblackisblack --w8080 --werror --equ USEHL --equ USEJP --equ CPC --equ LOOP --equ NODELAY black.asm black.ams
ok $? 'Generate amsdos with name'

${PASMO} incbin_test.asm $BIN
cmp -s $BIN all.check
ok $? 'Assembled incbin_test.asm'

# End
