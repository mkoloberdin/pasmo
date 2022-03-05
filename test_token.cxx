// test_token.cxx

#include "token.h"

#include "test_protocol.h"

#include <string>
#include <iostream>
#include <stdexcept>

using namespace Pasmo::Test;

//**************************************************************

void test_token()
{
    is(gettokenname(TypeDEFB), "DEFB",
            "gettokenname printable");
    is(gettokenname(TypeEndLine), std::string(1, TypeEndLine),
            "gettokenname no printable");

    Token tnum(1234);
    is(tnum.num(), 1234, "Numeric token");

    Token tok;
    is(tok.str(), "(undef)", "Undef token");
    tok = Token(TypeEndLine, "");
    is(tok.str(), "(eol)", "End line token");
    tok = Token(TypeIdentifier, "ident");
    is(tok.str(), "ident", "Identifier token");
    tok = Token(TypeLiteral, "literal");
    is(tok.str(), "literal", "Literal token");
    tok = Token(65535);
    is(tok.str(), "FFFF", "Number token");
    tok = Token(TypeDEFB, "DEFB");
    is(tok.str(), "DEFB", "Keyword token");
}

void test_tokenizer()
{
    Tokenizer tkz("a", false);
    Token tk = tkz.gettoken();
    tk = tkz.gettoken();
    tkz.ungettoken();
    tkz.ungettoken();

    tkz = Tokenizer("+ - * / = ~ ! ( ) [ ] ; comment", false);
    tkz = Tokenizer("100 ; line with number", false);

    tkz = Tokenizer(true);
    ok(tkz.getnocase(), "case insensitive tokenizer");
    ok(tkz.empty(), "empty tokenizer");
    tkz.push_back(Token(TypeIdentifier, "a"));
    nok(tkz.empty(), "non empty after push_back");

    tkz = Tokenizer(TypeEndOfInclude, "");
    nok(tkz.empty(), "created non empty");

    tkz = Tokenizer("AF'", false);
    tk = tkz.gettoken();
    is(tk.type(), TypeAFp, "alternative register AF");

    Tokenizer tkn2(tkz);
}

void test_unget()
{
    throws_logic("invalid ungettoken throws",
        [] ()
        {
            Tokenizer tkz("DB 'Hello'", false);
            tkz.ungettoken();
        }
    );
}

void test_include()
{
    throws_logic("invalid getincludefile throws", 
        [] ()
        {
            Tokenizer tkz("hi:", false);
            tkz.getincludefile();
        }
    );
    throws_runtime("Invalid INCLUDE",
        [] ()
        { Tokenizer tkz("INCLUDE", false); } );
    throws_runtime("Invalid quote in INCLUDE",
        [] ()
        { Tokenizer tkz("INCLUDE \"somefile", false); } );
    throws_runtime("Invalid single quote in INCLUDE",
        [] ()
        { Tokenizer tkz("INCLUDE 'somefile", false); } );

    Tokenizer tkz(".ERROR Error message", false);
    Token tk = tkz.gettoken();
    ok(tk.type() == Type_ERROR, ".ERROR directive");
    tk = tkz.gettoken();
    is(tk.type(), TypeLiteral, "error message token typr");
    is(tk.str(), "Error message", "error message content");
}

void test_chars()
{
    throws_runtime("Invalid control character",
        [] ()
        { Tokenizer tkz("\x04", false); } );
    throws_runtime("Invalid character",
        [] ()
        { Tokenizer tkz("`", false); } );
}

void test_unclosed()
{
    throws_runtime("Unclosed single quoted throws",
        [] ()
        { Tokenizer tkz("DB 'Hello", false); } );

    throws_runtime("Unclosed quoted throws",
        [] ()
        { Tokenizer tkz("DB \"Hello", false); } );
    throws_runtime("Unclosed escape throws",
        [] ()
        { Tokenizer tkz("DB \"Hello\\", false); } );
    throws_runtime("Unclosed octal escape throws",
        [] ()
        { Tokenizer tkz("DB \"Hello\\1", false); } );
    throws_runtime("Unclosed octal escape 2 digits throws",
        [] ()
        { Tokenizer tkz("DB \"Hello\\12", false); } );
    throws_runtime("Unclosed hex escape empty throws",
        [] ()
        { Tokenizer tkz("DB \"Hello\\x", false); } );
    throws_runtime("Unclosed hex escape throws",
        [] ()
        { Tokenizer tkz("DB \"Hello\\x1", false); } );
}

void test_invalid_num()
{
    throws_runtime("Empty 0x hex throws",
        [] ()
        { Tokenizer tkz("DB 0x", false); } );

    throws_runtime("Invalid % bin throws",
        [] ()
        { Tokenizer tkz("DB %11111111111111111 + 1", false); } );
    throws_runtime("Too big # hex throws",
        [] ()
        { Tokenizer tkz("DB #10000", false); } );
    throws_runtime("Too big $ hex throws",
        [] ()
        { Tokenizer tkz("DB $10000", false); } );

    throws_runtime("Empty &H throws",
        [] ()
        { Tokenizer tkz("DB &H", false); } );
    throws_runtime("Too big &H throws",
        [] ()
        { Tokenizer tkz("DB &H10000", false); } );
    throws_runtime("Invalid &O throws",
        [] ()
        { Tokenizer tkz("DB &O191", false); } );
    throws_runtime("Empty &O throws",
        [] ()
        { Tokenizer tkz("DB &O", false); } );
    throws_runtime("Empty &X throws",
        [] ()
        { Tokenizer tkz("DB &X", false); } );

    throws_runtime("Invalid suffixed dec throws",
        [] ()
        { Tokenizer tkz("DB 0ABd", false); } );
    throws_runtime("Invalid suffixed hex throws",
        [] ()
        { Tokenizer tkz("DB 0AGH", false); } );
    throws_runtime("Invalid suffixed oct throws",
        [] ()
        { Tokenizer tkz("DB 0ABq", false); } );
    throws_runtime("Invalid suffixed bin throws",
        [] ()
        { Tokenizer tkz("DB 02B", false); } );
}

void test_parse_expr()
{
    Tokenizer tkz;
    tkz = Tokenizer("value EQU a + 1 + 0x2 + b", false);
    tkz = Tokenizer("value EQU a <", false);
    tkz = Tokenizer("value EQU a < b", false);
    tkz = Tokenizer("value EQU a <= b", false);
    tkz = Tokenizer("value EQU a << b", false);

    tkz = Tokenizer("value EQU a >", false);
    tkz = Tokenizer("value EQU a > b", false);
    tkz = Tokenizer("value EQU a >= b", false);
    tkz = Tokenizer("value EQU a >> b", false);

    tkz = Tokenizer("value EQU !", false);
    tkz = Tokenizer("value EQU ! a", false);
    tkz = Tokenizer("value EQU a != b", false);

    tkz = Tokenizer("value EQU a |", false);
    tkz = Tokenizer("value EQU a | b", false);
    tkz = Tokenizer("value EQU a || b", false);

    tkz = Tokenizer("value EQU a &", false);
    tkz = Tokenizer("value EQU a & b", false);
    tkz = Tokenizer("value EQU a && b", false);

    tkz = Tokenizer("value EQU a % b", false);

    tkz = Tokenizer("DB 'Single quoted', 'a'", false);
    tkz = Tokenizer("DB 'Single quoted '' with apostrophe'", false);

    tkz = Tokenizer("DB \"Quoted \\\\ \\n \\r \\t \\a \\0 \\12 \\777 \"", false);
    tkz = Tokenizer("DB \"Quoted \\x \\x1 \\x12 \"", false);

    tkz = Tokenizer("a EQU $ + 1", false);
    Token tk = tkz.gettoken();
    tk = tkz.gettoken();
    tk = tkz.gettoken();
    is(tk.type(), TypeDollar, "parsed $");

    tkz = Tokenizer("a EQU $a$b + 1", false);
    tk = tkz.gettoken();
    tk = tkz.gettoken();
    tk = tkz.gettoken();
    is(tk.type(), TypeNumber, "parsed $ prefixed hex");

    /*
    throws_runtime("Invalid $ hex throws",
        [] ()
        {
            Tokenizer tkz("a equ $$$ + 1", false);
        } );
    */

    tkz = Tokenizer("&abcd + 1", false);
    tk = tkz.gettoken();
    is(tk.num(), 0xABCD, "Hex prefixed with &");
    tkz = Tokenizer("%101 + 1", false);
    tk = tkz.gettoken();
    is(tk.num(), 0B101 , "Bin prefixed with %");

    tkz = Tokenizer("#", false);
    tkz = Tokenizer("##", false);
    tkz = Tokenizer("#ABCD + 0", false);

    tkz = Tokenizer("ab$cd:", true);
    tk = tkz.gettoken();
    is(tk.str(), "ABCD", "case insensitive identifier with $");

    tkz = Tokenizer("?", true);
    tkz = Tokenizer("?xyz:", true);

    tkz = Tokenizer("(a)", true);
    ok(tkz.endswithparen(), "Line ending with parenthesis");

    tkz = Tokenizer("INCLUDE somefile", true);
    tkz = Tokenizer("INCLUDE somefile ; comment", true);
    tkz = Tokenizer("INCLUDE \"somefile\"", true);
    tkz = Tokenizer("INCLUDE 'somefile'", true);
    tk = tkz.gettoken();
    tkz.getincludefile();
    tkz.reset();
}

int main()
{
    plan(50);

    test_token();
    test_tokenizer();
    test_unget();
    test_include();
    test_chars();
    test_unclosed();
    test_invalid_num();
    test_parse_expr();
}

// End
