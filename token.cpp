// token.cpp
// Revision 6-nov-2004

#include "token.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>

#include <ctype.h>

#include <assert.h>
#define ASSERT assert

//*********************************************************
//		Auxiliary functions and constants.
//*********************************************************

namespace {

std::ostream & hex2 (std::ostream & os)
{
	//os << std::hex << std::uppercase <<
	os.setf (std::ios::uppercase);
	os << std::hex <<
		std::setw (2) << std::setfill ('0');
	return os;
}

std::ostream & hex4 (std::ostream & os)
{
	//os << std::hex << std::uppercase <<
	os.setf (std::ios::uppercase);
	os << std::hex <<
		std::setw (4) << std::setfill ('0');
	return os;
}

std::string upper (const std::string & str)
{
	std::string r;
	const std::string::size_type l= str.size ();
	for (std::string::size_type i= 0; i < l; ++i)
		r+= toupper (str [i] );
	return r;
}

typedef std::map <std::string, TypeToken> tmap_t;

tmap_t tmap;

typedef std::map <TypeToken, std::string> invmap_t;

invmap_t invmap;

struct NameType {
	const char * const str;
	const TypeToken type;
	NameType (const char * str, TypeToken type) :
		str (str), type (type)
	{ }
};

#define NT(n) NameType (#n, Type ## n)

const NameType nt []= {
	// Directives
	NT (DEFB),
	NT (DB),
	NT (DEFM),
	NT (DEFW),
	NT (DW),
	NT (DEFS),
	NT (DS),
	NT (EQU),
	NT (DEFL),
	NT (ORG),
	NT (INCLUDE),
	NT (INCBIN),
	NT (IF),
	NT (ELSE),
	NT (ENDIF),
	NT (PUBLIC),
	NT (END),
	NT (LOCAL),
	NT (PROC),
	NT (ENDP),
	NT (MACRO),
	NT (ENDM),
	NT (EXITM),
	NT (REPT),
	NT (IRP),

	// Operators
	NT (MOD),
	NT (SHL),
	NT (SHR),
	NT (NOT),
	NT (EQ),
	NT (LT),
	NT (LE),
	NT (GT),
	NT (GE),
	NT (NE),
	NT (NUL),
	NT (DEFINED),

	// Nemonics
	NT (ADC),
	NT (ADD),
	NT (AND),
	NT (BIT),
	NT (CALL),
	NT (CCF),
	NT (CP),
	NT (CPD),
	NT (CPDR),
	NT (CPI),
	NT (CPIR),
	NT (CPL),
	NT (DAA),
	NT (DEC),
	NT (DI),
	NT (DJNZ),
	NT (EI),
	NT (EX),
	NT (EXX),
	NT (HALT),
	NT (IM),
	NT (IN),
	NT (INC),
	NT (IND),
	NT (INDR),
	NT (INI),
	NT (INIR),
	NT (JP),
	NT (JR),
	NT (LD),
	NT (LDD),
	NT (LDDR),
	NT (LDI),
	NT (LDIR),
	NT (NEG),
	NT (NOP),
	NT (OR),
	NT (OTDR),
	NT (OTIR),
	NT (OUT),
	NT (OUTD),
	NT (OUTI),
	NT (POP),
	NT (PUSH),
	NT (RES),
	NT (RET),
	NT (RETI),
	NT (RETN),
	NT (RL),
	NT (RLA),
	NT (RLC),
	NT (RLCA),
	NT (RLD),
	NT (RR),
	NT (RRA),
	NT (RRC),
	NT (RRCA),
	NT (RRD),
	NT (RST),
	NT (SBC),
	NT (SCF),
	NT (SET),
	NT (SLA),
	NT (SLL),
	NT (SRA),
	NT (SRL),
	NT (SUB),
	NT (XOR),

	// Registers.
	// C is listed as flag.
	NT (A),
	NT (AF),
	NameType ("AF'", TypeAFp),
	NT (B),
	NT (BC),
	NT (D),
	NT (E),
	NT (DE),
	NT (H),
	NT (L),
	NT (HL),
	NT (SP),
	NT (IX),
	NT (IXH),
	NT (IXL),
	NT (IY),
	NT (IYH),
	NT (IYL),
	NT (I),
	NT (R),

	// Flags
	NT (NZ),
	NT (Z),
	NT (NC),
	NT (C),
	NT (PO),
	NT (PE),
	NT (P),
	NT (M)
};

bool init_map ()
{
	#ifndef NDEBUG
	bool typeused [TypeXOR + 1]= { false };
	#endif

	for (const NameType * p= nt;
		p != nt + (sizeof (nt) / sizeof (nt [0] ) );
		++p)
	{
		tmap [p->str]= p->type;
		invmap [p->type]= p->str;
		#ifndef NDEBUG
		if (p->type <= TypeXOR)
			typeused [p->type]= true;
		#endif
	}

	#ifndef NDEBUG
	bool checkfailed= false;
	for (TypeToken t= TypeADC; t <= TypeXOR; t= TypeToken (t + 1) )
		if (! typeused [t] )
		{
			cerr << "Type " << t << " unasigned" << endl;
			checkfailed= true;
		}
	ASSERT (! checkfailed);
	#endif

	return true;
}

bool map_ready= init_map ();

TypeToken getliteraltoken (const std::string & str)
{
	tmap_t::iterator it= tmap.find (upper (str) );
	if (it != tmap.end () )
		return it->second;
	else
		return TypeUndef;
}

std::string gettokenname (TypeToken t)
{
	invmap_t::iterator it= invmap.find (t);
	if (it != invmap.end () )
		return it->second;
	else
		return std::string (1, static_cast <char> (t) );
}

} // namespace

//*********************************************************
//			class Token
//*********************************************************

Token::Token () :
	t (TypeUndef)
{
}

Token::Token (TypeToken t) :
	t (t)
{
}

Token::Token (address n) :
	t (TypeNumber),
	number (n)
{
}

Token::Token (TypeToken t, const std::string & s) :
	t (t),
	s (s)
{
}

std::string Token::str () const
{
	switch (t)
	{
	case TypeUndef:
		return "(undef)";
	case TypeEndLine:
		return "(eol)";
	case TypeIdentifier:
		return s;
	case TypeLiteral:
		//return std::string (1, '\'') + s + '\'';
		return s;
	case TypeNumber:
		{
			std::ostringstream oss;
			oss << hex4 << number;
			return oss.str ();
		}
	default:
		//return std::string (1, static_cast <char> (t) );
		return gettokenname (t);
	}
}

std::ostream & operator << (std::ostream & oss, const Token & tok)
{
	oss << tok.str ();
	return oss;
}

//*********************************************************
//			class Tokenizer
//*********************************************************

Tokenizer::Tokenizer (bool nocase) :
	//hassaved (false)
	endpassed (0),
	nocase (nocase)
{
}

Tokenizer::Tokenizer (const Tokenizer & tz) :
	tokenlist (tz.tokenlist),
	current (tokenlist.begin () ),
	//hassaved (false)
	endpassed (0),
	nocase (tz.nocase)
{
}

Tokenizer::Tokenizer (const std::string & line, bool nocase) :
	//iss (line),
	//hassaved (false)
	endpassed (0),
	nocase (nocase)
{
	std::istringstream iss (line);

	// Optional line number ignored.
	char c;
	while (iss >> c && isdigit (c) )
		continue;
	if (iss)
		iss.unget ();

	Token tok;
	while ( (tok= parsetoken (iss) ).type () != TypeEndLine)
	{
		tokenlist.push_back (tok);
		if (tok.type () == TypeINCLUDE || tok.type () == TypeINCBIN)
		{
			tokenlist.push_back
				(Token (TypeLiteral,
					parseincludefile (iss) ) );
		}
	}
	current= tokenlist.begin ();
}

Tokenizer::~Tokenizer ()
{
}

std::ostream & operator << (std::ostream & oss, const Tokenizer & tz)
{
	std::copy (tz.tokenlist.begin (), tz.tokenlist.end (),
		std::ostream_iterator <Token> (oss, " ") );
	return oss;
}

void Tokenizer::push_back (const Token & tok)
{
	tokenlist.push_back (tok);
	current= tokenlist.begin ();
}

#if 0
void Tokenizer::putback (const Token & tok)
{
	if (hassaved)
		throw "Internal error in Tokenizer::putback";
	saved= tok;
	hassaved= true;
}
#endif

Token Tokenizer::gettoken ()
{
	#if 0
	if (hassaved)
	{
		hassaved= false;
		return saved;
	}
	#endif
	if (current == tokenlist.end () )
	{
		++endpassed;
		return Token (TypeEndLine);
	}
	else
	{
		Token tok= * current;
		++current;
		return tok;
	}
}

void Tokenizer::ungettoken ()
{
	if (endpassed > 0)
	{
		ASSERT (current == tokenlist.end () );
		--endpassed;
	}
	else
	{
		if (current == tokenlist.begin () )
			throw "Internal error: Tokenizer underflowed";
		--current;
	}
}

std::string Tokenizer::getincludefile ()
{
	Token tok= gettoken ();
	if (tok.type () != TypeLiteral)
		throw "Internal unexpected error";
	return tok.str ();
}

namespace {

const char unclosed []= "Unclosed literal";

Token parsestringc (std::istringstream & iss)
{
	std::string str;
	for (;;)
	{
		char c= iss.get ();
		if (! iss)
			throw unclosed;
		if (c == '"')
			return Token (TypeLiteral, str);
		if (c == '\\')
		{
			c= iss.get ();
			if (! iss)
				throw unclosed;
			switch (c)
			{
			case '\\':
				break;
			case 'n':
				c= '\n'; break;
			case 'r':
				c= '\r'; break;
			case 't':
				c= '\t'; break;
			case 'a':
				c= '\a'; break;
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				{
					c-= '0';
					char c2= iss.get ();
					if (! iss)
						throw unclosed;
					if (c2 < '0' || c2 > '7')
						iss.unget ();
					else
					{
						c*= 8;
						c+= c2 - '0';
						c2= iss.get ();
						if (! iss)
							throw unclosed;
						if (c2 < '0' || c2 > '7')
							iss.unget ();
						else
						{
							c*= 8;
							c+= c2 - '0';
						}
					}
				}
				break;
			case 'x': case 'X':
				{
					char x [3]= { 0 };
					c= iss.get ();
					if (! iss)
						throw unclosed;
					if (! isxdigit (c) )
						iss.unget ();
					else
					{
						x [0]= c;
						c= iss.get ();
						if (! iss)
							throw unclosed;
						if (! isxdigit (c) )
							iss.unget ();
						else
							x [1]= c;
					}
					c= strtoul (x, NULL, 16);
				}
				break;
			}
			str+= c;
		}
		else
			str+= c;
	}
}

} // namespace

Token Tokenizer::parsetoken (std::istringstream & iss)
{
	char c;
	if (! (iss >> c) )
		return Token (TypeEndLine);
	switch (c)
	{
	case ';':
		return Token (TypeEndLine);
	case ',':
		return Token (TypeComma);
	case ':':
		return Token (TypeColon);
	case '+':
		return Token (TypePlus);
	case '-':
		return Token (TypeMinus);
	case '*':
		return Token (TypeMult);
	case '/':
		return Token (TypeDiv);
	case '=':
		return Token (TypeEqual);
	case '(':
		return Token (TypeOpen);
	case ')':
		return Token (TypeClose);
	case '$':
		iss >> c;
		if (iss && isxdigit (c) )
		{
			std::string str (1, c);
			while (iss >> c && isxdigit (c) )
				str+= c;
			if (str.empty () )
				throw "Invalid empty hex number";
			if (iss)
				iss.unget ();
			unsigned long n;
			char * aux;
			n= strtoul (str.c_str (), & aux, 16);
			if (* aux != '\0')
				throw "Invalid numeric format";
			if (n > 0xFFFFUL)
				throw "Number out of range";
			return Token (static_cast <address> (n) );
		}
		else
		{
			if (iss)
				iss.unget ();
			return Token (TypeDollar);
		}
		// break not needed here, only exits the if
		// with return or throw.
	case '\'':
		{
			std::string str;
			for (;;)
			{
				char c= iss.get ();
				if (! iss)
					throw unclosed;
				if (c == '\'')
				{
					c= iss.get ();
					if (! iss)
						return Token
							(TypeLiteral, str);
					if (c != '\'')
					{
						iss.unget ();
						return Token
							(TypeLiteral, str);
					}
				}
				str+= c;
			}
		}
		// break not needed here, only exits the block
		// with return or throw.
	case '"':
		return parsestringc (iss);
	case '#':
		// Hexadecimal number.
		{
			std::string str;
			while (iss >> c && isxdigit (c) )
				str+= c;
			if (str.empty () )
				throw "Invalid empty hex number";
			if (iss)
				iss.unget ();
			unsigned long n;
			char * aux;
			n= strtoul (str.c_str (), & aux, 16);
			if (* aux != '\0')
				throw "Invalid numeric format";
			if (n > 0xFFFFUL)
				throw "Number out of range";
			return Token (static_cast <address> (n) );
		}
		// break not needed here, only exits the block
		// with return or throw.
	case '&':
		// Hexadecimal, octal or binary number.
		iss >> c;
		if (! iss)
			throw "Empty hexadecimal number";
		{
			std::string str;
			int base= 16;
			switch (c)
			{
			case 'h': case 'H':
				break;
			case 'o': case 'O':
				base= 8;
				break;
			case 'x': case 'X':
				base= 2;
				break;
			default:
				if (isxdigit (c) )
					str= c;
				else
					throw "Syntax error";
			}
			while (iss >> c && isxdigit (c) )
				str+= c;
			if (str.empty () )
				throw "Syntax error";
			if (iss)
				iss.unget ();
			char * aux;
			unsigned long n= strtoul (str.c_str (), & aux, base);
			if (* aux != '\0')
				throw "Invalid numeric format";
			if (n > 0xFFFFUL)
				throw "Number out of range";
			return Token (static_cast <address> (n) );
		}
		// break not needed here, only exits the block
		// with return or throw.
	case '%':
		iss >> c;
		if (! iss || (c != '0' && c != '1') )
		{
			// Mod operator.
			if (iss)
				iss.unget ();
			return Token (TypeMod);
		}
		// Binary number.
		{
			std::string str;
			//while (iss >> c && (c == '0' || c == '1') )
			//	str+= c;
			do {
				str+= c;
			} while (iss >> c && (c == '0' || c == '1') );

			// Now this can't occur.
			//if (str.empty () )
			//	throw "Invalid empty binary number";

			if (iss)
				iss.unget ();
			unsigned long n;
			char * aux;
			n= strtoul (str.c_str (), & aux, 2);
			if (* aux != '\0')
				throw "Invalid numeric format";
			if (n > 0xFFFFUL)
				throw "Number out of range";
			return Token (static_cast <address> (n) );
		}
		// break not needed here, only exits the block
		// with return or throw.
	default:
		; // Nothing
	}
	if (isdigit (c) )
	{
		std::string str;
		do {
			if (c != '$')
				str+= c;
			c= iss.get ();
			if (! iss)
				c= 0;
		} while (isalnum (c) || c == '$');
		if (iss)
			iss.unget ();
		const std::string::size_type l= str.size ();
		unsigned long n;
		char * aux;

		// Hexadecimal with 0x prefix.
		if (str [0] == '0' && l > 1 &&
			(str [1] == 'x' || str [1] == 'X') )
		{
			str.erase (0, 2);
			if (str.empty () )
				throw "Invalid hex number";
			n= strtoul (str.c_str (), & aux, 16);
		}
		else
		{
			// Decimal, hexadecimal, octal and binary
			// with or without suffix.
			switch (toupper (str [l - 1]) )
			{
			case 'H': // Hexadecimal.
				str.erase (l - 1);
				n= strtoul (str.c_str (), & aux, 16);
				break;
			case 'O': // Octal.
			case 'Q': // Octal.
				str.erase (l - 1);
				n= strtoul (str.c_str (), & aux, 8);
				break;
			case 'B': // Binary.
				str.erase (l - 1);
				n= strtoul (str.c_str (), & aux, 2);
				break;
			case 'D': // Decimal
				str.erase (l - 1);
				n= strtoul (str.c_str (), & aux, 10);
				break;
			default: // Decimal
				n= strtoul (str.c_str (), & aux, 10);
			}
		}
		if (* aux != '\0')
			throw "Invalid numeric format";

		// Testing: do not forbid numbers out of 16 bits range,
		// just truncate it.
		//if (n > 0xFFFFUL)
		//	throw "Number out of range";

		return Token (static_cast <address> (n) );
	}
	if (isalpha (c) || c == '_' || c == '?' || c == '@' || c == '.')
	{
		std::string str;
		do
		{
			if (c != '$')
				str+= c;
			c= iss.get ();
			if (! iss)
				c= 0;
		} while (isalnum (c) || c == '_' || c == '$' || c == '?' ||
			c == '@' || c == '.');
		if (iss)
			iss.unget ();
		TypeToken t= getliteraltoken (str);
		if (t == TypeUndef)
		{
			if (nocase)
				str= upper (str);
			return Token (TypeIdentifier, str);
		}
		else
		{
			if (t == TypeAF && c == '\'')
			 {
			 	iss.get ();
				t= TypeAFp;
			 }
			return Token (t);
		}
	}
	//return Token ();
	std::ostringstream oss;
	oss << "Unexpected character: ";
	if (c < 32 || c >= 127)
		oss << '&' << hex2 << int (static_cast <unsigned char> (c) );
	else
		oss << '\'' << c << '\'';
	throw oss.str ();
}

std::string Tokenizer::parseincludefile (std::istringstream & iss)
{
	char c;
	while (iss >> c && isspace (c) )
		continue;
	if (! iss)
		throw "Filename required";
	std::string r;
	switch (c)
	{
	case '"':
		do
		{
			iss >> c;
			if (! iss)
				throw "Invalid file name";
			if (c != '"')
				r+= c;
		} while (c != '"');
		break;
	case '\'':
		do
		{
			iss >> c;
			if (! iss)
				throw "Invalid file name";
			if (c != '\'')
				r+= c;
		} while (c != '\'');
		break;
	default:
		r+= c;
		while (iss >> c && ! isspace (c) )
			r+= c;
		if (iss)
			iss.unget ();
	}
	return r;
}

// End of token.cpp
