// asm.cpp
// Revision 27-oct-2004

#include "asm.h"
#include "token.h"

// ostream not included with gcc 2.95
//#include <ostream>
#include <iostream>
using std::cerr;
using std::endl;

#include <fstream>
#include <iomanip>
#include <vector>
#include <map>
#include <stack>
#include <memory>
#include <iterator>
#include <stdexcept>

#include <ctype.h>

#include <assert.h>
#define ASSERT assert

using std::logic_error;

//*********************************************************
//		Auxiliary functions and constants.
//*********************************************************

namespace {

const address addrTRUE= 0xFFFF;
const address addrFALSE= 0;

const byte prefixIX= 0xDD;
const byte prefixIY= 0xFD;

const char AutoLocalPrefix= '_';

#if 0

inline void sethex (std::ostream & os, int n)
{
	// uppercase manipulator not available with gcc 2.95
	os.setf (std::ios::uppercase);
	os << std::hex << std::setw (n) << std::setfill ('0');
}

std::ostream & hex2 (std::ostream & os)
{
	sethex (os, 2);
	return os;
}

std::ostream & hex4 (std::ostream & os)
{
	sethex (os, 4);
	return os;
}

std::ostream & hex8 (std::ostream & os)
{
	sethex (os, 8);
	return os;
}

#else

class Hex2 {
public:
	Hex2 (byte b) : b (b)
	{ }
	byte getb () const
	{ return b; }
private:
	byte b;
};

class Hex4 {
public:
	Hex4 (address n) : n (n)
	{ }
	address getn () const
	{ return n; }
private:
	address n;
};

class Hex8 {
public:
	Hex8 (size_t nn) : nn (nn)
	{ }
	size_t getnn () const
	{ return nn; }
private:
	size_t nn;
};

inline Hex2 hex2 (byte b) { return Hex2 (b); }
inline Hex4 hex4 (address n) { return Hex4 (n); }
inline Hex8 hex8 (size_t nn) { return Hex8 (nn); }

const char hexdigit []= { '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

inline void showhex2 (std::ostream & os, byte b)
{
	os << hexdigit [(b >> 4) & 0x0F] << hexdigit [b & 0x0F];
}

inline void showhex4 (std::ostream & os, address n)
{
	showhex2 (os, n >> 8);
	showhex2 (os, n & 0xFF);
}

std::ostream & operator << (std::ostream & os, const Hex2 & h2)
{
	showhex2 (os, h2.getb () );
	return os;
}

std::ostream & operator << (std::ostream & os, const Hex4 & h4)
{
	showhex4 (os, h4.getn () );
	return os;
}

std::ostream & operator << (std::ostream & os, const Hex8 & h8)
{
	size_t nn= h8.getnn ();
	showhex4 (os, nn >> 16);
	showhex4 (os, nn & 0xFFF);
	return os;
}

#endif

std::string localname (size_t n)
{
	std::ostringstream oss;
	oss << hex8 (n);
	return oss.str ();
}

template <class T>
std::string to_string (const T & t)
{
	std::ostringstream oss;
	oss << t;
	return oss.str ();
}

} // namespace

//*********************************************************
//		Null ostream class.
//*********************************************************

namespace {

class Nullbuff : public std::streambuf
{
public:
	Nullbuff ()
	{
		setbuf (0, 0);
	}
protected:
	int overflow (int)
	{
		setp (pbase (), epptr () );
		return 0;
	}
	int sync ()
	{
		return 0;
	}
};

class Nullostream : public std::ostream
{
public:
	Nullostream () :
		std::ostream (& buff)
	{ }
private:
	Nullbuff buff;
};

} // namespace

//***********************************************
//		Auxiliary tables
//***********************************************

namespace {

typedef std::map <TypeToken, byte> simple1byte_t;
simple1byte_t simple1byte;
simple1byte_t simpleED;

bool init_maps ()
{
	simple1byte [TypeCCF]=  0x3F;
	simple1byte [TypeCPL]=  0x2F;
	simple1byte [TypeDAA]=  0x27;
	simple1byte [TypeDI]=   0xF3;
	simple1byte [TypeEI]=   0xFB;
	simple1byte [TypeEXX]=  0xD9;
	simple1byte [TypeHALT]= 0x76;
	simple1byte [TypeNOP]=  0x00;
	simple1byte [TypeSCF]=  0x37;
	simple1byte [TypeRLA]=  0x17;
	simple1byte [TypeRLCA]= 0x07;
	simple1byte [TypeRRA]=  0x1F;
	simple1byte [TypeRRCA]= 0x0F;

	simpleED [TypeCPD]=     0xA9;
	simpleED [TypeCPDR]=    0xB9;
	simpleED [TypeCPI]=     0xA1;
	simpleED [TypeCPIR]=    0xB1;
	simpleED [TypeIND]=     0xAA;
	simpleED [TypeINDR]=    0xBA;
	simpleED [TypeINI]=     0xA2;
	simpleED [TypeINIR]=    0xB2;
	simpleED [TypeLDD]=     0xA8;
	simpleED [TypeLDDR]=    0xB8;
	simpleED [TypeLDI]=     0xA0;
	simpleED [TypeLDIR]=    0xB0;
	simpleED [TypeNEG]=     0x44;
	simpleED [TypeOTDR]=    0xBB;
	simpleED [TypeOTIR]=    0xB3;
	simpleED [TypeOUTD]=    0xAB;
	simpleED [TypeOUTI]=    0xA3;
	simpleED [TypeRETI]=    0x4D;
	simpleED [TypeRETN]=    0x45;
	simpleED [TypeRLD]=     0x6F;
	simpleED [TypeRRD]=     0x67;
	return true;
}

bool map_ready= init_maps ();

} // namespace

//*********************************************************
//			Macro class
//*********************************************************

namespace {

class MacroBase {
public:
	MacroBase (std::vector <std::string> & param) :
		param (param)
	{ }
	MacroBase (const std::string & sparam) :
		param (1)
	{
		param [0]= sparam;
	}
	size_t getparam (const std::string & name) const;
	std::string getparam (size_t n) const;
	static const size_t noparam= size_t (-1);
private:
	std::vector <std::string> param;
};

size_t MacroBase::getparam (const std::string & name) const
{
	for (size_t i= 0; i < param.size (); ++i)
		if (name == param [i])
			return i;
	return noparam;
}

std::string MacroBase::getparam (size_t n) const
{
	if (n >= param.size () )
		return "(none)";
	return param [n];
}

class Macro : public MacroBase {
public:
	Macro (size_t line, std::vector <std::string> & param) :
		MacroBase (param),
		line (line)
	{ }
	size_t getline () const { return line; }
private:
	const size_t line;
};

class MacroIrp : public MacroBase {
public:
	MacroIrp (const std::string & sparam) :
		MacroBase (sparam)
	{ }
};

} // namespace

//*********************************************************
//		Local classes declarations.
//*********************************************************

namespace {

typedef std::map <std::string, address> mapvar_t;

class LocalLevel {
public:
	LocalLevel (Asm::In & asmin);
	virtual ~LocalLevel ();
	virtual bool is_auto () const;
	void add (const std::string & var);
private:
	Asm::In & asmin;
	mapvar_t saved;
	std::map <std::string, std::string> globalized;
};

class AutoLevel : public LocalLevel {
public:
	AutoLevel (Asm::In & asmin);
	bool is_auto () const;
};

class ProcLevel : public LocalLevel {
public:
	ProcLevel (Asm::In & asmin, size_t line);
	size_t getline () const;
private:
	size_t line;
};

class MacroLevel : public LocalLevel {
public:
	MacroLevel (Asm::In & asmin);
};

class LocalStack {
public:
	~LocalStack ();
	bool empty () const;
	void push (LocalLevel * level);
	LocalLevel * top ();
	void pop ();
private:
	typedef std::stack <LocalLevel *> st_t;
	st_t st;
};

} // namespace

//*********************************************************
//		class Asm::In declaration
//*********************************************************

class Asm::In {
public:
	In ();

	// This is not a copy constructor, it creates a new
	// instance copying only the options.
	explicit In (const Asm::In & in);

	~In ();

	void setdebugtype (DebugType type);
	void errtostdout ();
	void setbase (unsigned int addr);
	void caseinsensitive ();
	void autolocal ();
	void addincludedir (const std::string & dirname);

	void processfile (const std::string & filename);
	void emitobject (std::ostream & out);
	void emitplus3dos (std::ostream & out);
	void emittap (std::ostream & out, const std::string & filename);
	void emittapbas (std::ostream & out, const std::string & filename);
	void emithex (std::ostream & out);
	void emitamsdos (std::ostream & out, const std::string & filename);
	void emitprl (std::ostream & out, Asm & asmoff);
	void emitmsx (std::ostream & out);
	void dumppublic (std::ostream & out);
	void dumpsymbol (std::ostream & out);
private:
	void operator = (const Asm::In &); // Forbidden.

	void openis (std::ifstream & is, const std::string & filename,
		std::ios::openmode mode);
	void setentrypoint (address addr);
	void checkendline (Tokenizer & tz);
	void gencode (byte code);
	void gencodeword (address value);
	void setvalue (const std::string & var, address value);
	address getvalue (const std::string & var, bool required);
	void parsevalue (bool required, Tokenizer & tz, address & result);
	void parseopen (bool required, Tokenizer & tz, address & result);
	void parsemuldiv (bool required, Tokenizer & tz, address & result);
	void parseplusmin (bool required, Tokenizer & tz, address & result);
	void parserelops (bool required, Tokenizer & tz, address & result);
	void parsenot (bool required, Tokenizer & tz, address & result);
	void parseand (bool required, Tokenizer & tz, address & result);
	void parseorxor (bool required, Tokenizer & tz, address & result);
	address parseexpr (bool required, const Token & tok, Tokenizer & tz);
	void parsecomma (Tokenizer & tz);
	void parseopen (Tokenizer & tz);
	void parseclose (Tokenizer & tz);
	void parseA (Tokenizer & tz);
	void parseC (Tokenizer & tz);
	void loadfile (const std::string & filename);
	void getvalidline ();
	void parseif (Tokenizer & tz);
	void parseelse ();
	void parseline (Tokenizer & tz);
	void dopass ();
	void parsegeneric (Tokenizer & tz, Token tok);
	void parseorg (Tokenizer & tz);
	void setequorlabel (const std::string & name, address value);
	void parselabel (Tokenizer & tz, const std::string & name);
	void parsemacro (Tokenizer & tz, const std::string & name);
	void parsedesp (Tokenizer & tz, byte & desp);
	bool parsebyteparam (Tokenizer & tz, TypeToken tt,
		unsigned short & regcode,
		byte & prefix, bool & hasdesp, byte & desp,
		byte prevprefix= 0);
	void dobyteparam (Tokenizer & tz, byte codereg, byte codein);
	void dobyteparamCB (Tokenizer & tz, byte codereg);
	void parseIM (Tokenizer & tz);
	void parseRST (Tokenizer & tz);
	void parseLDA (Tokenizer & tz);
	void parseLDsimple (Tokenizer & tz, unsigned short regcode,
		byte prevprefix= 0);
	void parseLDdouble (Tokenizer & tz,
		unsigned short regcode, byte prefix);
	void parseLDSP (Tokenizer & tz);
	void parseLD (Tokenizer & tz);
	void parseCP (Tokenizer & tz);
	void parseAND (Tokenizer & tz);
	void parseOR (Tokenizer & tz);
	void parseXOR (Tokenizer & tz);
	void parseRL (Tokenizer & tz);
	void parseRLC (Tokenizer & tz);
	void parseRR (Tokenizer & tz);
	void parseRRC (Tokenizer & tz);
	void parseSLA (Tokenizer & tz);
	void parseSRA (Tokenizer & tz);
	void parseSLL (Tokenizer & tz);
	void parseSRL (Tokenizer & tz);
	void parseSUB (Tokenizer & tz);
	void parseADDADCSBCHL (Tokenizer & tz, byte prefix, byte basecode);
	void parseADD (Tokenizer & tz);
	void parseADC (Tokenizer & tz);
	void parseSBC (Tokenizer & tz);
	void parsePUSHPOP (Tokenizer & tz, bool isPUSH);
	void parseCALL (Tokenizer & tz);
	void parseRET (Tokenizer & tz);
	void parseJP (Tokenizer & tz);
	void parseJR (Tokenizer & tz);
	void parseDJNZ (Tokenizer & tz);
	void parseINCDEC (Tokenizer & tz, bool isINC);
	void parseEX (Tokenizer & tz);
	void parseIN (Tokenizer & tz);
	void parseOUT (Tokenizer & tz);
	void dobit (Tokenizer & tz, byte basecode);
	void parseBIT (Tokenizer & tz);
	void parseRES (Tokenizer & tz);
	void parseSET (Tokenizer & tz);
	void parseDEFB (Tokenizer & tz);
	void parseDEFW (Tokenizer & tz);
	void parseDEFS (Tokenizer & tz);
	void parseINCBIN (Tokenizer & tz);
	void parsePUBLIC (Tokenizer & tz);
	void parseEND (Tokenizer & tz);
	void parseLOCAL (Tokenizer & tz);
	void parsePROC (Tokenizer & tz);
	void parseENDP (Tokenizer & tz);

	bool nocase;
	bool autolocalmode;
	DebugType debugtype;
	byte mem [65536];
	address base;
	address current;
	address currentinstruction;
	address minused;
	address maxused;
	address entrypoint;
	bool hasentrypoint;
	size_t currentline;

	// ********** Variables ************

	mapvar_t mapvar;
	typedef std::set <std::string> setpublic_t;
	setpublic_t setpublic;
	enum Defined { NoDefined, DefinedDEFL, DefinedPass1, DefinedPass2 };
	typedef std::map <std::string, Defined> vardefined_t;
	vardefined_t vardefined;

	int pass;

	// ********* Lines and info for it **********

	typedef std::vector <std::string> vline_t;
	vline_t vline;
	struct FileRef {
		std::string filename;
		FileRef (const std::string & name) :
			filename (name)
		{ }
	};
	std::vector <FileRef> vfileref;
	struct LineInfo {
		size_t linenum;
		size_t filenum;
		LineInfo (size_t line, size_t file) :
			linenum (line), filenum (file)
		{ }
	};
	std::vector <LineInfo> vlineinfo;

	size_t iflevel;
	std::ostream * pout;
	std::ostream * perr;

	// ********* Local **********

	//class LocalLevel;
	friend class LocalLevel;

	std::vector <address> localvalue;
	size_t localcount;

	void initlocal () { localcount= 0; }

	#if 0

	class LocalLevel {
	public:
		LocalLevel (Asm::In & asmin) :
			asmin (asmin)
		{ }
		virtual ~LocalLevel ()
		{
			for (mapvar_t::iterator it= saved.begin ();
				it != saved.end ();
				++it)
			{
				const std::string & name=
					globalized [it->first];
				asmin.mapvar [name]=
					asmin.mapvar [it->first];
				asmin.mapvar [it->first]= it->second;
				std::swap (asmin.vardefined [it->first],
					asmin.vardefined [name] );
			}
		}
		void add (const std::string & var)
		{
			// Ignore redeclarations as LOCAL
			// of the same identifier.
			if (saved.find (var) != saved.end () )
				return;

			saved [var]= asmin.mapvar [var];
			const std::string name= localname (asmin.localcount);
			globalized [var]= name;
			std::swap (asmin.vardefined [var],
				asmin.vardefined [name] );
			++asmin.localcount;
			if (asmin.pass == 1)
			{
				asmin.localvalue.push_back (0);
				ASSERT (asmin.localcount ==
					asmin.localvalue.size () );
			}
			else
			{
				ASSERT (asmin.localcount <=
					asmin.localvalue.size () );
				asmin.mapvar [var]=
					asmin.mapvar [name];
			}
		}
	private:
		Asm::In & asmin;
		mapvar_t saved;
		std::map <std::string, std::string> globalized;
	};
	class ProcLevel : public LocalLevel {
	public:
		ProcLevel (Asm::In & asmin, size_t line) :
			LocalLevel (asmin),
			line (line)
		{ }
		size_t getline () const { return line; }
	private:
		size_t line;
	};
	class MacroLevel : public LocalLevel {
	public:
		MacroLevel (Asm::In & asmin) :
			LocalLevel (asmin)
		{ }
	};
	class LocalStack {
	public:
		~LocalStack ()
		{
			while (! st.empty () )
				pop ();
		}
		bool empty () const
		{
			return st.empty ();
		}
		void push (LocalLevel * level)
		{
			st.push (level);
		}
		LocalLevel * top ()
		{
			if (st.empty () )
				throw "Not in LOCAL valid level";
			return st.top ();
		}
		void pop ()
		{
			if (st.empty () )
				throw "Not in LOCAL valid level";
			delete st.top ();
			st.pop ();
		}
	private:
		typedef std::stack <LocalLevel *> st_t;
		st_t st;
	};

	#endif

	LocalStack localstack;

	AutoLevel * enterautolocal ();
	void finishautolocal ();
	void checkautolocal (const std::string & varname);

	// ********* Macro **********

	typedef std::map <std::string, Macro> mapmacro_t;
	mapmacro_t mapmacro;

	Macro * getmacro (const std::string & name)
	{
		mapmacro_t::iterator it= mapmacro.find (name);
		if (it == mapmacro.end () )
			return NULL;
		else
			return & it->second;
	}
	bool gotoENDM ();
	void expandMACRO (Macro macro, Tokenizer & tz);
	void parseREPT (Tokenizer & tz);
	void parseIRP (Tokenizer & tz);

	// ******** Paths for include ************

	std::vector <std::string> includepath;
};

//*********************************************************
//		Local classes definitions.
//*********************************************************

namespace {

LocalLevel::LocalLevel (Asm::In & asmin) :
	asmin (asmin)
{ }

LocalLevel::~LocalLevel ()
{
	for (mapvar_t::iterator it= saved.begin ();
		it != saved.end ();
		++it)
	{
		const std::string & name=
			globalized [it->first];
		asmin.mapvar [name]=
			asmin.mapvar [it->first];
		asmin.mapvar [it->first]= it->second;
		std::swap (asmin.vardefined [it->first],
			asmin.vardefined [name] );
	}
}

bool LocalLevel::is_auto () const
{
	return false;
}

void LocalLevel::add (const std::string & var)
{
	// Ignore redeclarations as LOCAL
	// of the same identifier.
	if (saved.find (var) != saved.end () )
		return;

	saved [var]= asmin.mapvar [var];
	const std::string name= localname (asmin.localcount);
	globalized [var]= name;
	std::swap (asmin.vardefined [var],
		asmin.vardefined [name] );
	++asmin.localcount;
	if (asmin.pass == 1)
	{
		asmin.localvalue.push_back (0);
		ASSERT (asmin.localcount ==
			asmin.localvalue.size () );
	}
	else
	{
		ASSERT (asmin.localcount <=
			asmin.localvalue.size () );
		asmin.mapvar [var]=
			asmin.mapvar [name];
	}
}

AutoLevel::AutoLevel (Asm::In & asmin) :
	LocalLevel (asmin)
{ }

bool AutoLevel::is_auto () const
{
	return true;
}

ProcLevel::ProcLevel (Asm::In & asmin, size_t line) :
	LocalLevel (asmin),
	line (line)
{ }

size_t ProcLevel::getline () const
{
	return line;
}

MacroLevel::MacroLevel (Asm::In & asmin) :
	LocalLevel (asmin)
{ }

LocalStack::~LocalStack ()
{
	while (! st.empty () )
		pop ();
}

bool LocalStack::empty () const
{
	return st.empty ();
}

void LocalStack::push (LocalLevel * level)
{
	st.push (level);
}

LocalLevel * LocalStack::top ()
{
	if (st.empty () )
		throw "Not in LOCAL valid level";
	return st.top ();
}

void LocalStack::pop ()
{
	if (st.empty () )
		throw "Not in LOCAL valid level";
	delete st.top ();
	st.pop ();
}

} // namespace

//*********************************************************
//		class Asm::In definitions
//*********************************************************

Asm::In::In () :
	nocase (false),
	autolocalmode (false),
	debugtype (NoDebug),
	base (0),
	current (0),
	currentinstruction (0),
	minused (65535),
	maxused (0),
	hasentrypoint (false),
	pout (& std::cout),
	perr (& std::cerr),
	localcount (0)
{
}

Asm::In::In (const Asm::In & in) :
	nocase (in.nocase),
	autolocalmode (in.autolocalmode),
	debugtype (in.debugtype),
	base (0),
	current (0),
	currentinstruction (0),
	minused (65535),
	maxused (0),
	hasentrypoint (false),
	pout (& std::cout),
	perr (in.perr),
	localcount (0),
	includepath (in.includepath)
{
}

Asm::In::~In ()
{
}

void Asm::In::setdebugtype (DebugType type)
{
	debugtype= type;
}

void Asm::In::errtostdout ()
{
	perr= & std::cout;
}

void Asm::In::setbase (unsigned int addr)
{
	if (addr > 65535)
		throw "Inavlid base value";
	base= static_cast <address> (addr);
	current= base;
	currentinstruction= base;
}

void Asm::In::caseinsensitive ()
{
	nocase= true;
}

void Asm::In::autolocal ()
{
	autolocalmode= true;
}

void Asm::In::addincludedir (const std::string & dirname)
{
	std::string aux (dirname);
	std::string::size_type l= aux.size ();
	if (l == 0)
		return;
	char c= aux [l - 1];
	if (c != '\\' && c != '/')
		aux+= '/';
	includepath.push_back (aux);
}

void Asm::In::openis (std::ifstream & is, const std::string & filename,
	std::ios::openmode mode)
{
	ASSERT (! is.is_open () );
	is.open (filename.c_str (), mode);
	if (is.is_open () )
		return;
	for (size_t i= 0; i < includepath.size (); ++i)
	{
		std::string path (includepath [i] );
		path+= filename;
		is.clear ();
		is.open (path.c_str (), mode);
		if (is.is_open () )
			return;
	}
	throw "File \"" + filename + "\" not found";
}

void Asm::In::setentrypoint (address addr)
{
	if (pass < 2)
		return;
	if (hasentrypoint)
		* perr << "WARNING: Entry point redefined" << endl;
	hasentrypoint= true;
	entrypoint= addr;
}

void Asm::In::checkendline (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	if (tok.type () != TypeEndLine)
	{
		* perr << "Found " << tok.str () << endl;
		throw "End line expected and '" + tok.str () + "'found";
	}
}

void Asm::In::gencode (byte code)
{
	if (current < minused)
		minused= current;
	if (current > maxused)
		maxused= current;
	mem [current]= code;
	++current;
}

void Asm::In::gencodeword (address value)
{
	gencode (value & 0xFF);
	gencode (value >> 8);
}

void Asm::In::setvalue (const std::string & var, address value)
{
	checkautolocal (var);

	mapvar [var]= value;
}

address Asm::In::getvalue (const std::string & var, bool required)
{
	checkautolocal (var);

	mapvar_t::iterator it= mapvar.find (var);
	if (it == mapvar.end () )
	{
		if (pass > 1 || required)
			throw var + " undefined";
		else
			return 0;
	}
	return it->second;
}

void Asm::In::parsevalue (bool required, Tokenizer & tz, address & result)
{
	Token tok= tz.gettoken ();
	switch (tok.type () )
	{
	case TypeNumber:
		result= tok.num ();
		break;
	case TypeIdentifier:
		#if 0
		if (required)
			result= getvalue (tok.str () );
		else
			result= getvalue0 (tok.str () );
		#else
		result= getvalue (tok.str (), required);
		#endif
		break;
	case TypeDollar:
		//result= current;
		result= currentinstruction;
		break;
	case TypeLiteral:
		if (tok.str ().size () != 1)
			throw "Invalid literal, length 1 required";
		result= tok.str () [0];
		break;
	case TypeNUL:
		tok= tz.gettoken ();
		if (tok.type () == TypeEndLine)
			result= addrTRUE;
		else
		{
			// Absorv the rest of the line.
			result= addrFALSE;
			do {
				tok= tz.gettoken ();
			} while (tok.type () != TypeEndLine);
		}
		break;
	default:
		throw "Value Expected but '" + tok.str () + "' found";
	}
}

void Asm::In::parseopen (bool required, Tokenizer & tz, address & result)
{
	Token tok= tz.gettoken ();
	if (tok.type () == TypeOpen)
	{
		parseorxor (required, tz, result);
		#if 0
		tok= tz.gettoken ();
		if (tok.type () != TypeClose)
			throw "Close expected";
		#else
		parseclose (tz);
		#endif
	}
	else
	{
		//tz.putback (tok);
		tz.ungettoken ();
		parsevalue (required, tz, result);
	}
}

void Asm::In::parsemuldiv (bool required, Tokenizer & tz, address & result)
{
	parseopen (required, tz, result);
	Token tok= tz.gettoken ();
	TypeToken tt= tok.type ();
	while (tt == TypeMult || tt == TypeDiv ||
		tt == TypeMOD || tt == TypeMod ||
		tt == TypeSHL || tt == TypeSHR)
	{
		address guard;
		parseopen (required, tz, guard);
		switch (tt)
		{
		case TypeMult:
			result*= guard;
			break;
		case TypeDiv:
			if (guard == 0)
			{
				if (required || pass >= 2)
					throw "Division by zero";
				else
					result= 0;
			}
			else
				result/= guard;
			break;
		case TypeMOD:
		case TypeMod:
			if (guard == 0)
			{
				if (required || pass >= 2)
					throw "Division by zero";
				else
					result= 0;
			}
			else
				result%= guard;
			break;
		case TypeSHL:
			result<<= guard;
			break;
		case TypeSHR:
			result>>= guard;
			break;
		default:
			throw "Unexpected error";
		}
		tok= tz.gettoken ();
		tt= tok.type ();
	}
	//tz.putback (tok);
	tz.ungettoken ();
}

void Asm::In::parseplusmin (bool required, Tokenizer & tz, address & result)
{
	parsemuldiv (required, tz, result);
	Token tok= tz.gettoken ();
	TypeToken tt= tok.type ();
	while (tt == TypePlus || tt == TypeMinus)
	{
		address guard;
		parsemuldiv (required, tz, guard);
		switch (tt)
		{
		case TypePlus:
			result+= guard;
			break;
		case TypeMinus:
			result-= guard;
			break;
		default:
			throw "Unexpected error";
		}
		tok= tz.gettoken ();
		tt= tok.type ();
	}
	//tz.putback (tok);
	tz.ungettoken ();
}

void Asm::In::parserelops (bool required, Tokenizer & tz, address & result)
{
	parseplusmin (required, tz, result);
	Token tok= tz.gettoken ();
	TypeToken tt= tok.type ();
	while (tt == TypeEQ || tt == TypeLT || tt == TypeLE ||
		tt == TypeGT || tt == TypeGE || tt == TypeNE)
	{
		address guard;
		parseplusmin (required, tz, guard);
		switch (tt)
		{
		case TypeEQ:
			result= (result == guard) ? addrTRUE : addrFALSE;
			break;
		case TypeLT:
			result= (result < guard) ? addrTRUE : addrFALSE;
			break;
		case TypeLE:
			result= (result <= guard) ? addrTRUE : addrFALSE;
			break;
		case TypeGT:
			result= (result > guard) ? addrTRUE : addrFALSE;
			break;
		case TypeGE:
			result= (result >= guard) ? addrTRUE : addrFALSE;
			break;
		case TypeNE:
			result= (result != guard) ? addrTRUE : addrFALSE;
			break;
		default:
			throw "Unexpected error";
		}
		tok= tz.gettoken ();
		tt= tok.type ();
	}
	//tz.putback (tok);
	tz.ungettoken ();
}

void Asm::In::parsenot (bool required, Tokenizer & tz, address & result)
{
	Token tok= tz.gettoken ();
	TypeToken tt= tok.type ();
	// NOT and unary + and -.
	if (tt == TypeNOT || tt == TypePlus || tt == TypeMinus)
	{
		parsenot (required, tz, result);
		switch (tt)
		{
		case TypeNOT:
			result= ~ result;
			break;
		case TypePlus:
			break;
		case TypeMinus:
			result= - result;
			break;
		default:
			throw "Unexpected error";
		}
	}
	else
	{
		//tz.putback (tok);
		tz.ungettoken ();
		//parseplusmin (required, tz, result);
		parserelops (required, tz, result);
	}
}

void Asm::In::parseand (bool required, Tokenizer & tz, address & result)
{
	parsenot (required, tz, result);
	Token tok= tz.gettoken ();
	TypeToken tt= tok.type ();
	while (tt == TypeAND)
	{
		address guard;
		parsenot (required, tz, guard);
		result&= guard;
		tok= tz.gettoken ();
		tt= tok.type ();
	}
	//tz.putback (tok);
	tz.ungettoken ();
}

void Asm::In::parseorxor (bool required, Tokenizer & tz, address & result)
{
	parseand (required, tz, result);
	Token tok= tz.gettoken ();
	TypeToken tt= tok.type ();
	while (tt == TypeOR || tt == TypeXOR)
	{
		address guard;
		parseand (required, tz, guard);
		switch (tt)
		{
		case TypeOR:
			result|= guard;
			break;
		case TypeXOR:
			result^= guard;
			break;
		default:
			throw "Unexpected error";
		}
		tok= tz.gettoken ();
		tt= tok.type ();
	}
	//tz.putback (tok);
	tz.ungettoken ();
}

address Asm::In::parseexpr (bool required, const Token & /* tok */,
	Tokenizer & tz)
{
	//tz.putback (tok);
	tz.ungettoken ();
	address result;
	parseorxor (required, tz, result);
	return result;
}

void Asm::In::parsecomma (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	if  (tok.type () != TypeComma)
		throw "Comma expected but '" + tok.str () + "' found";
}

void Asm::In::parseA (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	if  (tok.type () != TypeA)
		throw "Register A expected but '" + tok.str () + "' found";
}

void Asm::In::parseC (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	if  (tok.type () != TypeC)
		throw "Register C expected but '" + tok.str () + "' found";
}

void Asm::In::parseclose (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	if  (tok.type () != TypeClose)
		throw "')' expected but '" + tok.str () + "' found";
}

void Asm::In::parseopen (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	if  (tok.type () != TypeOpen)
		throw "'(' expected but '" + tok.str () + "' found";
}

void Asm::In::loadfile (const std::string & filename)
{
	// Load the file in memory.

	// Changed this to introduce the -I option.
	#if 0
	std::ifstream file (filename.c_str () );
	if (! file.is_open () )
		throw "File not found";
	#else
	std::ifstream file;
	openis (file, filename, std::ios::in);
	#endif

	vfileref.push_back (FileRef (filename) );
	size_t filenum= vfileref.size () - 1;
	std::string line;
	size_t linenum= 0;

	try
	{	
		while (std::getline (file, line) )
		{
			++linenum;
	
			Tokenizer tz (line, nocase);
			Token tok= tz.gettoken ();
			if (tok.type () == TypeINCLUDE)
			{
				std::string includefile= tz.getincludefile ();
				checkendline (tz);
				loadfile (includefile);
			}
			else
			{
				vline.push_back (line);
				vlineinfo.push_back
					(LineInfo (linenum, filenum) );
			}
		}
	}
	catch (...)
	{
		* perr << "ERROR on line " << linenum <<
			" of file " << filename << endl;
		throw;
	}
}

void Asm::In::getvalidline ()
{
	for (;;)
	{
		if (currentline >= vline.size () )
			break;
		const std::string & line= vline [currentline];
		if (! line.empty () && line [0] != ';')
			break;
		++currentline;
	}
}

void Asm::In::parseif (Tokenizer & tz)
{
	address v;
	Token tok= tz.gettoken ();
	v= parseexpr (true, tok, tz);
	checkendline (tz);
	if (v != 0)
		++iflevel;
	else
	{
		size_t ifline= currentline;
		int level= 1;
		for ( ++currentline; currentline < vline.size ();
			++currentline)
		{
			getvalidline ();
			if (currentline >= vline.size () )
				break;
			const std::string & line= vline [currentline];
			Tokenizer tz (line, nocase);
			tok= tz.gettoken ();
			TypeToken tt= tok.type ();
			if (tt == TypeIF)
				++level;
			else if (tt == TypeELSE)
			{
				++iflevel;
				break;
			}
			else if (tt == TypeENDIF)
			{
				--level;
				if (level == 0)
					break;
			}
			else if (tt == TypeENDM)
			{
				// Let the current line be reexamined
				// for ending expandMACRO or emit an
				// error.
				--currentline;
				break;
			}
			else
			{
				* pout << "- " << line << endl;
			}
		}
		if (currentline >= vline.size () )
		{
			currentline= ifline;
			throw "IF without ENDIF";
		}
	}
}

void Asm::In::parseelse ()
{
	if (iflevel == 0)
		throw "ELSE without IF";

	size_t elseline= currentline;
	int level= 1;
	for (++currentline; currentline < vline.size (); ++currentline)
	{
		getvalidline ();
		if (currentline >= vline.size () )
			break;
		const std::string & line= vline [currentline];
		Tokenizer tz (line, nocase);
		Token tok= tz.gettoken ();
		TypeToken tt= tok.type ();
		if (tt == TypeIF)
			++level;
		else if (tt == TypeENDIF)
		{
			--level;
			if (level == 0)
				break;
		}
		else if (tt == TypeENDM)
		{
			// Let the current line be reexamined
			// for ending expandMACRO or emit an
			// error.
			--currentline;
			break;
		}
		else
			* pout << "- " << line << endl;
	}
	if (currentline >= vline.size () )
	{
		currentline= elseline;
		throw "ELSE without ENDIF";
	}
	--iflevel;
}

void Asm::In::parseline (Tokenizer & tz)
{
	Token tok= tz.gettoken ();

	currentinstruction= current;
	switch (tok.type () )
	{
	case TypeEndLine:
		return;
	case TypeORG:
		parseorg (tz);
		break;
	case TypeIdentifier:
		{
			const std::string & name= tok.str ();

			// Check needed to allow redefinition of macro.
			tok= tz.gettoken ();
			//tz.putback (tok);
			tz.ungettoken ();
			if (tok.type () == TypeMACRO)
				parselabel (tz, name);
			else
			{
				Macro * pmacro= getmacro (name);
				if (pmacro != NULL)
				{
					* pout << "Expanding MACRO " <<
						name << endl;
					expandMACRO (* pmacro, tz);
					* pout << "End of MACRO " <<
						name << endl;
				}
				else
					parselabel (tz, name);
			}
		}
		break;
	case TypeIF:
		parseif (tz);
		break;
	case TypeELSE:
		checkendline (tz);
		parseelse ();
		break;
	case TypeENDIF:
		checkendline (tz);
		if (iflevel == 0)
			throw "ENDIF without IF";
		--iflevel;
		break;
	case TypePUBLIC:
		parsePUBLIC (tz);
		break;
	case TypeMACRO:
		// Style MACRO identifier, params
		tok= tz.gettoken ();
		if (tok.type () != TypeIdentifier)
			throw "Identifier expected but '" +
				tok.str () + "' found";
		{
			const std::string & name= tok.str ();
			parsecomma (tz);
			parsemacro (tz, name); 
		}
		break;
	#if 0
	case TypeREPT:
		parseREPT (tz);
		break;
	#endif
	default:
		parsegeneric (tz, tok);
	}
}

AutoLevel * Asm::In::enterautolocal ()
{
	AutoLevel * pav;
	if (localstack.empty () || ! localstack.top ()->is_auto () )
	{
		* perr << "Enter autolocal level" << endl;
		pav= new AutoLevel (* this);
		localstack.push (pav);
	}
	else
	{
		pav= dynamic_cast <AutoLevel *> (localstack.top () );
		ASSERT (pav);
	}
	return pav;
}

void Asm::In::finishautolocal ()
{
	if (! localstack.empty () )
	{
		LocalLevel * plevel= localstack.top ();
		if (plevel->is_auto () )
		{
			if (autolocalmode)
			{
				* perr << "Exit autolocal level" << endl;
				localstack.pop ();
			}
			else
				throw logic_error
					("Unexpected autolocal block");
		}
	}
}

void Asm::In::checkautolocal (const std::string & varname)
{
	if (autolocalmode && varname [0] == AutoLocalPrefix)
	{
		AutoLevel *pav= enterautolocal ();
		pav->add (varname);
	}
}

void Asm::In::dopass ()
{
	initlocal ();
	mapmacro.clear ();
	current= base;
	iflevel= 0;
	for (currentline= 0; currentline < vline.size (); ++currentline)
	{
		getvalidline ();
		if (currentline >= vline.size () )
			break;
		const std::string & line= vline [currentline];

		* pout << line << endl;

		Tokenizer tz (line, nocase);
		parseline (tz);
	}
	if (iflevel > 0)
		throw "IF without ENDIF";

	finishautolocal ();

	if (! localstack.empty () )
	{
		ProcLevel * proc=
			dynamic_cast <ProcLevel *> (localstack.top () );
		if (proc == NULL)
			throw "Unexpected local element open";
		currentline= proc->getline ();
		throw "Unbalanced PROC";
	}
}

void Asm::In::processfile (const std::string & filename)
{
	loadfile (filename);

	// Process the file(s) readed.

	try 
	{
		Nullostream nullout;
		for (pass= 1; pass <= 2; ++pass)
		{
			if (pass == 1)
			{
				if (debugtype == DebugAll)
					pout= & std::cout;
				else
					pout= & nullout;
			}
			else
			{
				if (debugtype != NoDebug)
					pout= & std::cout;
				else
					pout= & nullout;
			}

			dopass ();
		}

		// Keep pout pointing to something valid.
		pout= & std::cout;
	}
	catch (...)
	{
		if (currentline >= vlineinfo.size () )
		{
			//throw "Unexpected error after end of file";
			* perr << "ERROR detected after end of file" << endl;
			throw;
		}
		LineInfo & linf= vlineinfo [currentline];
		* perr << "ERROR on line " << linf.linenum <<
			" of file " << vfileref [linf.filenum].filename <<
			endl;
		throw;
	}
}

void Asm::In::parsegeneric (Tokenizer & tz, Token tok)
{
	TypeToken t= tok.type ();
	simple1byte_t::iterator it= simple1byte.find (t);
	if (it != simple1byte.end () )
	{
		#if 0
		if (tz.gettoken ().type () != TypeEndLine)
			throw "End of line expected";
		#else
		checkendline (tz);
		#endif
		gencode (it->second);
		* pout << hex2 (it->second) << '\t' <<
			tok.str () << endl;
		return;
	}
	it= simpleED.find (t);
	if (it != simpleED.end () )
	{
		#if 0
		if (tz.gettoken ().type () != TypeEndLine)
			throw "End of line expected";
		#else
		checkendline (tz);
		#endif
		gencode (0xED);
		gencode (it->second);
		* pout << "ED " << hex2 (it->second) << '\t' <<
			tok.str () << endl;
		return;
	}

	switch (t)
	{
	case TypeIdentifier:
		{
			Macro * pmacro= getmacro (tok.str () );
			if (pmacro != NULL)
			{
				* pout << "Expanding MACRO " << tok.str () <<
					endl;
				expandMACRO (* pmacro, tz);
				* pout << "End of MACRO " << tok.str () <<
					endl;
			}
			else
				throw "Unexpected identifier " + tok.str ();
		}
		break;
	case TypeDEFB:
	case TypeDB:
	case TypeDEFM:
		parseDEFB (tz);
		break;
	case TypeDEFW:
	case TypeDW:
		parseDEFW (tz);
		break;
	case TypeDEFS:
	case TypeDS:
		parseDEFS (tz);
		break;
	case TypeINCBIN:
		parseINCBIN (tz);
		break;
	case TypeEND:
		parseEND (tz);
		break;
	case TypeLOCAL:
		parseLOCAL (tz);
		break;
	case TypePROC:
		parsePROC (tz);
		break;
	case TypeENDP:
		parseENDP (tz);
		break;
	case TypeMACRO:
		// Is processed previously.
		ASSERT (false);
		throw "Unexpected error in MACRO";
	case TypeREPT:
		parseREPT (tz);
		break;
	case TypeIRP:
		parseIRP (tz);
		break;
	case TypeENDM:
		throw "ENDM outside of MACRO";
	case TypeIM:
		parseIM (tz);
		break;
	case TypeRST:
		parseRST (tz);
		break;
	case TypeLD:
		parseLD (tz);
		break;
	case TypeCP:
		parseCP (tz);
		break;
	case TypeAND:
		parseAND (tz);
		break;
	case TypeOR:
		parseOR (tz);
		break;
	case TypeXOR:
		parseXOR (tz);
		break;
	case TypeRL:
		parseRL (tz);
		break;
	case TypeRLC:
		parseRLC (tz);
		break;
	case TypeRR:
		parseRR (tz);
		break;
	case TypeRRC:
		parseRRC (tz);
		break;
	case TypeSLA:
		parseSLA (tz);
		break;
	case TypeSRA:
		parseSRA (tz);
		break;
	case TypeSRL:
		parseSRL (tz);
		break;
	case TypeSLL:
		parseSLL (tz);
		break;
	case TypeSUB:
		parseSUB (tz);
		break;
	case TypeADD:
		parseADD (tz);
		break;
	case TypeADC:
		parseADC (tz);
		break;
	case TypeSBC:
		parseSBC (tz);
		break;
	case TypePUSH:
		parsePUSHPOP (tz, true);
		break;
	case TypePOP:
		parsePUSHPOP (tz, false);
		break;
	case TypeCALL:
		parseCALL (tz);
		break;
	case TypeRET:
		parseRET (tz);
		break;
	case TypeJP:
		parseJP (tz);
		break;
	case TypeJR:
		parseJR (tz);
		break;
	case TypeDJNZ:
		parseDJNZ (tz);
		break;
	case TypeDEC:
		parseINCDEC (tz, false);
		break;
	case TypeINC:
		parseINCDEC (tz, true);
		break;
	case TypeEX:
		parseEX (tz);
		break;
	case TypeIN:
		parseIN (tz);
		break;
	case TypeOUT:
		parseOUT (tz);
		break;
	case TypeBIT:
		parseBIT (tz);
		break;
	case TypeRES:
		parseRES (tz);
		break;
	case TypeSET:
		parseSET (tz);
		break;
	case TypeEQU:
		throw "EQU without label";
	case TypeDEFL:
		throw "DEFL without label";
	default:
		//throw "Syntax error";
		throw "Unexpected " + tok.str ();
	}
}

void Asm::In::parseorg (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	address org= parseexpr (true, tok, tz);
	current= org;
	* pout << "\tORG " << org << endl;
}

void Asm::In::parsePUBLIC (Tokenizer & tz)
{
	for (;;)
	{
		Token tok= tz.gettoken ();
		if (tok.type () != TypeIdentifier)
			throw "Invalid PUBLIC declaration";
		setpublic.insert (tok.str () );
		tok= tz.gettoken ();
		if (tok.type () == TypeEndLine)
			break;
		if (tok.type () != TypeComma)
			throw "Invalid PUBLIC declaration";
	}
}

void Asm::In::parseEND (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	if (tok.type () != TypeEndLine)
	{
		address end= parseexpr (false, tok, tz);
		checkendline (tz);
		setentrypoint (end);
	}
	currentline= vline.size ();
}

void Asm::In::parseLOCAL (Tokenizer & tz)
{
	if (autolocalmode)
		throw "LOCAL directive not allowed in autolocal mode";
	LocalLevel * plocal= localstack.top ();
	for (;;)
	{
		Token tok= tz.gettoken ();
		if (tok.type () != TypeIdentifier)
			//throw "Syntax error";
			throw "Unexpected " + tok.str ();
		plocal->add (tok.str () );
		tok= tz.gettoken ();
		TypeToken tt= tok.type ();
		if (tt == TypeEndLine)
			break;
		if (tt != TypeComma)
			//throw "Syntax error";
			throw "Unexpected " + tok.str ();
	}
}

void Asm::In::parsePROC (Tokenizer & tz)
{
	if (autolocalmode)
		throw "PROC directive not allowed in autolocal mode";
	checkendline (tz);
	ProcLevel * pproc= new ProcLevel (* this, currentline);
	localstack.push (pproc);
}

void Asm::In::parseENDP (Tokenizer & tz)
{
	checkendline (tz);
	if (localstack.empty () ||
		dynamic_cast <ProcLevel *> (localstack.top () ) == NULL)
	{
		throw "Unbalanced ENDP";
	}
	localstack.pop ();
}

namespace {

const char redefinedEQU []= " Redefined from previous EQU or label";
const char redefinedDEFL []= " Redefined from previous DEFL";

} // namespace

void Asm::In::setequorlabel (const std::string & name, address value)
{
	if (autolocalmode)
	{
		if (name [0] == AutoLocalPrefix)
		{
			AutoLevel *pav= enterautolocal ();
			ASSERT (pav);
			pav->add (name);
		}
		else
			finishautolocal ();
	}

	switch (vardefined [name] )
	{
	case NoDefined:
		if (pass > 1)
			throw name + " undefined in pass 1";
		// Else nothing to do.
		break;
	case DefinedDEFL:
		throw name + redefinedDEFL;
	case DefinedPass1:
		if (pass == 1)
			throw name + redefinedEQU;
		// Else nothing to do (this may chnage).
		break;
	case DefinedPass2:
		ASSERT (pass > 1);
		throw name + redefinedEQU;
	}
	vardefined [name]= pass == 1 ? DefinedPass1 : DefinedPass2;
	setvalue (name, value);
}

void Asm::In::parselabel (Tokenizer & tz, const std::string & name)
{
	Token tok= tz.gettoken ();
	TypeToken tt= tok.type ();
	if (tt == TypeColon)
	{
		tok= tz.gettoken ();
		tt= tok.type ();
	}

	switch (tt)
	{
	case TypeEQU:
		{
			tok= tz.gettoken ();
			address value= parseexpr (false, tok, tz);
			checkendline (tz);
			setequorlabel (name, value);
		}
		break;
	case TypeDEFL:
		{
			tok= tz.gettoken ();
			address value= parseexpr (false, tok, tz);
			checkendline (tz);
			switch (vardefined [name] )
			{
			case NoDefined:
				// Fine in this case.
				break;
			case DefinedDEFL:
				// Fine also.
				break;
			case DefinedPass1:
			case DefinedPass2:
				throw name + redefinedEQU;
			}
			vardefined [name]= DefinedDEFL;
			setvalue (name, value);
		}
		break;
	case TypeMACRO:
		// Style identifier MACRO params 
		parsemacro (tz, name);
		break;
	default:
		// In any other case, generic label. Assign the
		// current position to it and parse the rest
		// of the line.

		//setvalue (name, current);
		setequorlabel (name, current);
		if (tok.type () != TypeEndLine)
			parsegeneric (tz, tok);
	}
}

void Asm::In::parsemacro (Tokenizer & tz, const std::string & name)
{
	* pout << "Defining MACRO " << name << endl;

	// Get parameter list.
	std::vector <std::string> param;
	Token tok= tz.gettoken ();
	TypeToken tt= tok.type ();
	if (tt != TypeEndLine)
		for (;;)
		{
			if (tt != TypeIdentifier)
				throw "Identifier expected but '" +
					tok.str () + "' found";
			param.push_back (tok.str () );
			tok= tz.gettoken ();
			tt= tok.type ();
			if (tt == TypeEndLine)
				break;
			tok= tz.gettoken ();
			tt= tok.type ();
		}

	// Clear previous definition if exists.
	mapmacro_t::iterator it= mapmacro.find (name);
	if (it != mapmacro.end () )
		mapmacro.erase (it);

	// Store the macro definition.
	mapmacro.insert (std::make_pair (name,
		Macro (currentline, param) ) );

	// Skip macro body.
	size_t level= 1;
	size_t macroline= currentline;
	for (++currentline;
		currentline < vline.size ();
		++currentline)
	{
		Tokenizer tz (vline [currentline], nocase);
		Token tok= tz.gettoken ();
		TypeToken tt= tok.type ();
		if (tt == TypeENDM)
		{
			if (--level == 0)
				break;
		}
		if (tt == TypeIdentifier)
		{
			tok= tz.gettoken ();
			tt= tok.type ();
		}
		if (tt == TypeMACRO || tt == TypeREPT || tt == TypeIRP)
			++level;
	}
	if (currentline >= vline.size () )
	{
		currentline= macroline;
		throw "MACRO without ENDM";
	}
}

enum { regBC= 0, regDE= 1, regHL= 2, regAF= 3, regSP= 3 };

enum { regA= 7, regB= 0, regC= 1, regD= 2, regE= 3,
	regH= 4, regL= 5, reg_HL_= 6
};

void Asm::In::parsedesp (Tokenizer & tz, byte & desp)
{
	Token tok= tz.gettoken ();
	switch (tok.type () )
	{
	case TypeClose:
		desp= 0;
		break;
	case TypePlus:
		tok= tz.gettoken ();
		{
			address addr= parseexpr (false, tok, tz);
			// We allow positive greater than 127 just in
			// case someone uses hexadecimals such as 0FFh
			// as offsets.
			if (addr > 255)
				throw "Offset out of range";
			desp= static_cast <byte> (addr);
			parseclose (tz);
		}
		break;
	case TypeMinus:
		tok= tz.gettoken ();
		{
			address addr= parseexpr (false, tok, tz);
			if (addr > 128)
				throw "Offset out of range";
			desp= static_cast <byte> (256 - addr);
			parseclose (tz);
		}
		break;
	default:
		throw "Expected '+', '-' or ')' but '" +
			tok.str () + "' found";
	}
}

bool Asm::In::parsebyteparam (Tokenizer & tz, TypeToken tt,
	unsigned short & regcode,
	byte & prefix, bool & hasdesp, byte & desp,
	byte prevprefix)
{
	prefix= 0;
	hasdesp= false;
	desp= 0;
	Token tok;
	switch (tt)
	{
	case TypeA:
		regcode= regA; break;
	case TypeB:
		regcode= regB; break;
	case TypeC:
		regcode= regC; break;
	case TypeD:
		regcode= regD; break;
	case TypeE:
		regcode= regE; break;
	case TypeH:
		regcode= regH; break;
	case TypeL:
		regcode= regL; break;
	case TypeIXH:
		if (prevprefix == prefixIY)
			throw "Invalid instruction";
		if (prevprefix == 0)
			prefix= prefixIX;
		regcode= regH;
		break;
	case TypeIYH:
		if (prevprefix == prefixIX)
			throw "Invalid instruction";
		if (prevprefix == 0)
			prefix= prefixIY;
		regcode= regH;
		break;
	case TypeIXL:
		if (prevprefix == prefixIY)
			throw "Invalid instruction";
		if (prevprefix == 0)
			prefix= prefixIX;
		regcode= regL;
		break;
	case TypeIYL:
		if (prevprefix == prefixIX)
			throw "Invalid instruction";
		if (prevprefix == 0)
			prefix= prefixIY;
		regcode= regL;
		break;
	case TypeOpen:
		tok= tz.gettoken ();
		switch (tok.type () )
		{
		case TypeHL:
			regcode= reg_HL_;
			parseclose (tz);
			break;
		case TypeIX:
			regcode= reg_HL_;
			prefix= prefixIX;
			hasdesp= true;
			parsedesp (tz, desp);
			break;
		case TypeIY:
			regcode= reg_HL_;
			prefix= prefixIY;
			hasdesp= true;
			parsedesp (tz, desp);
			break;
		default:
			//throw "Error por ahora";
			// Backtrack the parsing to the begiining
			// of the expression.
			tz.ungettoken ();
			return false;
		}
		if (prevprefix != 0)
			throw "Invalid instruction";
		break;
	default:
		return false;
	}
	return true;
}

void Asm::In::dobyteparam (Tokenizer & tz, byte codereg, byte codein)
{
	Token tok= tz.gettoken ();
	unsigned short reg;
	byte prefix;
	bool hasdesp;
	byte desp;
	if (parsebyteparam (tz, tok.type (), reg, prefix, hasdesp, desp) )
	{
		checkendline (tz);
		if (prefix)
		{
			gencode (prefix);
			* pout << hex2 (prefix) << ' ';
		}
		byte code= codereg + reg;
		gencode (code);
		* pout << hex2 (code);
		if (hasdesp)
		{
			gencode (desp);
			* pout << ' ' << hex2 (desp);
		}
		* pout << endl;
	}
	else
	{
		//address addr= getvalue0 (tok);
		address addr= parseexpr (false, tok, tz);
		checkendline (tz);
		gencode (codein);
		byte param= static_cast <byte> (addr);
		gencode (param);
		* pout << hex2 (codein) << ' ' << hex2 (param) <<
			endl;
	}
}

void Asm::In::dobyteparamCB (Tokenizer & tz, byte codereg)
{
	Token tok= tz.gettoken ();
	unsigned short reg;
	byte prefix;
	bool hasdesp;
	byte desp;
	if (parsebyteparam (tz, tok.type (), reg, prefix, hasdesp, desp) )
	{
		checkendline (tz);
		if (prefix)
		{
			gencode (prefix);
			* pout << hex2 (prefix) << ' ';
		}
		gencode (0xCB);
		* pout << "CB ";
		if (hasdesp)
		{
			gencode (desp);
			* pout << hex2 (desp) << ' ';
		}
		byte code= codereg + reg;
		gencode (code);
		* pout << hex2 (code);
		* pout << endl;
	}
	else
		throw "Invalid operand";
}

void Asm::In::parseIM (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	//address v= getvalue (tok);
	address v= parseexpr (true, tok, tz);
	byte code;
	switch (v)
	{
	case 0:
		code= 0x46; break;
	case 1:
		code= 0x56; break;
	case 2:
		code= 0x5E; break;
	default:
		throw "Invalid IM value";
	}
	checkendline (tz);
	gencode (0xED);
	gencode (code);
	* pout << "ED " << hex2 (code) <<
		"\tIM " << v << endl;
}

void Asm::In::parseRST (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	//address v= getvalue (tok);
	address v= parseexpr (true, tok, tz);
	byte code;
	switch (v)
	{
	case 0x00:
		code= 0xC7; break;
	case 0x08:
		code= 0xCF; break;
	case 0x10:
		code= 0xD7; break;
	case 0x18:
		code= 0xDF; break;
	case 0x20:
		code= 0xE7; break;
	case 0x28:
		code= 0xEF; break;
	case 0x30:
		code= 0xF7; break;
	case 0x38:
		code= 0xFF; break;
	default:
		throw "Invalid RST value";
	}
	checkendline (tz);
	gencode (code);
	* pout << int (code) <<
		"\tRST " << hex2 (v) << endl;
}

void Asm::In::parseLDA (Tokenizer & tz)
{
	parsecomma (tz);
	Token tok= tz.gettoken ();
	switch (tok.type () )
	{
	case TypeA:
		gencode (0x7F);
		* pout << "7F\tLD A, A" << endl;
		break;
	case TypeB:
		gencode (0x78);
		* pout << "78\tLD A, B" << endl;
		break;
	case TypeC:
		gencode (0x79);
		* pout << "79\tLD A, C" << endl;
		break;
	case TypeD:
		gencode (0x7A);
		* pout << "7A\tLD A, D" << endl;
		break;
	case TypeE:
		gencode (0x7B);
		* pout << "7B\tLD A, E" << endl;
		break;
	case TypeH:
		gencode (0x7C);
		* pout << "7C\tLD A, H" << endl;
		break;
	case TypeI:
		gencode (0xED);
		gencode (0x57);
		* pout << "ED 57\tLD A, I" << endl;
		break;
	case TypeL:
		gencode (0x7D);
		* pout << "7D\tLD A, L" << endl;
		break;
	case TypeR:
		gencode (0xED);
		gencode (0x5F);
		* pout << "ED 5F\tLD A, R" << endl;
		break;
	case TypeIXH:
		gencode (prefixIX);
		gencode (0x7C);
		* pout << "DD 7C\tLD A, IXH" << endl;
		break;
	case TypeIXL:
		gencode (prefixIX);
		gencode (0x7D);
		* pout << "DD 7D\tLD A, IXL" << endl;
		break;
	case TypeIYH:
		gencode (prefixIY);
		gencode (0x7C);
		* pout << "FD 7C\tLD A, IYH" << endl;
		break;
	case TypeIYL:
		gencode (prefixIY);
		gencode (0x7D);
		* pout << "FD 7D\tLD A, IYL" << endl;
		break;
	case TypeOpen:
		tok= tz.gettoken ();
		switch (tok.type () )
		{
		case TypeBC:
			parseclose (tz);
			gencode (0x0A);
			* pout << "0A\tLD A, (BC)" << endl;
			break;
		case TypeDE:
			parseclose (tz);
			gencode (0x1A);
			* pout << "1A\tLD A, (DE)" << endl;
			break;
		case TypeHL:
			parseclose (tz);
			gencode (0x7E);
			* pout << "7E\tLD A, (HL)" << endl;
			break;
		case TypeIX:
			{
				byte desp;
				parsedesp (tz, desp);
				gencode (prefixIX);
				gencode (0x7E);
				gencode (desp);
				* pout << "DD 7E " <<
					hex2 (desp) <<
					"\tLD A, (IX+n)" << endl;
			}
			break;
		case TypeIY:
			{
				byte desp;
				parsedesp (tz, desp);
				gencode (prefixIY);
				gencode (0x7E);
				gencode (desp);
				* pout << "DD 7E " <<
					hex2 (desp) <<
					"\tLD A, (IX+n)" << endl;
			}
			break;
		default:
			{
				//address addr= getvalue0 (tok);
				address addr= parseexpr (false, tok, tz);
				parseclose (tz);
				gencode (0x3A);
				gencodeword (addr);
				* pout << "3A " << hex2 (addr & 0xFF) <<
					' ' << (addr >> 8) <<
					"\tLD A, (nn)" << endl;
			}
		}
		break;
	default:
		//address value= getvalue (tok);
		// Require a value in pass 1 seems to be an error.
		//address value= parseexpr (true, tok, tz);
		address value= parseexpr (false, tok, tz);
		gencode (0x3E);
		gencode (value & 0xFF);
		* pout << "3E " << hex2 (value & 0xFF) <<
			"\tLD A, n" << endl;
	}
	checkendline (tz);
}

void Asm::In::parseLDsimple (Tokenizer & tz, unsigned short regcode,
	byte prevprefix)
{
	#if 0
	switch (regcode)
	{
	case regB:
		code= 0x06;
		break;
	case regC:
		code= 0x0E;
		break;
	case regD:
		code= 0x16;
		break;
	case regE:
		code= 0x1E;
		break;
	case regH:
		code= 0x26;
		break;
	case regL:
		code= 0x2E;
		break;
	case reg_HL_:
		code= 0x36;
		break;
	default:
		throw "Error por ahora";
	}
	#endif

	parsecomma (tz);
	Token tok= tz.gettoken ();

	unsigned short reg2;
	#if 0
	switch (tok.type () )
	{
	case TypeA:
		reg2= regA; break;
	case TypeB:
		reg2= regB; break;
	case TypeC:
		reg2= regC; break;
	case TypeD:
		reg2= regD; break;
	case TypeE:
		reg2= regE; break;
	case TypeH:
		reg2= regH; break;
	case TypeL:
		reg2= regL; break;
	case TypeOpen:
		tok= tz.gettoken ();
		switch (tok.type () )
		{
		case TypeHL:
			reg2= reg_HL_;
			parseclose (tz);
			break;
		default:
			throw "Error por ahora";
		}
		break;
	default:
		checkendline (tz);
		// Por ahora
		code= (regcode << 3) + 0x06;
		//address value= getvalue (tok);
		address value= parseexpr (true, tok, tz);

		gencode (code);
		gencode (value & 0xFF);
		* pout << hex2 (code) << ' ' <<
			hex2 (value & 0xFF) << endl;
		return;
	}
	#else
	byte prefix;
	bool hasdesp;
	byte desp;
	if (parsebyteparam
		(tz, tok.type (), reg2, prefix, hasdesp, desp, prevprefix) )
	{
		checkendline (tz);
		if (prevprefix != 0 && prefix != 0)
			throw "Invalid instruction";
		if (prefix)
		{
			gencode (prefix);
			* pout << hex2 (prefix) << ' ';
		}
		if (prevprefix)
		{
			gencode (prevprefix);
			* pout << hex2 (prevprefix) << ' ';
		}
		byte code= 0x40 + (regcode << 3) + reg2;
		gencode (code);
		* pout << hex2 (code);
		if (hasdesp)
		{
			gencode (desp);
			* pout << ' ' << hex2 (desp);
		}
		* pout << endl;
	}
	else
	{
		// Por ahora
		byte code= (regcode << 3) + 0x06;
		//address value= getvalue (tok);
		// Require a value in pass 1 seems to be an error.
		//address value= parseexpr (true, tok, tz);
		address value= parseexpr (false, tok, tz);
		checkendline (tz);
		if (prevprefix)
		{
			gencode (prevprefix);
			* pout << hex2 (prevprefix) << ' ';
		}
		gencode (code);
		gencode (value & 0xFF);
		* pout << hex2 (code) << ' ' <<
			hex2 (value & 0xFF) << endl;
	}
	#endif
}

void Asm::In::parseLDdouble (Tokenizer & tz, unsigned short regcode, byte prefix)
{
	parsecomma (tz);
	Token tok= tz.gettoken ();

	if (tok.type () == TypeOpen)
	{
		tok= tz.gettoken ();
		//address value= getvalue0 (tok);
		address value= parseexpr (false, tok, tz);
		parseclose (tz);
		checkendline (tz);
		switch (regcode)
		{
		case regBC:
			gencode (0xED);
			gencode (0x4B);
			gencodeword (value);
			* pout << "ED 4B " << hex4 (value) <<
				"\tLD BC, (nn)" << endl;
			break;
		case regDE:
			gencode (0xED);
			gencode (0x5B);
			gencodeword (value);
			* pout << "ED 5B " << hex4 (value) <<
				"\tLD DE, (nn)" << endl;
			break;
		case regHL:
			if (prefix)
			{
				gencode (prefix);
				* pout << hex2 (prefix) << ' ';
			}
			gencode (0x2A);
			gencodeword (value);
			* pout << "2A " << hex4 (value) << endl;
			break;
		default:
			throw "Unexpected error";
		}
		return;
	}

	// Por ahora
	//address value= getvalue0 (tok);
	address value= parseexpr (false, tok, tz);
	checkendline (tz);

	if (prefix)
	{
		gencode (prefix);
		* pout << hex2 (prefix) << ' ';
	}
	gencode (regcode * 16 + 1);
	gencodeword (value);
	* pout << hex2 (regcode * 16 + 1) << ' ' <<
		hex4 (value) << endl;
}

void Asm::In::parseLDSP (Tokenizer & tz)
{
	parsecomma (tz);
	Token tok= tz.gettoken ();
	switch (tok.type () )
	{
	case TypeHL:
		gencode (0xF9);
		* pout << "F9\tLD SP, HL" << endl;
		break;
	case TypeIX:
		gencode (prefixIX);
		gencode (0xF9);
		* pout << "DD F9\tLD SP, IX" << endl;
		break;
	case TypeIY:
		gencode (prefixIY);
		gencode (0xF9);
		* pout << "FD F9\tLD SP, IY" << endl;
		break;
	case TypeOpen:
		{
			tok= tz.gettoken ();
			//address addr= getvalue0 (tok);
			address addr= parseexpr (false, tok, tz);
			parseclose (tz);
			gencode (0xED);
			gencode (0x7B);
			gencodeword (addr);
			* pout << "ED 7B " << hex4 (addr) <<
				"\tLD SP, (nn)" << endl;
		}
		break;
	default:
		{
			//address addr= getvalue0 (tok);
			address addr= parseexpr (false, tok, tz);
			gencode (0x31);
			gencodeword (addr);
			* pout << "31 " << hex4 (addr) <<
				"\tLD SP, nn" << endl;
		}
	}
	checkendline (tz);
}

void Asm::In::parseLD (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	switch (tok.type () )
	{
	case TypeA:
		//parseLDsimple (tz, regA);
		parseLDA (tz);
		break;
	case TypeB:
		parseLDsimple (tz, regB);
		break;
	case TypeC:
		parseLDsimple (tz, regC);
		break;
	case TypeD:
		parseLDsimple (tz, regD);
		break;
	case TypeE:
		parseLDsimple (tz, regE);
		break;
	case TypeH:
		parseLDsimple (tz, regH);
		break;
	case TypeL:
		parseLDsimple (tz, regL);
		break;
	case TypeIXH:
		parseLDsimple (tz, regH, prefixIX);
		break;
	case TypeIYH:
		parseLDsimple (tz, regH, prefixIY);
		break;
	case TypeIXL:
		parseLDsimple (tz, regL, prefixIX);
		break;
	case TypeIYL:
		parseLDsimple (tz, regL, prefixIY);
		break;
	case TypeI:
		parsecomma (tz);
		parseA (tz);
		checkendline (tz);
		gencode (0xED);
		gencode (0x47);
		* pout << "ED 47\tLD I, A" << endl;
		break;
	case TypeR:
		parsecomma (tz);
		parseA (tz);
		checkendline (tz);
		gencode (0xED);
		gencode (0x4F);
		* pout << "ED 4F\tLD R, A" << endl;
		break;
	case TypeBC:
		parseLDdouble (tz, regBC, 0);
		break;
	case TypeDE:
		parseLDdouble (tz, regDE, 0);
		break;
	case TypeHL:
		parseLDdouble (tz, regHL, 0);
		break;
	case TypeIX:
		parseLDdouble (tz, regHL, prefixIX);
		break;
	case TypeIY:
		parseLDdouble (tz, regHL, prefixIY);
		break;
	case TypeSP:
		parseLDSP (tz);
		break;
	case TypeOpen:
		tok= tz.gettoken ();
		switch (tok.type () )
		{
		case TypeBC:
			parseclose (tz);
			parsecomma (tz);
			#if 0
			tok= tz.gettoken ();
			if (tok.type () != TypeA)
				throw "Invalid operand";
			#else
			parseA (tz);
			#endif
			checkendline (tz);
			gencode (0x02);
			* pout << "02\tLD (BC), A" << endl;
			break;
		case TypeDE:
			parseclose (tz);
			parsecomma (tz);
			#if 0
			tok= tz.gettoken ();
			if (tok.type () != TypeA)
				throw "Invalid operand";
			#else
			parseA (tz);
			#endif
			checkendline (tz);
			gencode (0x12);
			* pout << "12\tLD (DE), A" << endl;
			break;
		case TypeHL:
			parseclose (tz);
			parseLDsimple (tz, reg_HL_);
			break;
		case TypeIX:
			{
				byte desp;
				parsedesp (tz, desp);
				parsecomma (tz);
				tok= tz.gettoken ();
				unsigned short reg;
				byte prefix= 0;
				bool hasdesp;
				byte despnotused;
				if (parsebyteparam (tz, tok.type (),
					reg, prefix, hasdesp, despnotused) )
				{
					checkendline (tz);
					if (prefix || hasdesp ||
							reg == reg_HL_)
						throw "Operand invalid";
					byte code= 0x70 + reg;
					gencode (prefixIX);
					gencode (code);
					gencode (desp);
					* pout << "DD "  <<
						hex2 (code) <<
						' ' << hex2 (desp) <<
						endl;
				}
				else
				{
					//address addr= getvalue0 (tok);
					address addr=
						parseexpr (false, tok, tz);
					checkendline (tz);
					gencode (prefixIX);
					gencode (0x36);
					gencode (desp);
					byte n= static_cast <byte> (addr);
					gencode (n);
					* pout << "DD 36 " <<
						hex2 (desp) << ' ' <<
						hex2 (n) << endl;
				}
			}
			break;
		case TypeIY:
			{
				byte desp;
				parsedesp (tz, desp);
				parsecomma (tz);
				tok= tz.gettoken ();
				unsigned short reg;
				byte prefix= 0;
				bool hasdesp;
				byte despnotused;
				if (parsebyteparam (tz, tok.type (),
					reg, prefix, hasdesp, despnotused) )
				{
					checkendline (tz);
					if (prefix || hasdesp ||
							reg == reg_HL_)
						throw "Operand invalid";
					byte code= 0x70 + reg;
					gencode (prefixIY);
					gencode (code);
					gencode (desp);
					* pout << "FD "  <<
						hex2 (code) <<
						' ' << hex2 (desp) <<
						endl;
				}
				else
				{
					//address addr= getvalue0 (tok);
					address addr=
						parseexpr (false, tok, tz);
					checkendline (tz);
					gencode (prefixIY);
					gencode (0x36);
					gencode (desp);
					byte n= static_cast <byte> (addr);
					gencode (n);
					* pout << "FD 36 " <<
						hex2 (desp) << ' ' <<
						hex2 (n) << endl;
				}
			}
			break;
		default:
			// LD (nn), ...
			{
				//address addr= getvalue0 (tok);
				address addr= parseexpr (false, tok, tz);
				parseclose (tz);
				parsecomma (tz);
				tok= tz.gettoken ();
				byte code;
				byte prefix= 0;
				switch (tok.type () )
				{
				case TypeA:
					code= 0x32;
					break;
				case TypeBC:
					prefix= 0xED;
					code= 0x43;
					break;
				case TypeDE:
					prefix= 0xED;
					code= 0x53;
					break;
				case TypeHL:
					code= 0x22;
					break;
				case TypeIX:
					prefix= prefixIX;
					code= 0x22;
					break;
				case TypeIY:
					prefix= prefixIY;
					code= 0x22;
					break;
				case TypeSP:
					prefix= 0xED;
					code= 0x73;
					break;
				default:
					throw "Operand invalid";
				}
				checkendline (tz);
				if (prefix)
				{
					gencode (prefix);
					* pout << hex2 (prefix) << ' ';
				}
				gencode (code);
				* pout << hex2 (code) << ' ';
				gencodeword (addr);
				* pout << hex4 (addr) << endl;
			}
		}
		break;
	default:
		throw "Operand invalid";
	}
}

void Asm::In::parseCP (Tokenizer & tz)
{
	#if 0
	unsigned short reg;
	Token tok= tz.gettoken ();
	byte prefix;
	bool hasdesp;
	byte desp;
	if (parsebyteparam (tz, tok.type (), reg, prefix, hasdesp, desp) )
	{
		checkendline (tz);
		if (prefix)
		{
			gencode (prefix);
			* pout << hex2 (prefix) << ' ';
		}
		byte code= 0xB8 + reg;
		gencode (code);
		* pout << hex2 (code);
		if (hasdesp)
		{
			gencode (desp);
			* pout << ' ' << hex2 (desp);
		}
		* pout << endl;
	}
	else
	{
		//address addr= getvalue0 (tok);
		address addr= parseexpr (false, tok, tz);
		checkendline (tz);
		gencode (0xFE);
		byte param= static_cast <byte> (addr);
		gencode (param);
		* pout << "FE " << hex2 (param) <<
			"\tCP nn" << endl;
	}
	#else
	dobyteparam (tz, 0xB8, 0xFE);
	#endif
}

void Asm::In::parseAND (Tokenizer & tz)
{
	#if 0
	unsigned short reg;
	Token tok= tz.gettoken ();
	byte prefix;
	bool hasdesp;
	byte desp;
	if (parsebyteparam (tz, tok.type (), reg, prefix, hasdesp, desp) )
	{
		checkendline (tz);
		if (prefix)
		{
			gencode (prefix);
			* pout << hex2 (prefix) << ' ';
		}
		byte code= 0xA0 + reg;
		gencode (code);
		* pout << hex2 (code);
		if (hasdesp)
		{
			gencode (desp);
			* pout << ' ' << hex2 (desp);
		}
		* pout << endl;
	}
	else
	{
		//address addr= getvalue0 (tok);
		address addr= parseexpr (false, tok, tz);
		checkendline (tz);
		gencode (0xE6);
		byte param= static_cast <byte> (addr);
		gencode (param);
		* pout << "E6 " << hex2 (param) <<
			"\tAND n" << endl;
	}
	#else
	dobyteparam (tz, 0xA0, 0xE6);
	#endif
}

void Asm::In::parseOR (Tokenizer & tz)
{
	#if 0
	unsigned short reg;
	Token tok= tz.gettoken ();
	byte prefix;
	bool hasdesp;
	byte desp;
	if (parsebyteparam (tz, tok.type (), reg, prefix, hasdesp, desp) )
	{
		checkendline (tz);
		if (prefix)
		{
			gencode (prefix);
			* pout << hex2 (prefix) << ' ';
		}
		byte code= 0xB0 + reg;
		gencode (code);
		* pout << hex2 (code);
		if (hasdesp)
		{
			gencode (desp);
			* pout << ' ' << hex2 (desp);
		}
		* pout << endl;
	}
	else
	{
		//address addr= getvalue0 (tok);
		address addr= parseexpr (false, tok, tz);
		checkendline (tz);
		gencode (0xF6);
		byte param= static_cast <byte> (addr);
		gencode (param);
		* pout << "F6 " << hex2 (param) <<
			"\tOR n" << endl;
	}
	#else
	dobyteparam (tz, 0xB0, 0xF6);
	#endif
}

void Asm::In::parseXOR (Tokenizer & tz)
{
	dobyteparam (tz, 0xA8, 0xEE);
}

void Asm::In::parseRL (Tokenizer & tz)
{
	dobyteparamCB (tz, 0x10);
}

void Asm::In::parseRLC (Tokenizer & tz)
{
	dobyteparamCB (tz, 0x00);
}

void Asm::In::parseRR (Tokenizer & tz)
{
	dobyteparamCB (tz, 0x18);
}

void Asm::In::parseRRC (Tokenizer & tz)
{
	dobyteparamCB (tz, 0x08);
}

void Asm::In::parseSLA (Tokenizer & tz)
{
	dobyteparamCB (tz, 0x20);
}

void Asm::In::parseSRA (Tokenizer & tz)
{
	dobyteparamCB (tz, 0x28);
}

void Asm::In::parseSRL (Tokenizer & tz)
{
	dobyteparamCB (tz, 0x38);
}

void Asm::In::parseSLL (Tokenizer & tz)
{
	dobyteparamCB (tz, 0x30);
}

void Asm::In::parseSUB (Tokenizer & tz)
{
	dobyteparam (tz, 0x90, 0xD6);
}

void Asm::In::parseADDADCSBCHL (Tokenizer & tz, byte prefix, byte basecode)
{
	parsecomma (tz);
	Token tok= tz.gettoken ();
	unsigned short reg;
	switch (tok.type () )
	{
	case TypeBC:
		reg= regBC; break;
	case TypeDE:
		reg= regDE; break;
	case TypeHL:
		if (prefix != 0)
			throw "Operand invalid";
		reg= regHL; break;
	case TypeSP:
		reg= regSP; break;
	case TypeIX:
		if (prefix != prefixIX)
			throw "Operand invalid";
		reg= regHL;
		break;
	case TypeIY:
		if (prefix != prefixIY)
			throw "Operand invalid";
		reg= regHL;
		break;
	default:
		throw "Operand invalid";
	}
	checkendline (tz);
	if (prefix)
	{
		gencode (prefix);
		* pout << hex2 (prefix) << ' ';
	}
	if (basecode == 0x42 || basecode == 0x4A)
	{
		gencode (0xED);
		* pout << "ED ";
	}
	//byte code= (reg << 4) + (isADD ? 0x09 : 0x4A);
	byte code= (reg << 4) + basecode;
	gencode (code);
	* pout << hex2 (code) << endl;
}

void Asm::In::parseADD (Tokenizer & tz)
{
	//unsigned short reg;
	Token tok= tz.gettoken ();
	//byte prefix;
	//bool hasdesp;
	//byte desp;
	switch (tok.type () )
	{
	case TypeA:
		break;
	case TypeHL:
		parseADDADCSBCHL (tz, 0, 0x09);
		return;
	case TypeIX:
		parseADDADCSBCHL (tz, prefixIX, 0x09);
		return;
	case TypeIY:
		parseADDADCSBCHL (tz, prefixIY, 0x09);
		return;
	default:
		throw "Invalid operand";
	}
	parsecomma (tz);
	#if 0
	tok= tz.gettoken ();
	if (parsebyteparam (tz, tok.type (), reg, prefix, hasdesp, desp) )
	{
		checkendline (tz);
		if (prefix)
		{
			gencode (prefix);
			* pout << hex2 (prefix) << ' ';
		}
		byte code= 0x80 + reg;
		gencode (code);
		* pout << hex2 (code);
		if (hasdesp)
		{
			gencode (desp);
			* pout << ' ' << hex2 (desp);
		}
		* pout << endl;
	}
	else
	{
		//address addr= getvalue0 (tok);
		address addr= parseexpr (false, tok, tz);
		checkendline (tz);
		gencode (0xC6);
		byte param= static_cast <byte> (addr);
		gencode (param);
		* pout << "C6 " << hex2 (param) <<
			"\tADD A, n" << endl;
	}
	#else
	dobyteparam (tz, 0x80, 0xC6);
	#endif
}

void Asm::In::parseADC (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	switch (tok.type () )
	{
	case TypeA:
		break;
	case TypeHL:
		parseADDADCSBCHL (tz, 0, 0x4A);
		return;
	default:
		throw "Invalid param to ADC";
	}
	parsecomma (tz);
	#if 0
	tok= tz.gettoken ();
	unsigned short reg;
	byte prefix;
	bool hasdesp;
	byte desp;
	if (parsebyteparam (tz, tok.type (), reg, prefix, hasdesp, desp) )
	{
		checkendline (tz);
		if (prefix)
		{
			gencode (prefix);
			* pout << hex2 (prefix) << ' ';
		}
		byte code= 0x88 + reg;
		gencode (code);
		* pout << hex2 (code);
		if (hasdesp)
		{
			gencode (desp);
			* pout << ' ' << hex2 (desp);
		}
		* pout << endl;
	}
	else
	{
		//address addr= getvalue0 (tok);
		address addr= parseexpr (false, tok, tz);
		checkendline (tz);
		gencode (0xCE);
		byte param= static_cast <byte> (addr);
		gencode (param);
		* pout << "CE " << hex2 (param) <<
			"\tADC A, n" << endl;
	}
	#else
	dobyteparam (tz, 0x88, 0xCE);
	#endif
}

void Asm::In::parseSBC (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	switch (tok.type () )
	{
	case TypeA:
		#if 0
		{
			parsecomma (tz);
			tok= tz.gettoken ();
			unsigned short reg;
			byte prefix;
			bool hasdesp;
			byte desp;
			if (parsebyteparam (tz, tok.type (), reg, prefix,
				hasdesp, desp) )
			{
				checkendline (tz);
				if (prefix)
				{
					gencode (prefix);
					* pout << hex2 (prefix) << ' ';
				}
				byte code= 0x98 + reg;
				gencode (code);
				* pout << hex2 (code);
				if (hasdesp)
				{
					gencode (desp);
					* pout << ' ' << hex2 (desp);
				}
				* pout << endl;
			}
			else
			{
				//address addr= getvalue0 (tok);
				address addr= parseexpr (false, tok, tz);
				checkendline (tz);
				gencode (0xDE);
				byte param= static_cast <byte> (addr);
				gencode (param);
				* pout << "DE " << hex2 (param) <<
					"\tSBC A, n" << endl;
			}
		}
		#else
		parsecomma (tz);
		dobyteparam (tz, 0x98, 0xDE);
		#endif
		break;
	case TypeHL:
		parseADDADCSBCHL (tz, 0, 0x42);
		break;
	default:
		throw "Invalid operand";
	}
}

// Casos
//	push bc -> C5
//	push de -> D5
//	push hl -> E5
//	push af -> F5
//	push ix -> DD E5
//	push iy -> FD E5
//	pop bc -> C1
//	pop de -> D1
//	pop hl -> E1
//	pop af -> F1
//	pop ix -> DD E1
//	pop iy -> FD E1

void Asm::In::parsePUSHPOP (Tokenizer & tz, bool isPUSH)
{
	Token tok= tz.gettoken ();
	byte code= 0;
	byte prefix= 0;

	//* perr << "Tipo: " << tok.type () << endl;
	switch (tok.type () )
	{
	case TypeBC:
		code= regBC;
		break;
	case TypeDE:
		code= regDE;
		break;
	case TypeHL:
		code= regHL;
		break;
	case TypeAF:
		code= regAF;
		break;
	case TypeIX:
		code= regHL;
		prefix= prefixIX;
		break;
	case TypeIY:
		code= regHL;
		prefix= prefixIY;
		break;
	default:
		//throw "Error de sintaxis";
		throw "Unexpected " + tok.str ();
	}
	checkendline (tz);

	//* perr << "Code= " << int (code) << endl;
	code<<= 4;
	code+= isPUSH ? 0xC5 : 0xC1;

	if (prefix)
		gencode (prefix);
	gencode (code);

	if (prefix)
		* pout << hex2 (prefix) << ' ';
	* pout << hex2 (code) << endl;
}

// Codigos JP y CALL

// jp NN     -> C3
// jp (hl)   -> E9
// jp nz, NN -> C2
// jp z,  NN -> CA
// jp nc, NN -> D2
// jp c, NN  -> DA
// jp po, NN -> E2
// jp pe, NN -> EA
// jp p, NN  -> F2
// jp m, NN  -> FA

// call NN     -> CD
// call nz, NN -> C4
// call z, NN  -> CC
// call nc, NN -> D4
// call c, NN  -> DC
// call po, NN -> E4
// call pe, NN -> EC
// call p, NN  -> F4
// call m, NN  -> FC

void Asm::In::parseCALL (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	address addr;
	#if 0
	switch (tok.type () )
	{
	case TypeNumber:
		addr= tok.num ();
		break;
	case TypeIdentifier:
		//addr= getvalue0 (tok.str () );
		addr= parseexpr (false, tok, tz);
		break;
	default:
		//throw "Error de sintaxis";
		throw "Unexpected " + tok.str ();
	}
	#else
	byte code;
	switch (tok.type () )
	{
	case TypeC:
		code= 0xDC; break;
	case TypeM:
		code= 0xFC; break;
	case TypeNC:
		code= 0xD4; break;
	case TypeNZ:
		code= 0xC4; break;
	case TypeP:
		code= 0xF4; break;
	case TypePE:
		code= 0xEC; break;
	case TypePO:
		code= 0xE4; break;
	case TypeZ:
		code= 0xCC; break;
	default:
		code= 0xCD;
	}
	if (code != 0xCD)
	{
		parsecomma (tz);
		tok= tz.gettoken ();
	}
	//addr= getvalue0 (tok);
	addr= parseexpr (false, tok, tz);
	checkendline (tz);
	#endif
	gencode (code);
	gencodeword (addr);
	* pout << hex2 (code) << ' ' << hex4 (addr) << endl;
}

void Asm::In::parseRET (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	// Provisional
	byte code= 0;
	switch (tok.type () )
	{
	case TypeEndLine:
		code= 0xC9;
		break;
	case TypeNZ:
		code= 0xC0;
		break;
	case TypeZ:
		code= 0xC8;
		break;
	case TypeNC:
		code= 0xD0;
		break;
	case TypeC:
		code= 0xD8;
		break;
	case TypePO:
		code= 0xE0;
		break;
	case TypePE:
		code= 0xE8;
		break;
	case TypeP:
		code= 0xF0;
		break;
	case TypeM:
		code= 0xF8;
		break;
	default:
		//throw "Syntax error";
		throw "Expected flag type but '" + tok.str () + "'found";
	}
	if (code != 0xC9)
		checkendline (tz);
	gencode (code);
	* pout << hex2 (code) << "\tRET" << endl;
}

void Asm::In::parseJP (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	byte code= 0;
	switch (tok.type () )
	{
	case TypeNZ:
		code= 0xC2;
		break;
	case TypeZ:
		code= 0xCA;
		break;
	case TypeNC:
		code= 0xD2;
		break;
	case TypeC:
		code= 0xDA;
		break;
	case TypePO:
		code= 0xE2;
		break;
	case TypePE:
		code= 0xEA;
		break;
	case TypeP:
		code= 0xF2;
		break;
	case TypeM:
		code= 0xFA;
		break;
	case TypeOpen:
		tok= tz.gettoken ();
		{
			byte prefix= 0;
			switch (tok.type () )
			{
			case TypeHL:
				break;
			case TypeIX:
				prefix= prefixIX;
				break;
			case TypeIY:
				prefix= prefixIY;
				break;
			default:
				throw "Invalid JP ()";
			}
			parseclose (tz);
			checkendline (tz);
			if (prefix)
			{
				gencode (prefix);
				* pout << hex2 (prefix) << ' ';
			}
			gencode (0xE9);
			* pout << "E9\tJP ()" << endl;
		}
		return;
	default:
		code= 0xC3;
	}
	if (code != 0xC3)
	{
		#if 0
		tok= tz.gettoken ();
		if (tok.type () != TypeComma)
			throw "Esperada coma";
		#else
		parsecomma (tz);
		#endif
		tok= tz.gettoken ();
	}

	address addr= 0;
	switch (tok.type () )
	{
	case TypeNumber:
		addr= tok.num ();
		break;
	case TypeIdentifier:
		//addr= getvalue0 (tok.str () );
		addr= parseexpr (false, tok, tz);
		break;
	default:
		//throw "Error de sintaxis";
		throw "Unexpected " + tok.str ();
	}
	checkendline (tz);

	gencode (code);
	gencodeword (addr);
	* pout << hex2 (code) << ' ' << hex4 (addr) << endl;
}

void Asm::In::parseJR (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	byte code= 0;
	switch (tok.type () )
	{
	case TypeNZ:
		code= 0x20;
		break;
	case TypeZ:
		code= 0x28;
		break;
	case TypeNC:
		code= 0x30;
		break;
	case TypeC:
		code= 0x38;
		break;
	default:
		code= 0x18;
	}
	if (code != 0x18)
	{
		#if 0
		tok= tz.gettoken ();
		if (tok.type () != TypeComma)
			throw "Esperada coma";
		#else
		parsecomma (tz);
		#endif
		tok= tz.gettoken ();
	}
	//bool defined= true;
	#if 0
	address addr= 0;
	switch (tok.type () )
	{
	case TypeNumber:
		addr= tok.num ();
		break;
	case TypeIdentifier:
		//addr= getvalue0 (tok.str () );
		addr= parseexpr (false, tok, tz);
		break;
	default:
		//throw "Error de sintaxis";
		throw "Unexpected " + tok.str ();
	}
	#else
	address addr= parseexpr (false, tok, tz);
	#endif
	int dif= 0;
	//if (defined)
	if (pass >= 2)
	{
		dif= addr - (current + 2);
		if (dif > 127 || dif < -128)
		{
			* pout << "addr= " << addr <<
				" current= " << current <<
				" dif= " << dif << endl;
			throw "JR out of range";
		}
	}
	gencode (code);
	signed char c= static_cast <signed char> (dif);
	gencode (c);
	* pout << hex2 (code) << ' ' <<
		hex2 (int (c) & 0xFF) << endl;
}

void Asm::In::parseDJNZ (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	//bool defined= true;
	#if 0
	address addr= 0;
	switch (tok.type () )
	{
	case TypeNumber:
		addr= tok.num ();
		break;
	case TypeIdentifier:
		#if 0
		defined= valuedefined (tok.str () );
		if (defined)
			addr= getvalue (tok.str () );
		else
			throw "Forward JR unimplemnted yet";
		#else
		addr= getvalue0 (tok.str () );
		#endif
		break;
	default:
		//throw "Error de sintaxis";
		throw "Unexpected " + tok.str ();
	}
	#else
	address addr= parseexpr (false, tok, tz);
	#endif
	int dif= 0;
	//if (defined)
	if (pass >= 2)
	{
		dif= addr - (current + 2);
		if (dif > 127 || dif < -128)
			throw "JR out of range";
	}
	gencode (0x10);
	signed char c= static_cast <signed char> (dif);
	gencode (c);
	* pout << "10 " << hex2 (int (c) & 0xFF) << endl;
}

void Asm::In::parseINCDEC (Tokenizer & tz, bool isINC)
{
	Token tok= tz.gettoken ();
	byte code;
	byte prefix= 0;
	bool hasdesp= false;
	byte desp;
	switch (tok.type () )
	{
	case TypeA:
		code= isINC ? 0x3C : 0x3D;
		break;
	case TypeB:
		code= isINC ? 0x04 : 0x05;
		break;
	case TypeC:
		code= isINC ? 0x0C : 0x0D;
		break;
	case TypeD:
		code= isINC ? 0x14 : 0x15;
		break;
	case TypeE:
		code= isINC ? 0x1C : 0x1D;
		break;
	case TypeH:
		code= isINC ? 0x24 : 0x25;
		break;
	case TypeL:
		code= isINC ? 0x2C : 0x2D;
		break;
	case TypeIXH:
		code= isINC ? 0x24 : 0x25;
		prefix= prefixIX;
		break;
	case TypeIXL:
		code= isINC ? 0x2C : 0x2D;
		prefix= prefixIX;
		break;
	case TypeIYH:
		code= isINC ? 0x24 : 0x25;
		prefix= prefixIY;
		break;
	case TypeIYL:
		code= isINC ? 0x2C : 0x2D;
		prefix= prefixIY;
		break;
	case TypeBC:
		code= isINC ? 0x03 : 0x0B;
		break;
	case TypeDE:
		code= isINC ? 0x13 : 0x1B;
		break;
	case TypeHL:
		code= isINC ? 0x23 : 0x2B;
		break;
	case TypeIX:
		code= isINC ? 0x23 : 0x2B;
		prefix= prefixIX;
		break;
	case TypeIY:
		code= isINC ? 0x23 : 0x2B;
		prefix= prefixIY;
		break;
	case TypeSP:
		code= isINC ? 0x33 : 0x3B;
		break;
	case TypeOpen:
		tok= tz.gettoken ();
		switch (tok.type () )
		{
		case TypeHL:
			parseclose (tz);
			code= isINC ? 0x34 : 0x35;
			break;
		case TypeIX:
			code= isINC ? 0x34 : 0x35;
			prefix= prefixIX;
			hasdesp= true;
			parsedesp (tz, desp);
			break;
		case TypeIY:
			code= isINC ? 0x34 : 0x35;
			prefix= prefixIY;
			hasdesp= true;
			parsedesp (tz, desp);
			break;
		default:
			throw "Invalid operand " + tok.str ();
		}
		break;
	default:
		throw "Invalid operand " + tok.str ();
	}
	checkendline (tz);

	if (prefix != 0)
		gencode (prefix);
	gencode (code);
	if (hasdesp)
		gencode (desp);

	if (prefix != 0)
		* pout << hex2 (prefix) <<  ' ';
	* pout << hex2 (code);
	if (hasdesp)
		* pout << ' ' << hex2 (desp);
	* pout << '\t' <<
		(isINC ? "INC" : "DEC") << endl;
}

void Asm::In::parseEX (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	switch (tok.type () )
	{
	case TypeAF:
		parsecomma (tz);
		tok= tz.gettoken ();
		if (tok.type () != TypeAFp)
			throw "Invalid operand for EX";
		gencode (0x08);
		* pout << "08\tEX AF, AF'" << endl;
		break;
	case TypeDE:
		parsecomma (tz);
		tok= tz.gettoken ();
		if (tok.type () != TypeHL)
			throw "Invalid operand for EX";
		gencode (0xEB);
		* pout << "EB\tEX DE, HL" << endl;
		break;
	case TypeOpen:
		tok= tz.gettoken ();
		if (tok.type () != TypeSP)
			throw "Invalid operand for EX";
		parseclose (tz);
		parsecomma (tz);
		tok= tz.gettoken ();
		switch (tok.type () )
		{
		case TypeHL:
			gencode (0xE3);
			* pout << "E3\tEX (SP), HL" << endl;
			break;
		case TypeIX:
			gencode (prefixIX);
			gencode (0xE3);
			* pout << "DD E3\tEX (SP), IX" << endl;
			break;
		case TypeIY:
			gencode (prefixIY);
			gencode (0xE3);
			* pout << "FD E3\tEX (SP), IY" << endl;
			break;
		default:
			throw "Invalid operand for EX";
		}
		break;
	default:
		throw "Invalid operand for EX";
	}
	checkendline (tz);
}

void Asm::In::parseIN (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	byte code;
	switch (tok.type () )
	{
	case TypeB:
		code= 0x40; break;
	case TypeC:
		code= 0x48; break;
	case TypeD:
		code= 0x50; break;
	case TypeE:
		code= 0x58; break;
	case TypeH:
		code= 0x60; break;
	case TypeL:
		code= 0x68; break;
	case TypeA:
		parsecomma (tz);
		parseopen (tz);
		tok= tz.gettoken ();
		if (tok.type () == TypeC)
		{
			parseclose (tz);
			gencode (0xED);
			gencode (0x78);
			* pout << "ED 78\tIN A, (C)" << endl;
		}
		else
		{
			//address addr= getvalue0 (tok);
			address addr= parseexpr (false, tok, tz);
			byte b= static_cast <byte> (addr);
			gencode (0xDB);
			gencode (b);
			* pout << "DB " << hex2 (b) <<
				"\tIN A, (n)" << endl;
			parseclose (tz);
		}
		checkendline (tz);
		return;
	default:
		throw "Invalid operand";
	}
	parsecomma (tz);
	parseopen (tz);
	parseC (tz);
	parseclose (tz);
	checkendline (tz);
	gencode (0xED);
	gencode (code);
	* pout << "ED " << hex2 (code) << endl;
}

void Asm::In::parseOUT (Tokenizer & tz)
{
	parseopen (tz);
	Token tok= tz.gettoken ();
	if (tok.type () != TypeC)
	{
		//address addr= getvalue0 (tok);
		address addr= parseexpr (false, tok, tz);
		byte b= static_cast <byte> (addr);
		parseclose (tz);
		parsecomma (tz);
		parseA (tz);
		checkendline (tz);
		gencode (0xD3);
		gencode (b);
		* pout << "D3 " << hex2 (b) <<
			"\tOUT (n), A" << endl;
		return;
	}
	parseclose (tz);
	parsecomma (tz);
	tok= tz.gettoken ();
	byte code;
	switch (tok.type () )
	{
	case TypeA:
		code= 0x79; break;
	case TypeB:
		code= 0x41; break;
	case TypeC:
		code= 0x49; break;
	case TypeD:
		code= 0x51; break;
	case TypeE:
		code= 0x59; break;
	case TypeH:
		code= 0x61; break;
	case TypeL:
		code= 0x69; break;
	default:
		throw "Invalid operand";
	}
	checkendline (tz);
	gencode (0xED);
	gencode (code);
	* pout << "ED " << hex2 (code) << endl;
}

void Asm::In::dobit (Tokenizer & tz, byte basecode)
{
	Token tok= tz.gettoken ();
	//address addr= getvalue0 (tok);
	address addr= parseexpr (false, tok, tz);
	if (addr > 7)
		throw "Bit position out of range";
	parsecomma (tz);
	dobyteparamCB (tz, basecode + (addr << 3) );
}

void Asm::In::parseBIT (Tokenizer & tz)
{
	dobit (tz, 0x40);
}

void Asm::In::parseRES (Tokenizer & tz)
{
	dobit (tz, 0x80);
}

void Asm::In::parseSET (Tokenizer & tz)
{
	dobit (tz, 0xC0);
}

void Asm::In::parseDEFB (Tokenizer & tz)
{
	for (;;)
	{
		Token tok= tz.gettoken ();
		switch (tok.type () )
		{
		//case TypeNumber:
		//	gencode (tok.num () & 0xFF);
		//	break;
		//case TypeIdentifier:
		//	gencode (getvalue0 (tok.str () ) & 0xFF);
		//	break;
		case TypeLiteral:
			{
				const std::string & str= tok.str ();
				const std::string::size_type l= str.size ();
				if (l == 1)
				{
					// Admit expressions like 'E' + 80H
					gencode (parseexpr (false, tok, tz) );
					break;
				}
				for (std::string::size_type i= 0; i < l; ++i)
				{
					gencode (str [i] );
				}
			}
			break;
		default:
			gencode (parseexpr (false, tok, tz) );
		}
		tok= tz.gettoken ();
		if (tok.type () == TypeEndLine)
			break;
		if (tok.type () != TypeComma)
			//throw "Error de sintaxis";
			throw "Unexpected " + tok.str ();
	}
}

void Asm::In::parseDEFW (Tokenizer & tz)
{
	for (;;)
	{
		Token tok= tz.gettoken ();
		//gencodeword (getvalue0 (tok) );
		gencodeword (parseexpr (false, tok, tz) );
		tok= tz.gettoken ();
		if (tok.type () == TypeEndLine)
			break;
		if (tok.type () != TypeComma)
			//throw "Error de sintaxis";
			throw "Unexpected " + tok.str ();
	}
}

void Asm::In::parseDEFS (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	//address addr= getvalue (tok);
	address addr= parseexpr (true, tok, tz);
	byte value= 0;
	tok= tz.gettoken ();
	switch (tok.type () )
	{
	case TypeEndLine:
		// Nothing to do
		break;
	case TypeComma:
		tok= tz.gettoken ();
		{
			//address addr= getvalue0 (tok);
			address addr= parseexpr (false, tok, tz);
			checkendline (tz);
			value= static_cast <byte> (addr);
		}
		break;
	default:
		//throw "Syntax error";
		throw "Unexpected " + tok.str ();
	}
	for (address i= 0; i < addr; ++i)
		gencode (value);
}

void Asm::In::parseINCBIN (Tokenizer & tz)
{
	std::string includefile= tz.getincludefile ();
	checkendline (tz);
	* pout << "\tINCBIN " << includefile << endl;

	// Changed this to introduce the -I option.
	#if 0
	std::ifstream f (includefile.c_str (), std::ios::in | std::ios::binary);
	if (! f.is_open () )
		throw "File not found";
	#else
	std::ifstream f;
	openis (f, includefile, std::ios::in | std::ios::binary);
	#endif

	char buffer [1024];
	for (;;)
	{
		f.read (buffer, sizeof (buffer) );
		for (std::streamsize i= 0, r= f.gcount (); i < r; ++i)
			gencode (static_cast <byte> (buffer [i] ) );
		if (! f)
		{
			if (f.eof () )
				break;
			else
				throw "Error in INCBIN reading \"" + 
					includefile + "\" file";
		}
	}
}

namespace {

typedef std::vector <Token> MacroParam;
typedef std::vector <MacroParam> MacroParamList;

void getmacroparams (MacroParamList & params, Tokenizer & tz)
{
	for (;;)
	{
		Token tok= tz.gettoken ();
		TypeToken tt= tok.type ();
		if (tt == TypeEndLine)
			break;
		MacroParam param;
		while (tt != TypeEndLine && tt != TypeComma)
		{
			param.push_back (tok);
			tok= tz.gettoken ();
			tt= tok.type ();
		}
		params.push_back (param);
		if (tt == TypeEndLine)
			break;
	}
}

Tokenizer substmacroparams (MacroBase & macro, Tokenizer & tz,
	const MacroParamList & params)
{
	//if (params.empty () )
	//	return tz;

	Tokenizer r (tz.getnocase () );
	for (;;)
	{
		Token tok= tz.gettoken ();
		TypeToken tt= tok.type ();
		if (tt == TypeEndLine)
			break;
		if (tt != TypeIdentifier)
			r.push_back (tok);
		else
		{
			const std::string & name= tok.str ();
			size_t n= macro.getparam (name);
			if (n == Macro::noparam)
				r.push_back (tok);
			else
			{
				// If there are no sufficient parameters
				// expand to nothing.
				if (n < params.size () )
				{
					const MacroParam & param= params [n];
					for (size_t i= 0; i < param.size ();
						++i)
					{
						r.push_back (param [i] );
					}
				}
			}
		}
	}
	return r;
}

} // namespace

bool Asm::In::gotoENDM ()
{
	size_t level= 1;
	for (; ; ++currentline)
	{
		getvalidline ();
		if (currentline >= vline.size () )
			return false;
		const std::string & line= vline [currentline];
		Tokenizer tz (line, nocase);
		Token tok= tz.gettoken ();
		TypeToken tt= tok.type ();
		if (tt == TypeIdentifier)
		{
			tok= tz.gettoken ();
			tt= tok.type ();
		}
		if (tt == TypeENDM)
		{
			if (--level == 0)
				break;
		}
		if (tt == TypeMACRO || tt == TypeREPT || tt == TypeIRP)
			++level;
	}
	return true;
}

void Asm::In::expandMACRO (Macro macro, Tokenizer & tz)
{
	// Get parameters.
	MacroParamList params;
	#if 0
	for (;;)
	{
		Token tok= tz.gettoken ();
		TypeToken tt= tok.type ();
		if (tt == TypeEndLine)
			break;
		MacroParam param;
		while (tt != TypeEndLine && tt != TypeComma)
		{
			param.push_back (tok);
			tok= tz.gettoken ();
			tt= tok.type ();
		}
		params.push_back (param);
		if (tt == TypeEndLine)
			break;
	}
	#else
	getmacroparams (params, tz);
	#endif
	checkendline (tz); // Redundant, for debugging.

	for (size_t i= 0; i < params.size (); ++i)
	{
		* pout << macro.getparam (i) << "= ";
		const MacroParam & p= params [i];
		std::copy (p.begin (), p.end (),
			std::ostream_iterator <Token> (* pout, " ") );
		* pout << endl;
	}

	// Set the local frame.
	MacroLevel * pproc= new MacroLevel (* this);
	localstack.push (pproc);

	// Do the expansion,
	size_t expandline= currentline;
	size_t previflevel= iflevel;
	try
	{
		for (currentline= macro.getline () + 1;
			currentline < vline.size ();
			++currentline)
		{
			getvalidline ();
			if (currentline >= vline.size () )
				break;
			const std::string & line= vline [currentline];
			Tokenizer tz (line, nocase);
			Token tok= tz.gettoken ();
			TypeToken tt= tok.type ();
			if (tt == TypeENDM || tt == TypeEXITM)
				break;
			//tz.putback (tok);
			tz.ungettoken ();
			Tokenizer tzsubst
				(substmacroparams (macro, tz, params) );
			* pout << tzsubst << endl;
			parseline (tzsubst);
		}
		if (currentline >= vline.size () )
			throw "Unexpected unclosed MACRO";
	}
	catch (...)
	{
		LineInfo & linf= vlineinfo [expandline];
		* perr << "ERROR expanding macro on line " << linf.linenum <<
			" of file " << vfileref [linf.filenum].filename <<
			endl;
		throw;
	}

	// Clear the local frame, including unclosed PROCs.
	while (dynamic_cast <MacroLevel *> (localstack.top () ) == NULL)
		localstack.pop ();
	localstack.pop ();

	// IF whitout ENDIF inside a macro are valid.
	iflevel= previflevel;

	currentline= expandline;
}

void Asm::In::parseREPT (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	address numrep= parseexpr (true, tok, tz);
	checkendline (tz);

	* pout << "\tREPT " << numrep << endl;
	
	// Set the local frame.
	MacroLevel * pproc= new MacroLevel (* this);
	localstack.push (pproc);
	size_t initline= currentline;
	size_t previflevel= iflevel;

	for (address i= 0; i < numrep; ++i)
	{
		bool exited= false;
		for (currentline= initline + 1;
			currentline < vline.size ();
			++currentline)
		{
			getvalidline ();
			if (currentline >= vline.size () )
				break;
			const std::string & line= vline [currentline];
			Tokenizer tz (line, nocase);
			tok= tz.gettoken ();
			TypeToken tt= tok.type ();
			if (tt == TypeENDM)
			{
				* pout << "\tENDM" << endl;
				break;
			}
			if (tt == TypeEXITM)
			{
				* pout << "\tEXITM" << endl;
				if (! gotoENDM () )
					throw "REPT without ENDM";
				exited= true;
				break;
			}
			//tz.putback (tok);
			tz.ungettoken ();
			* pout << tz << endl;
			parseline (tz);
		}
		if (currentline >= vline.size () )
		{
			currentline= initline;
			throw "REPT without ENDM";
		}
		if (exited)
			break;
	}
	// Clear the local frame, including unclosed PROCs.
	while (dynamic_cast <MacroLevel *> (localstack.top () ) == NULL)
		localstack.pop ();
	localstack.pop ();

	// IF whitout ENDIF inside a macro are valid.
	iflevel= previflevel;	
}

void Asm::In::parseIRP (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	if (tok.type () != TypeIdentifier)
		throw "IRP param not valid: " + tok.str ();
	const std::string arg= tok.str ();
	MacroIrp macroirp (arg);

	tok= tz.gettoken ();
	if (tok.type () != TypeComma)
		throw "Comma expected after IRP param, found: " + tok.str ();
	MacroParamList params;
	getmacroparams (params, tz);
	checkendline (tz); // Redundant, for debugging.
	if (params.empty () )
		throw "IRP without parameters";

	* pout << "IRP" << endl;

	// Set the local frame.
	MacroLevel * pproc= new MacroLevel (* this);
	localstack.push (pproc);

	size_t initline= currentline;
	size_t previflevel= iflevel;

	MacroParamList actualparam (1);
	for (size_t irpn= 0; irpn < params.size (); ++irpn)
	{
		bool exited= false;
		actualparam [0]= params [irpn];
		for (currentline= initline + 1;
			currentline < vline.size ();
			++currentline)
		{
			getvalidline ();
			if (currentline >= vline.size () )
				break;
			const std::string & line= vline [currentline];
			Tokenizer tz (line, nocase);
			tok= tz.gettoken ();
			TypeToken tt= tok.type ();
			if (tt == TypeENDM)
			{
				* pout << "\tENDM" << endl;
				break;
			}
			if (tt == TypeEXITM)
			{
				* pout << "\tEXITM" << endl;
				if (! gotoENDM () )
					throw "IRP without ENDM";
				exited= true;
				break;
			}
			//tz.putback (tok);
			tz.ungettoken ();
			Tokenizer tzsubst (substmacroparams
				(macroirp, tz, actualparam) );
			* pout << tzsubst << endl;
			parseline (tzsubst);
		}
		
		if (currentline >= vline.size () )
		{
			currentline= initline;
			throw "IRP without ENDM";
		}
		if (exited)
			break;
	}

	// Clear the local frame, including unclosed PROCs.
	while (dynamic_cast <MacroLevel *> (localstack.top () ) == NULL)
		localstack.pop ();
	localstack.pop ();

	// IF whitout ENDIF inside a macro are valid.
	iflevel= previflevel;	
}

void Asm::In::emitobject (std::ostream & out)
{
	if (debugtype != NoDebug)
		* pout << "Emiting raw binary from " << hex4 (minused) <<
			" to " << hex4 (maxused) << endl;

	for (int i= minused; i <= maxused; ++i)
	{
		out.put (mem [i] );
	}
}

void Asm::In::emitplus3dos (std::ostream & out)
{
	if (debugtype != NoDebug)
		* pout << "Emiting PLUS3DOS from " << hex4 (minused) <<
			" to " << hex4 (maxused) << endl;

	// PLUS3DOS haeader.

	byte plus3 [128]= {
		'P', 'L', 'U', 'S', '3', 'D', 'O', 'S', // Identifier.
		0x1A, // CP/M EOF.
		1,    // Issue number.
		0,    // Version number.
	};
	address codesize= maxused - minused + 1;
	size_t filesize= codesize + 128;
	// Size of file including code and header.
	plus3 [11]= filesize & 0xFF;
	plus3 [12]= (filesize >> 8) & 0xFF;
	plus3 [13]= (filesize >> 16) & 0xFF;
	plus3 [14]= (filesize >> 24) & 0xFF;
	plus3 [15]= 3; // Type: code.
	// Size of code.
	plus3 [16]= codesize & 0xFF;
	plus3 [17]= (codesize >> 8) & 0xFF;
	// Start address.
	plus3 [18]= minused & 0xFF;
	plus3 [19]= (minused >> 8) & 0xFF;
	// Don't know if used for something.
	plus3 [20]= 0x80;
	plus3 [21]= 0x80;
	// Checksum
	byte check= 0;
	for (int i= 0; i < 127; ++i)
		check+= plus3 [i];
	plus3 [127]= check;

	// Write the file.

	out.write (reinterpret_cast <char *> (plus3), sizeof (plus3) );
	out.write (reinterpret_cast <char *> (mem + minused), codesize);
	size_t round= 128 - (codesize % 128);
	if (round != 128)
	{
		char aux [128]= {};
		out.write (aux, round);
	}

	if (! out)
		throw "Error writing";
}

void Asm::In::emittap (std::ostream & out, const std::string & filename)
{
	if (debugtype != NoDebug)
		* pout << "Emiting TAP from " << hex4 (minused) <<
			" to " << hex4 (maxused) << endl;

	address codesize= maxused - minused + 1;

	// Pepare data needed.

	// Block 1: code header.

	byte block1 [21]= {
		0x13, 0x00, // Length of block: 17 bytes + flag + checksum.
		0x00,       // Flag: 00 -> header
		0x03,       // Type: code block.
	};
	std::string::size_type l= filename.size ();
	if (l > 10)
		l= 10;
	for (std::string::size_type i= 0; i < 10; ++i)
		block1 [4 + i]= i < l ? filename [i] : ' ';
	// Length of the code block.
	block1 [14]= codesize & 0xFF;
	block1 [15]= codesize >> 8;
	// Start of the code block.
	block1 [16]= minused & 0xFF;
	block1 [17]= minused >> 8;
	// Parameter 2: 32768 in a code block.
	block1 [18]= 0x00;
	block1 [19]= 0x80;
	// Checksum
	byte check= block1 [2]; // Flag byte included.
	for (int i= 3; i < 20; ++i)
		check^= block1 [i];
	block1 [20]= check;

	// Block 2: code data.

	// Initialise the header of block.
	byte headblock2 [3]= { };
	address blocksize= codesize + 2; // Code + flag + checksum.
	headblock2 [0]= blocksize & 0xFF;
	headblock2 [1]= blocksize >> 8;
	headblock2 [2]= 0xFF; // Flag: data block.
	// Compute the checksum.
	check= 0xFF; // Flag byte included.
	// BUG FIX.
	//for (int i= minused; i < static_cast <int> (maxused) + codesize; ++i)
	for (int i= minused; i <= static_cast <int> (maxused); ++i)
		check^= mem [i];

	// Write the file.

	// Write block 1.
	out.write (reinterpret_cast <char *> (block1), sizeof (block1) );
	// Write header of block 2.
	out.write (reinterpret_cast <char *> (headblock2),
		sizeof (headblock2) );
	// Write content of block 2.
	out.write (reinterpret_cast <char *> (mem + minused), codesize);
	// Write checksum of block 2.
	out.write (reinterpret_cast <char *> (& check), 1);

	if (! out)
		throw "Error writing";
}

namespace {

std::string spectrum_number (address n)
{
	std::string str (1, '\x0E');
	str+= '\x00';
	str+= '\x00';
	str+= static_cast <unsigned char> (n & 0xFF);
	str+= static_cast <unsigned char> (n >> 8);
	str+= '\x00';
	return str;
}

} // namespace

void Asm::In::emittapbas (std::ostream & out, const std::string & filename)
{
	if (debugtype != NoDebug)
		* pout << "Emiting TAP basic loader" << endl;

	address clearpos= minused - 1;
	std::string line= "\xFD"; // CLEAR.
	line+= to_string (clearpos);
	line+= spectrum_number (clearpos);
	line+= '\x0D';
	address len= static_cast <address> (line.size () );

	// Line number 10
	std::string basic (1, '\x00');
	basic+= '\x0A';
	// Length of line.
	basic+= static_cast <unsigned char> (len & 0xFF);
	basic+= static_cast <unsigned char> (len >> 8);
	// Body of line
	basic+= line;

	// Test: line 15: POKE 23610, 255
	// To avoid a error message when using +3 loader.
	line= "\xF4" "23610";
	line+= spectrum_number (23610);
	line+=",255";
	line+= spectrum_number (255);
	line+= '\x0D';
	basic+= '\x00';
	basic+= '\x0F';
	len= static_cast <address> (line.size () );
	basic+= static_cast <unsigned char> (len & 0xFF);
	basic+= static_cast <unsigned char> (len >> 8);
	basic+= line;

	// Line 20: LOAD "" CODE
	basic+= '\x00';
	basic+= "\x14\x05";
	basic+= '\x00';
	basic+= "\xEF\"\"\xAF\x0D";

	if (hasentrypoint)
	{
		// RANDOMIZE USR.
		line= "\xF9\xC0";
		line+= to_string (entrypoint);
		line+= spectrum_number (entrypoint);
		line+= '\x0D';
		len= static_cast <address> (line.size () );
		// Line number 30
		basic+= '\x00';
		basic+= '\x1E';
		// Length of line.
		basic+= static_cast <unsigned char> (len & 0xFF);
		basic+= static_cast <unsigned char> (len >> 8);
		// Body of line
		basic+= line;
	}

	// Block 1: Basic header

	byte block1 [21]= {
		0x13, 0x00, // Length of block: 17 bytes + flag + checksum.
		0x00,       // Flag: 00 -> header
		0x00,       // Type: Basic block.
	};
	for (int i= 0; i < 10; ++i)
		block1 [4 + i]= "loader    " [i];
	// Length of the basic block.
	address basicsize= static_cast <address> (basic.size () );
	block1 [14]= basicsize & 0xFF;
	block1 [15]= basicsize >> 8;
	// Autostart in line 10.
	block1 [16]= '\x0A';
	block1 [17]= '\x00';
	// Start of variable area: at the end.
	block1 [18]= block1 [14];
	block1 [19]= block1 [15];
	// Checksum
	byte check= block1 [2]; // Flag byte included.
	for (int i= 3; i < 20; ++i)
		check^= block1 [i];
	block1 [20]= check;

	// Block 2: basic.

	// Initialise the header of block.
	byte headblock2 [3]= { };
	address blocksize= basicsize + 2; // Code + flag + checksum.
	headblock2 [0]= blocksize & 0xFF;
	headblock2 [1]= blocksize >> 8;
	headblock2 [2]= 0xFF; // Flag: data block.
	// Compute the checksum.
	check= 0xFF; // Flag byte included.
	for (int i= 0; i < basicsize; ++i)
		check^= static_cast <unsigned char> (basic [i]);

	// Write the file.

	// Write block 1.
	out.write (reinterpret_cast <char *> (block1), sizeof (block1) );
	// Write header of block 2.
	out.write (reinterpret_cast <char *> (headblock2),
		sizeof (headblock2) );
	// Write content of block 2.
	for (int i= 0; i < basicsize; ++i)
		out.put (basic [i] );
	// Write checksum of block 2.
	out.write (reinterpret_cast <char *> (& check), 1);

	emittap (out, filename);
}

void Asm::In::emithex (std::ostream & out)
{
	if (debugtype != NoDebug)
		* pout << "Emiting Intel HEX from " << hex4 (minused) <<
			" to " << hex4 (maxused) << endl;

	address end= maxused + 1;
	for (address i= minused; i < end; i+= 16)
	{
		address len= end - i;
		if (len > 16)
			len= 16;
		out << ':' << hex2 (len) << hex4 (i) << "00";
		byte sum= len + ( (i >> 8) & 0xFF) + i & 0xFF;
		for (address j= 0; j < len; ++j)
		{
			byte b= mem [i + j];
			out << hex2 (b);
			sum+= b;
		}
		out << hex2 (0x100 - sum);
		out << "\r\n";
	}
	out << ":00000001FF\r\n";

	if (! out)
		throw "Error writing";
}

void Asm::In::emitamsdos (std::ostream & out, const std::string & filename)
{
	if (debugtype != NoDebug)
		* pout << "Emiting Amsdos from " << hex4 (minused) <<
			" to " << hex4 (maxused) << endl;

	byte amsdos [128]= { 0 };

	address codesize= maxused - minused + 1;
	address len= codesize;

	// 00: User number, use 0.
	// 01-0F: filename, padded with 0.
	std::string::size_type l= filename.size ();
	if (l > 15)
		l= 15;
	for (std::string::size_type i= 0; i < l; ++i)
		amsdos [i + 1]= filename [i];
	for (std::string::size_type i= l; i < 15; ++i)
		amsdos [i + 1]= '\0';
	// 10: Block number, 0 in disk files.
	// 11: last block flag, 0 in disk files.
	amsdos [0x12]= 2; // File type: binary.
	// 13-14 // Length of block, 0 in disk files
	// 15-16: Load address.
	amsdos [0x15]= minused & 0xFF;
	amsdos [0x16]= minused >> 8;
	// 17: first block flag, 0 in disk files.
	// 18-19: logical length.
	amsdos [0x18]= len & 0xFF;
	amsdos [0x19]= len >> 8;
	// 1A-1B: Entry address.
	address entry= 0;
	if (hasentrypoint)
		entry= entrypoint;
	amsdos [0x1A]= entry & 0xFF;
	amsdos [0x1B]= entry >> 8;
	// 1C-3F: Unused.
	// 40-42: real length of file.
	amsdos [0x40]= len & 0xFF;
	amsdos [0x41]= (len >> 8) & 0xFF;
	amsdos [0x42]= 0;
	// 43-44 checksum of bytes 00-42
	address check= 0;
	for (int i= 0; i < 0x43; ++i)
		check+= amsdos [i];
	amsdos [0x43]= check & 0xFF;
	amsdos [0x44]= check >> 8;
	// 45-7F: Unused.

	out.write (reinterpret_cast <char *> (amsdos), sizeof (amsdos) );
	out.write (reinterpret_cast <char *> (mem + minused), codesize);

	if (! out)
		throw "Error writing";
}

void Asm::In::emitmsx (std::ostream & out)
{
	if (debugtype != NoDebug)
		* pout << "Emiting MSX from " << hex4 (minused) <<
			" to " << hex4 (maxused) << endl;

	// Header of an MSX BLOADable disk file.
	byte header [7]= { 0xFE }; // Header identification byte.
	// Start address.
	header [1]= minused & 0xFF;
	header [2]= minused >> 8;
	// End address.
	header [3]= maxused & 0xFF;
	header [4]= maxused >> 8;
	// Exec address.
	address entry= 0;
	if (hasentrypoint)
		entry= entrypoint;
	header [5]= entry & 0xFF;
	header [6]= entry >> 8;	

	// Write hader.
	out.write (reinterpret_cast <char *> (header), sizeof (header) );

	// Write code.
	address codesize= maxused - minused + 1;
	out.write (reinterpret_cast <char *> (mem + minused), codesize);
}

void Asm::In::emitprl (std::ostream & out, Asm & asmmainoff)
{
	if (debugtype != NoDebug)
		* pout << "Emiting PRL from " << hex4 (minused) <<
			" to " << hex4 (maxused) << endl;

	static const std::string
		nosync ("PRL genration failed: out of sync");
	In & asmoff= * asmmainoff.pin;

	if (minused - base != asmoff.minused - asmoff.base)
		throw nosync;
	if (maxused - base != asmoff.maxused - asmoff.base)
		throw nosync;
	address len= maxused - minused + 1;
	address off= asmoff.base - base;

	// PRL header.

	byte prlhead [256]= { 0 };
	prlhead [1]= len & 0xFF;
	prlhead [2]= len >> 8;
	out.write (reinterpret_cast <char *> (prlhead), sizeof (prlhead) );
	address reloclen= (len + 7) / 8;
	byte * reloc= new byte [reloclen];
	memset (reloc, 0, reloclen);

	// Build relocation bitmap.
	for (address i= minused; i <= maxused; ++i)
	{
		byte b= mem [i];
		byte b2= asmoff.mem [i + off];
		if (b != b2)
		{
			if (b2 - b != off / 256)
			{
				* perr << "off= " << hex4 (off) <<
					", b= " << hex2 (b) <<
					", b2= " << hex2 (b2) <<
					endl;
				throw nosync;
			}
			address pos= i - minused;
			static const byte mask [8]= {
				0x80, 0x40, 0x20, 0x10,
				0x08, 0x04, 0x02, 0x01
			};
			reloc [pos / 8]|= mask [pos % 8];
		}
	}

	// Write code in position 0x0100
	out.write (reinterpret_cast <char *> (asmoff.mem + minused + off),
		len);
	// Write relocation bitmap.
	out.write (reinterpret_cast <char *> (reloc), reloclen);

	if (! out)
		throw "Error writing";
}

void Asm::In::dumppublic (std::ostream & out)
{
	for (setpublic_t::iterator pit= setpublic.begin ();
		pit != setpublic.end ();
		++pit)
	{
		mapvar_t::iterator it= mapvar.find (* pit);
		if (it != mapvar.end () )
		{
			out << it->first << "\tEQU 0" <<
				hex4 (it->second) << 'H' << endl;
		}
	}
}

void Asm::In::dumpsymbol (std::ostream & out)
{
	for (mapvar_t::iterator it= mapvar.begin ();
		it != mapvar.end ();
		++it)
	{
		// Dump only EQU and label valid symbols.
		if (vardefined [it->first] != DefinedPass2)
			continue;
		out << it->first << "\tEQU 0" << hex4 (it->second) << 'H'
			<< endl;
	}
}

//*********************************************************
//			class Asm
//*********************************************************

Asm::Asm () :
	pin (new In)
{
}

Asm::Asm (const Asm & a) :
	pin (new In (* a.pin) )
{
}

Asm::~Asm ()
{
	delete pin;
}

void Asm::setdebugtype (DebugType type)
{
	pin->setdebugtype (type);
}

void Asm::errtostdout ()
{
	pin->errtostdout ();
}

void Asm::setbase (unsigned int addr)
{
	pin->setbase (addr);
}

void Asm::caseinsensitive ()
{
	pin->caseinsensitive ();
}

void Asm::autolocal ()
{
	pin->autolocal ();
}

void Asm::addincludedir (const std::string & dirname)
{
	pin->addincludedir (dirname);
}

void Asm::processfile (const std::string & filename)
{
	pin->processfile (filename);
}

void Asm::emitobject (std::ostream & out)
{
	pin->emitobject (out);
}

void Asm::emitplus3dos (std::ostream & out)
{
	pin->emitplus3dos (out);
}

void Asm::emittap (std::ostream & out, const std::string & filename)
{
	pin->emittap (out, filename);
}

void Asm::emittapbas (std::ostream & out, const std::string & filename)
{
	pin->emittapbas (out, filename);
}

void Asm::emithex (std::ostream & out)
{
	pin->emithex (out);
}

void Asm::emitamsdos (std::ostream & out, const std::string & filename)
{
	pin->emitamsdos (out, filename);
}

void Asm::emitprl (std::ostream & out, Asm & asmoff)
{
	pin->emitprl (out, asmoff);
}

void Asm::emitmsx (std::ostream & out)
{
	pin->emitmsx (out);
}

void Asm::dumppublic (std::ostream & out)
{
	pin->dumppublic (out);
}

void Asm::dumpsymbol (std::ostream & out)
{
	pin->dumpsymbol (out);
}

// End of asm.cpp
