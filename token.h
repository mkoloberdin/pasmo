// token.h
// Revision 6-apr-2004

#include <string>
#include <sstream>
#include <deque>

#include <limits.h>

#if USHRT_MAX == 65535

typedef unsigned short address;

#else

// Using the C99 types, hoping they will be available.

#include <stdint.h>

typedef uint16_t address:

#endif

typedef unsigned char byte;

enum TypeToken {
	TypeUndef= 0,

	TypeEndLine= 1,
	TypeComma= ',',
	TypeColon= ':',
	TypePlus= '+',
	TypeMinus= '-',
	TypeMult= '*',
	TypeDiv= '/',
	TypeEqual= '=',
	TypeOpen= '(',
	TypeClose= ')',
	TypeDollar= '$',
	TypeMod= '%',

	// Valores
	TypeIdentifier= 0x100,
	TypeLiteral,
	TypeNumber,

	// Operadores.
	TypeMOD,
	TypeSHL,
	TypeSHR,
	TypeNOT,
	// AND, OR y XOR son nemónicos.
	TypeEQ,
	TypeLT,
	TypeLE,
	TypeGT,
	TypeGE,
	TypeNE,
	TypeNUL,

	// Directivas
	TypeDEFB,
	TypeDB,
	TypeDEFM,
	TypeDEFW,
	TypeDW,
	TypeDEFS,
	TypeDS,
	TypeEQU,
	TypeDEFL,
	TypeORG,
	TypeINCLUDE,
	TypeINCBIN,
	TypeIF,
	TypeELSE,
	TypeENDIF,
	TypePUBLIC,
	TypeEND,
	TypeLOCAL,
	TypePROC,
	TypeENDP,
	TypeMACRO,
	TypeENDM,
	TypeEXITM,
	TypeREPT,
	TypeIRP,

	// Nemonicos
	TypeADC,
	TypeADD,
	TypeAND,
	TypeBIT,
	TypeCALL,
	TypeCCF,
	TypeCP,
	TypeCPD,
	TypeCPDR,
	TypeCPI,
	TypeCPIR,
	TypeCPL,
	TypeDAA,
	TypeDEC,
	TypeDI,
	TypeDJNZ,
	TypeEI,
	TypeEX,
	TypeEXX,
	TypeHALT,
	TypeIM,
	TypeIN,
	TypeINC,
	TypeIND,
	TypeINDR,
	TypeINI,
	TypeINIR,
	TypeJP,
	TypeJR,
	TypeLD,
	TypeLDD,
	TypeLDDR,
	TypeLDI,
	TypeLDIR,
	TypeNEG,
	TypeNOP,
	TypeOR,
	TypeOTDR,
	TypeOTIR,
	TypeOUT,
	TypeOUTD,
	TypeOUTI,
	TypePOP,
	TypePUSH,
	TypeRES,
	TypeRET,
	TypeRETI,
	TypeRETN,
	TypeRL,
	TypeRLA,
	TypeRLC,
	TypeRLCA,
	TypeRLD,
	TypeRR,
	TypeRRA,
	TypeRRC,
	TypeRRCA,
	TypeRRD,
	TypeRST,
	TypeSBC,
	TypeSCF,
	TypeSET,
	TypeSLA,
	TypeSRA,
	TypeSLL,
	TypeSRL,
	TypeSUB,
	TypeXOR,
	
	// Registers
	// C is listed as flag.
	TypeA,
	TypeAF,
	TypeAFp, // AF'
	TypeB,
	TypeBC,
	TypeD,
	TypeE,
	TypeDE,
	TypeH,
	TypeL,
	TypeHL,
	TypeSP,
	TypeIX,
	TypeIXH,
	TypeIXL,
	TypeIY,
	TypeIYH,
	TypeIYL,
	TypeI,
	TypeR,

	// Flags
	TypeNZ,
	TypeZ,
	TypeNC,
	TypeC,
	TypePO,
	TypePE,
	TypeP,
	TypeM,
};

class Token {
public:
	Token ();
	Token (TypeToken t);
	Token (address n);
	Token (TypeToken t, const std::string & s);
	TypeToken type () const { return t; }
	std::string str () const;
	address num () const { return number; }
private:
	TypeToken t;
	std::string s;
	address number;
};

std::ostream & operator << (std::ostream & oss, const Token & tok);

class Tokenizer {
public:
	Tokenizer (bool nocase);
	Tokenizer (const Tokenizer & tz);
	Tokenizer (const std::string & line, bool nocase);
	~Tokenizer ();
	bool getnocase () const { return nocase; }
	void push_back (const Token & tok);
	//void putback (const Token & tok);
	Token gettoken ();
	void ungettoken ();
	std::string getincludefile ();
	friend std::ostream & operator << (std::ostream & oss,
		const Tokenizer & tz);
private:
	//std::istringstream iss;
	void operator = (const Tokenizer &); // Forbidden.

	typedef std::deque <Token> tokenlist_t;
	tokenlist_t tokenlist;
	tokenlist_t::iterator current;

	//bool hassaved;
	//Token saved;
	size_t endpassed;
	bool nocase;

	Token parsetoken (std::istringstream & iss);
	std::string parseincludefile (std::istringstream & iss);
};

// End of token.h
