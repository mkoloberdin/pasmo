// asmerror.h

#include <string>

#include "token.h"

struct ErrorAlreadyReported
{
};

class AsmError
{
    const size_t nline;
    const std::string msg;
public:
    AsmError(size_t linepos, const std::string & message);
    size_t getline() const;
    std::string message() const;
};

class ErrorDirective : public AsmError
{
public:
    ErrorDirective(size_t linepos, const Token & tok);
};

class NoInstruction : public AsmError
{
public:
    NoInstruction(size_t linepos, const Token & tok);
};

class TokenExpected : public AsmError
{
public:
    TokenExpected(TypeToken ttexpect, const Token & tokfound,
        size_t linepos);
};

class IdentifierExpected : public AsmError
{
public:
    IdentifierExpected(size_t linepos, const Token & tok);
};

AsmError DivisionByZero(size_t linepos);
AsmError EQUwithoutlabel(size_t linepos);
AsmError DEFLwithoutlabel(size_t linepos);
AsmError Length1Required(size_t linepos);
AsmError UndefinedVar(size_t linepos, const std::string & varname);
AsmError EndLineExpected(size_t linepos, const Token & tok);
AsmError ValueExpected(size_t linepos, const Token & tok);
AsmError MacroExpected(size_t linepos, const std::string & name);

AsmError InvalidInstruction(size_t linepos);
AsmError InvalidOperand(size_t linepos);

AsmError BitOutOfRange(size_t linepos);
AsmError OffsetExpected(size_t linepos, const Token & tok);
AsmError OffsetOutOfRange(size_t linepos);
AsmError IFwithoutENDIF(size_t linepos);
AsmError ENDIFwithoutIF(size_t linepos);
AsmError ELSEwithoutIF(size_t linepos);
AsmError ELSEwithoutENDIF(size_t linepos);
AsmError UnbalancedPROC(size_t linepos);
AsmError UnbalancedENDP(size_t linepos);
AsmError MACROwithoutENDM(size_t linepos);
AsmError ENDMOutOfMacro(size_t linepos);
AsmError InvalidInAutolocal(size_t linepos);
AsmError RelativeOutOfRange(size_t linepos);
AsmError IsPredefined(size_t linepos);

// End
