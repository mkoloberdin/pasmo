// test_asm.cxx

#include "token.h"
#include "asm.h"
#include "asmerror.h"

#include "test_protocol.h"

using namespace Pasmo::Test;

void throws_asmerror(const char * msg, void (*fn) (void))
{
    bool thr = false;
    try
    {
        fn();
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
        thr = true;
    }
    ok(thr, msg);
}

void parseline(Asm & as, const std::string line)
{
    Tokenizer tkz(line, false);
    as.parseline(tkz);
}

void parseline_throws(Asm & as, const std::string line, const char * msg)
{
    bool thr = false;
    try
    {
        parseline(as, line);
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
        thr = true;
    }
    ok(thr, msg);
}

void assembleline_throws(const char * line, const char *msg)
{
    bool thr = false;
    try
    {
        Asm as;
        parseline(as, line);
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
        thr = true;
    }
    ok(thr, msg);
}

void assembleline(const char * line, const char *msg)
{
    bool thr = false;
    try
    {
        Asm as;
        parseline(as, line);
        thr = true;
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
    }
    ok(thr, msg);
}

void predefine()
{
    Asm as;
    as.addpredef("something=42");
    ok(true, "Predefine with default");
    as.addpredef("somevalue=42");
    ok(true, "Predefine with value");
    parseline_throws(as, "somevalue DEFL 77", "DEFL predefined");
    parseline_throws(as, "somevalue EQU 77", "EQU predefined");

    throws_runtime("Invalid predefine symbol",
        [] ()
        {
            Asm as;
            as.addpredef("7=42");
        }
    );
    throws_runtime("Invalid predefine syntax",
        [] ()
        {
            Asm as;
            as.addpredef("something:42");
        }
    );
    throws_runtime("Invalid predefine string",
        [] ()
        {
            Asm as;
            as.addpredef("something='literal'");
        }
    );
    throws_runtime("Invalid predefine expression",
        [] ()
        {
            Asm as;
            as.addpredef("something=1 + 2");
        }
    );
}

void redefine_equ()
{
    Asm as;
    as.setpass(1);
    Tokenizer tkz;
    tkz = Tokenizer("abc EQU 1", false);
    as.parseline(tkz);
    tkz = Tokenizer("abc EQU 1", false);
    bool thr = false;
    try
    {
        as.parseline(tkz);
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
        thr = true;
    }
    ok(thr, "Redefine EQU throws");

    tkz = Tokenizer("xyz DEFL 1", false);
    as.parseline(tkz);
    tkz = Tokenizer("xyz EQU 1", false);
    thr = false;
    try
    {
        as.parseline(tkz);
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
        thr = true;
    }
    ok(thr, "Redefine DEFL with EQU throws");
    parseline_throws(as, "abc DEFL 99", "Redefine EQU with DEFL");
}

void redefine_entry()
{
    Asm as;
    as.setpass(2);
    Tokenizer tkz;
    tkz = Tokenizer("start: nop", false);
    as.parseline(tkz);
    tkz = Tokenizer("END start", false);
    as.parseline(tkz);
    tkz = Tokenizer("start2: nop", false);
    as.parseline(tkz);
    tkz = Tokenizer("END start2", false);
    bool thr = false;
    try
    {
        as.parseline(tkz);
        thr = true;
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
    }
    ok(thr, "Redefine entry point warns");
}

void warn8080()
{
    Asm as;
    as.warn8080();
    as.setpass(1);
    Tokenizer tkz;
    tkz = Tokenizer("EXX", false);
    as.parseline(tkz);
    ok(true, "Warning --w8080 option");
}

void no86()
{
    Asm as;
    as.set86();
    as.setpass(2);
    Tokenizer tkz;
    tkz = Tokenizer("EXX", false);
    bool thr = false;
    try
    {
        as.parseline(tkz);
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
        thr = true;
    }
    ok(thr, "Warning --w8080 option");
    parseline_throws(as, "DJNZ $ + 1000", "DJNZ out or range in 86 mode");
}

void bracket()
{
    Asm as;
    as.bracketonly();
    as.setpass(1);
    Tokenizer tkz;
    tkz = Tokenizer("ld b, [", false);
    bool thr = false;
    try
    {
        as.parseline(tkz);
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
        thr = true;
    }
    ok(thr, "Unclosed bracket in bracket only mode");

    tkz = Tokenizer("ld (hl), a", false);
    thr = false;
    try
    {
        as.parseline(tkz);
    }
    catch (const AsmError & ex)
    {
        diag(std::string("caught ") + ex.message());
        thr = true;
    }
    ok(thr, "Invalid parenthesis in bracket only mode");

    parseline_throws(as, "INC (HL)", "Invalid INC () in bracket only mode");
    parseline(as, "OUT [C], A");
    ok(true, "OUT with bracket in bracket only mode");

    parseline_throws(as, "EX (SP), HL", "Invalid EX (SP) in bracket only mode");

    parseline(as, "ORG 0100H");
    parseline(as, "LD SP, (12)");
    is(as.peekbyte(0x100), 0x31, "LD SP, (n) in bracket only mode");

    parseline(as, "ORG 0100H");
    parseline(as, "LD A, (012h)");
    parseline(as, "DB 0ABh");
    is(as.peekbyte(0x100), 0x3E, "LD A, (n) in bracket only mode");
    is(as.peekword(0x101), 0xAB12, "LD A, (n) in bracket only mode, argument");
    parseline(as, "ORG 0100H");
    parseline(as, "LD B, (012h)");
    is(as.peekbyte(0x100), 0x06, "LD B, (n) in bracket only mode");
}

void expressions()
{
    Asm as;
    as.setpass(2);
    parseline(as, "v1 EQU 2");
    is(as.getvalue("v1"), 2, "eval number");
    parseline(as, "v2 EQU v1 + v1");
    is(as.getvalue("v2"), 4, "eval +");
    parseline(as, "v3 EQU v2 * v2");
    is(as.getvalue("v3"), 16, "eval *");
    parseline(as, "v4 EQU v3 - v2");
    is(as.getvalue("v4"), 12, "eval -");
    parseline(as, "v5 EQU v3 / v1");
    is(as.getvalue("v5"), 8, "eval /");

    parseline(as, "v6 EQU v3 = v2 ? v2 : v1");
    is(as.getvalue("v6"), 2, "eval conditional");
    parseline(as, "v7 EQU v3 || v2 || v1");
    ok(as.getvalue("v7"), "eval ||");
    parseline(as, "v8 EQU v3 && v2 && v1");
    ok(as.getvalue("v8"), "eval &&");
    parseline(as, "v9 EQU NOT + v1");
    is(as.getvalue("v9"), ~2, "eval NOT");
    parseline(as, "v10 EQU ! v1");
    is(as.getvalue("v10"), 0, "eval logical !");

    parseline(as, "v11 EQU v3 MOD v4");
    is(as.getvalue("v11"), 4, "eval MOD");
    parseline_throws(as, "v12 EQU v1 / v10", "Division by zero");
    parseline_throws(as, "v13 EQU v1 MOD v10", "MOD by zero");

    parseline(as, "v14 EQU v1 SHL 2");
    is(as.getvalue("v14"), 8, "SHL");
    parseline(as, "v15 EQU v5 SHR 1");
    is(as.getvalue("v15"), 4, "SHR");

    parseline(as, "v16 EQU v1 < v2");
    ok(as.getvalue("v16"), "eval <");
    parseline(as, "v17 EQU v3 > v4");
    ok(as.getvalue("v17"), "eval >");
    parseline(as, "v18 EQU v1 <= v2");
    ok(as.getvalue("v18"), "eval <=");
    parseline(as, "v19 EQU v3 >= v4");
    ok(as.getvalue("v19"), "eval >=");
    parseline(as, "v20 EQU v3 != v4");
    ok(as.getvalue("v20"), "eval !=");

    parseline(as, "v21 EQU v2 AND v4");
    is(as.getvalue("v21"), 4, "eval AND");
    parseline(as, "v22 EQU v2 OR v4");
    is(as.getvalue("v22"), 12, "eval OR");
    parseline(as, "v23 EQU v2 XOR v4");
    is(as.getvalue("v23"), 8, "eval XOR");

    parseline(as, "v24 EQU 0ABCDh");
    parseline(as, "v25 EQU LOW v24");
    is(as.getvalue("v25"), 0xCD, "eval LOW");
    parseline(as, "v26 EQU HIGH v24");
    is(as.getvalue("v26"), 0xAB, "eval HIGH");

    parseline(as, "labelorg ORG 512");
    is(as.getvalue("labelorg"), 512, "ORG with label");

    parseline_throws(as, "JR $ + 1000", "JR out or range");
    parseline_throws(as, "DJNZ $ + 1000", "DJNZ out or range");

    parseline(as, "CP (42)");
    ok(true, "Suspicious CP (n)");

    parseline(as, "ORG 65535");
    parseline(as, "LD A, [IX + 1]");
    ok(true, "Cross 64 KiB insisde instruction");
}

void defined_var()
{
    Asm as;
    as.setpass(1);
    parseline(as, "v1 EQU DEFINED a1");
    parseline(as, "a1 EQU 1");
    as.setpass(2);
    // Defined in pass 1 gives not defined in pass 2
    parseline(as, "v1 EQU DEFINED a1");
    parseline(as, "v2 EQU DEFINED v1");
}

void autolocal()
{
    Asm as;
    as.setpass(1);
    as.autolocal();
    parseline_throws(as, "PUBLIC _autolocalvar", "autolocal declared PUBLIC");
    parseline(as, "PROC");
    parseline_throws(as, "LOCAL _autolocalvar", "autolocal declared LOCAL");
    parseline(as, "ENDP");
    parseline_throws(as, "MACRO _autolocalvar", "autolocal name as MACRO");
}

//**************************************************************

int main()
{
    plan(140);

    {
    Asm as;
    as.processfile();
    ok(true, "Process empty file");
    }

    {
    Asm as;
    Tokenizer tkz;
    tkz = Tokenizer("LD H, (n)", false);
    as.parseline(tkz);
    ok(true, "Parse LD H...");
    }

    assembleline(".Z80", ".Z80 directive");
    assembleline_throws(".Z80 nana", "invalid .Z80 directive");
    assembleline(".WARNING Message", ".WARNING directive");

    assembleline("PUBLIC var1, var2", "PUBLIC directive");

    assembleline("DB notdefined", "Using symbol not yet defined");
    assembleline("DB 1 || notdefined", "Using symbol not yet defined, ignored");
    assembleline("DB 'a'", "Literal length 1 as value");
    assembleline("DB ?", "Uninitialized DB");
    assembleline("DW 65535, 33", "DW immediate values");
    assembleline("DW ?", "Uninitialized DW");
    assembleline("i1 EQU NUL", "NUL empty");
    assembleline("i1 EQU NUL abc def", "NUL non empty");
    assembleline("DEFS 42", "DEFS with default fill");
    assembleline("DEFS 42, 127", "DEFS with explicit fill");
    assembleline("DEFS 42, ?", "DEFS unitialized");

    assembleline("JP [HL]", "JP [HL]");

    assembleline_throws(".ERROR Error message", ".ERROR directive");
    assembleline_throws("ORG notdefined", "ORG with undefined symbol");
    assembleline_throws("ORG 1 / 0", "Division by zero");
    assembleline_throws("EQU 42", "EQU without label");
    assembleline_throws("x1 EQU +", "invalid expression");
    assembleline_throws("DEFL 42", "DEFL without label");
    assembleline_throws("ORG 'abc' + 1", "Literal length 1 required");
    assembleline_throws("DB DEFINED 'a'", "DEFINED with invalid argument");
    assembleline_throws("'abc' def ghi", "No instruction");
    assembleline_throws("PROC nonsense", "PROC with invalid argument");

    assembleline_throws("LD 0, A", "Invalid LD");
    assembleline_throws("LD A B", "No comma");
    assembleline_throws("LD (HL), (HL)", "Invalid instruction");
    assembleline_throws("LD [HL), a", "Paren instead of bracket");
    assembleline_throws("LD (HL], a", "Bracket instead of paren");
    assembleline_throws("LD [IX), a", "Paren instead of bracket in index");
    assembleline_throws("LD (IX], a", "Bracket instead of paren in index");
    assembleline_throws("LD IXH, IYH", "Invalid LD IXH, IYH");
    assembleline_throws("LD IXH, IYL", "Invalid LD IXH, IYL");
    assembleline_throws("LD IYH, IXH", "Invalid LD IYH, IXH");
    assembleline_throws("LD IYH, IXL", "Invalid LD IYH, IXL");

    assembleline_throws("BIT 8, 1", "Invalid bit");

    assembleline_throws("LD A, (IX,1)", "Invalid index offset");
    assembleline_throws("LD A, (IX+256)", "Index offset out of range");
    assembleline_throws("LD A, (IX-129)", "Index offset negative out of range");
    assembleline("LD A, (IX)", "Implicit index offset 0");
    assembleline("LD A, [IX]", "Implicit index offset 0 with bracket");
    assembleline("LD A, (IX-1)", "Negative index offset");
    assembleline_throws("LD (256), B", "Invalid LD (nn) operand");
    assembleline_throws("LD (IX), (HL)", "Invalid LD (IX) operand");
    assembleline_throws("LD (IX), IYH", "Invalid LD (IX), IYH");

    assembleline_throws("INC R", "Invalid INC operand");
    assembleline_throws("INC (BC)", "Invalid INC () operand");
    assembleline_throws("PUSH R", "Invalid PUSH operand");

    assembleline_throws("ADD B, A", "Invalid ADD operand");
    assembleline_throws("ADD HL, A", "Invalid ADD HL operand");
    assembleline_throws("ADD HL, IX", "Invalid ADD HL, IX");
    assembleline_throws("ADD HL, IY", "Invalid ADD HL,I IY");
    assembleline_throws("ADD IX, HL", "Invalid ADD IX operand");
    assembleline_throws("ADC B, A", "Invalid ADC operand");
    assembleline_throws("SBC B, A", "Invalid SBC operand");

    assembleline_throws("RL BC", "Invalid RL operand");

    assembleline_throws("EX BC, DE", "Invalid EX first operand");
    assembleline_throws("EX AF, DE", "Invalid EX AF operand");
    assembleline_throws("EX DE, BC", "Invalid EX DE operand");
    assembleline_throws("EX (BC), DE", "Invalid EX ( first operand");
    assembleline_throws("EX (SP), DE", "Invalid EX (SP) operand");

    assembleline_throws("OUT A, C", "Invalid OUT");
    assembleline("OUT [C], A", "OUT with bracket");
    assembleline_throws("OUT (C), HL", "Invalid OUT operand");
    assembleline_throws("IN HL, (C)", "Invalid IN operand");
    assembleline_throws("JP (BC)", "Invalid JP () operand");
    assembleline_throws("JR P, $", "Invalid JR flag");

    assembleline_throws("IM 3", "Invalid IM");
    assembleline_throws("RST 1000", "Invalid RST");

    assembleline_throws("ENDIF", "ENDIF without IF");
    assembleline_throws("ELSE", "ELSE without IF");
    assembleline_throws("ENDP", "ENDP without PROC");
    assembleline_throws("ENDM", "ENDM without macro");

    assembleline_throws("IRP ident,", "IRP without parameters");
    assembleline_throws("PUBLIC 123", "PUBLIC invalid symbol");
    assembleline_throws("PUBLIC bad syntax", "PUBLIC without separator");

    assembleline_throws("some: thinh", "MACRO expected");

    assembleline_throws(".SHIFT", ".SHIFT outside of macro");

    assembleline_throws("INCBIN somefilename.bin", "INCBIN file not found");

    predefine();
    redefine_equ();
    redefine_entry();
    warn8080();
    no86();
    bracket();
    expressions();
    defined_var();
    autolocal();
}

// End
