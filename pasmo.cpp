// pasmo.cpp
// Revision 7-apr-2004

#include "asm.h"

#include <fstream>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

namespace {

class Usage { };

enum ObjectFormat {
	ObjectRaw, ObjectHex, ObjectPrl,
	ObjectPlus3Dos, ObjectTap, ObjectTapBas,
	ObjectAmsdos, ObjectMsx
};

std::ostream * perr= & std::cerr;

int doit (int argc, char * * argv)
{
	Asm a;

	// Set options.

	ObjectFormat format= ObjectRaw;
	bool emitpublic= false;
	int argpos= 1;
	std::string headername;
	Asm::DebugType debugtype= Asm::NoDebug;
	bool redirecterr= false;
	bool nocase= false;
	for ( ; argpos < argc; ++argpos)
	{
		const std::string arg (argv [argpos] );
		if (arg == "--hex")
			format= ObjectHex;
		else if (arg == "--prl")
			format= ObjectPrl;
		else if (arg == "--plus3dos")
			format= ObjectPlus3Dos;
		else if (arg == "--tap")
			format= ObjectTap;
		else if (arg == "--tapbas")
			format= ObjectTapBas;
		else if (arg == "--amsdos")
			format= ObjectAmsdos;
		else if (arg == "--msx")
			format= ObjectMsx;
		else if (arg == "--public")
			emitpublic= true;
		else if (arg == "--name")
		{
			++argpos;
			if (argpos >= argc)
				throw "Option --name needs an argument";
			headername= argv [argpos];
		}
		else if (arg == "-d")
			debugtype= Asm::DebugSecondPass;
		else if (arg == "-1")
			debugtype= Asm::DebugAll;
		else if (arg == "--err")
			redirecterr= true;
		else if (arg == "--nocase")
			nocase= true;
		else if (arg == "-I")
		{
			++argpos;
			if (argpos >= argc)
				throw "Option -I needs an argument";
			a.addincludedir (argv [argpos] );
		}
		else if (arg == "--")
		{
			++argpos;
			break;
		}
		else
			break;
	}

	if (argpos >= argc)
		throw Usage ();
	char * filein= argv [argpos];
	++argpos;
	if (argpos >= argc)
		throw Usage ();

	char * fileout= argv [argpos];
	++argpos;
	char * filesymbol= NULL;
	if (argpos < argc)
	{
		filesymbol= argv [argpos];
		++argpos;
		if (argpos < argc)
			cerr << "Extra arguments ignored" << endl;
	}

	if (headername.empty () )
		headername= fileout;

	if (redirecterr)
		perr= & std::cout;

	a.setdebugtype (debugtype);

	if (redirecterr)
		a.errtostdout ();
	if (nocase)
		a.caseinsensitive ();

	// Assemble.

	a.processfile (filein);

	// Generate ouptut file.

	std::ofstream out (fileout,
		std::ios::out | std::ios::binary);
	if (! out.is_open () )
		throw "Error creating object file";

	switch (format)
	{
	case ObjectRaw:
		a.emitobject (out);
		break;
	case ObjectHex:
		a.emithex (out);
		break;
	case ObjectPrl:
		{
			#if 0
			Asm aoff;
			if (redirecterr)
				aoff.errtostdout ();
			if (nocase)
				aoff.caseinsensitive ();
			#else
			Asm aoff (a);
			#endif
			aoff.setbase (0x100);
			aoff.processfile (filein);
			a.emitprl (out, aoff);
		}
		break;
	case ObjectPlus3Dos:
		a.emitplus3dos (out);
		break;
	case ObjectTap:
		a.emittap (out, headername);
		break;
	case ObjectTapBas:
		a.emittapbas (out, headername);
		break;
	case ObjectAmsdos:
		a.emitamsdos (out, headername);
		break;
	case ObjectMsx:
		a.emitmsx (out);
		break;
	}
	out.close ();

	// Generate symbol table if required.

	if (filesymbol)
	{
		//std::ofstream sout (filesymbol);
		std::ofstream sout;
		std::streambuf * cout_buf= 0;
		if (strcmp (filesymbol, "-") != 0)
		{
			sout.open (filesymbol);
			if (! sout.is_open () )
				throw "Error creating symbols file";
			cout_buf= cout.rdbuf ();
			cout.rdbuf (sout.rdbuf () );
		}
		if (emitpublic)
			a.dumppublic (cout);
		else
			a.dumpsymbol (cout);
		if (cout_buf)
		{
			cout.rdbuf (cout_buf);
			sout.close ();
		}
	}

	return 0;
}

} // namespace

int main (int argc, char * * argv)
{
	try
	{
		return doit (argc, argv);
	}
	catch (const char * str)
	{
		* perr << "ERROR: " << str << endl;
		return 1;
	}
	catch (const std::string & str)
	{
		* perr << "ERROR: " << str << endl;
		return 1;
	}
	catch (std::exception & e)
	{
		* perr << "ERROR: " << e.what () << endl;
	}
	catch (Usage &)
	{
		cerr <<	"Pasmo v. 0.3.6 (C) 2004 Julian Albo\n\n"
			"Usage:\n\n"
			"\tpasmo [options] source object [symbol]\n\n"
			"See the README file for details.\n";
	}
}

// End of pasmo.cpp
