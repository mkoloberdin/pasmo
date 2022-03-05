// asmerror.cxx

#include "asmerror.h"

AsmError::AsmError(size_t linepos, const std::string & message) :
    nline(linepos),
    msg(message)
{
}

size_t AsmError::getline() const
{
    return nline;
}

std::string AsmError::message() const
{
    return msg;
}

//--------------------------------------------------------------

ErrorDirective::ErrorDirective(size_t linepos, const Token & tok) :
    AsmError(linepos, ".ERROR directive: " + tok.str() )
{ }

NoInstruction::NoInstruction(size_t linepos, const Token & tok) :
    AsmError(linepos, "Unexpected '" + tok.str() + "' used as instruction")
{ }

TokenExpected::TokenExpected(TypeToken ttexpect, const Token & tokfound,
        size_t linepos) :
    AsmError(linepos, "Expected '" + gettokenname(ttexpect) +
        "' but '" + tokfound.str() + "' found")
{ }

IdentifierExpected::IdentifierExpected(size_t linepos, const Token & tok) :
    AsmError(linepos, "Identifier expected but '" +
        tok.str() + "' found")
{ }

//--------------------------------------------------------------

AsmError DivisionByZero(size_t linepos)
{
    return AsmError(linepos, "Division by zero");
}

AsmError EQUwithoutlabel(size_t linepos)
{
    return AsmError(linepos, "EQU without label");
}

AsmError DEFLwithoutlabel(size_t linepos)
{
    return AsmError(linepos, "DEFL without label");
}

AsmError Length1Required(size_t linepos)
{
    return AsmError(linepos, "Invalid literal, length 1 required");
}

AsmError UndefinedVar(size_t linepos, const std::string & varname)
{
    return AsmError(linepos, "Symbol '" + varname + "' is undefined");
}

AsmError EndLineExpected(size_t linepos, const Token & tok)
{
    return AsmError(linepos, "End line expected but '" +
        tok.str() + "' found");
}

AsmError ValueExpected(size_t linepos, const Token & tok)
{
    return AsmError(linepos, "Value expected but '" +
            tok.str() + "' found");
}

AsmError MacroExpected(size_t linepos, const std::string & name)
{
    return AsmError(linepos, "Macro name expected but '" +
            name + "' found");
}

AsmError InvalidInstruction(size_t linepos)
{
    return AsmError(linepos, "Invalid instruction");
}

AsmError InvalidOperand(size_t linepos)
{
    return AsmError(linepos, "Invalid operand");
}

AsmError BitOutOfRange(size_t linepos)
{
    return AsmError(linepos, "Bit position out of range");
}

AsmError OffsetExpected(size_t linepos, const Token & tok)
{
    return AsmError(linepos, "Offset expression expected but '" +
            tok.str() + "' found");
}

AsmError OffsetOutOfRange(size_t linepos)
{
    return AsmError(linepos, "Offset out of range");
}

AsmError IFwithoutENDIF(size_t linepos)
{
    return AsmError(linepos, "IF without ENDIF");
}

AsmError ENDIFwithoutIF(size_t linepos)
{
    return AsmError(linepos, "ENDIF without IF");
}

AsmError ELSEwithoutIF(size_t linepos)
{
    return AsmError(linepos, "ELSE without IF");
}

AsmError ELSEwithoutENDIF(size_t linepos)
{
    return AsmError(linepos, "ELSE without ENDIF");
}

AsmError UnbalancedPROC(size_t linepos)
{
    return AsmError(linepos, "Unbalanced PROC");
}

AsmError UnbalancedENDP(size_t linepos)
{
    return AsmError(linepos, "Unbalanced ENDP");
}

AsmError MACROwithoutENDM(size_t linepos)
{
    return AsmError(linepos, "MACRO without ENDM");
}

AsmError ENDMOutOfMacro(size_t linepos)
{
    return AsmError(linepos, "ENDM outside of macro");
}

AsmError InvalidInAutolocal(size_t linepos)
{
    return AsmError(linepos, "Invalid use of name in autolocal mode");
}

AsmError RelativeOutOfRange(size_t linepos)
{
    return AsmError(linepos, "Relative jump out of range");
}

AsmError IsPredefined(size_t linepos)
{
    return AsmError(linepos, "Can't redefine, is predefined");
}

//--------------------------------------------------------------
// End
