// pasmo.cpp
// Revision 7-nov-2004

#include "asm.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>

using std::cout;
using std::cerr;
using std::endl;

namespace {

const std::string version ("0.4.0");

class Usage { };

enum ObjectFormat {
	ObjectRaw, ObjectHex, ObjectPrl,
	ObjectPlus3Dos, ObjectTap, ObjectTapBas,
	ObjectAmsdos, ObjectMsx
};

std::ostream * perr= & std::cerr;

int doit (int argc, char * * argv)
{
	using std::string;

	//Asm a;

	// Default values for options.

	ObjectFormat format= ObjectRaw;
	bool verbose= false;
	bool emitpublic= false;
	int argpos= 1;
	string headername;
	Asm::DebugType debugtype= Asm::NoDebug;
	bool redirecterr= false;
	bool nocase= false;
	bool autolocal= false;
	std::vector <string> includedir;
	std::vector <string> labelpredef;

	// Process command line options.

	for ( ; argpos < argc; ++argpos)
	{
		const string arg (argv [argpos] );
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
		else if (arg == "-v")
			verbose= true;
		else if (arg == "-d")
			debugtype= Asm::DebugSecondPass;
		else if (arg == "-1")
			debugtype= Asm::DebugAll;
		else if (arg == "--err")
			redirecterr= true;
		else if (arg == "--nocase")
			nocase= true;
		else if (arg == "--alocal")
			autolocal= true;
		else if (arg == "-I")
		{
			++argpos;
			if (argpos >= argc)
				throw "Option -I needs an argument";
			//a.addincludedir (argv [argpos] );
			includedir.push_back (argv [argpos] );
		}
		else if (arg == "-E" || arg == "--equ")
		{
			++argpos;
			if (argpos >= argc)
				throw "Option --equ needs an argument";
			labelpredef.push_back (argv [argpos] );
		}
		else if (arg == "--")
		{
			++argpos;
			break;
		}
		else
			break;
	}

	// File parameters.

	if (argpos >= argc)
		throw Usage ();
	string filein= argv [argpos];
	++argpos;
	if (argpos >= argc)
		throw Usage ();

	string fileout= argv [argpos];
	++argpos;
	char * filesymbol= NULL;
	if (argpos < argc)
	{
		filesymbol= argv [argpos];
		++argpos;
		if (argpos < argc)
			cerr << "WARNING: Extra arguments ignored" << endl;
	}

	if (headername.empty () )
		headername= fileout;

	if (redirecterr)
		perr= & std::cout;

	// Assemble.

	Asm assembler;

	assembler.setdebugtype (debugtype);

	if (verbose)
		assembler.verbose ();
	if (redirecterr)
		assembler.errtostdout ();
	if (nocase)
		assembler.caseinsensitive ();
	if (autolocal)
		assembler.autolocal ();

	for (size_t i= 0; i < includedir.size (); ++i)
		assembler.addincludedir (includedir [i] );

	for (size_t i= 0; i < labelpredef.size (); ++i)
		assembler.addpredef (labelpredef [i] );

	assembler.processfile (filein);

	// Generate ouptut file.

	std::ofstream out (fileout.c_str (),
		std::ios::out | std::ios::binary);
	if (! out.is_open () )
		throw "Error creating object file";

	switch (format)
	{
	case ObjectRaw:
		assembler.emitobject (out);
		break;
	case ObjectHex:
		assembler.emithex (out);
		break;
	case ObjectPrl:
		{
			// Assembly with 1 page offset to obtain
			// the information needed to create the
			// prl relocation table.
			Asm assembleroff (assembler);
			assembleroff.setbase (0x100);
			assembleroff.processfile (filein);
			// And use this second version to generate
			// the prl.
			assembler.emitprl (out, assembleroff);
		}
		break;
	case ObjectPlus3Dos:
		assembler.emitplus3dos (out);
		break;
	case ObjectTap:
		assembler.emittap (out, headername);
		break;
	case ObjectTapBas:
		assembler.emittapbas (out, headername);
		break;
	case ObjectAmsdos:
		assembler.emitamsdos (out, headername);
		break;
	case ObjectMsx:
		assembler.emitmsx (out);
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
			assembler.dumppublic (cout);
		else
			assembler.dumpsymbol (cout);
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
	// Just call doit and show possible errors.

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
	catch (std::logic_error & e)
	{
		* perr << "ERROR: " << e.what () << endl <<
			"This error is unexpected, please "
			"send a bug report." << endl;
	}
	catch (std::exception & e)
	{
		* perr << "ERROR: " << e.what () << endl;
	}
	catch (Usage &)
	{
		cerr <<	"Pasmo v. " << version <<
			" (C) 2004 Julian Albo\n\n"
			"Usage:\n\n"
			"\tpasmo [options] source object [symbol]\n\n"
			"See the README file for details.\n";
	}
	catch (...)
	{
		cerr << "ERROR: Unexpected exception.\n"
			"Please send a bug report.\n";
	}
}

// End of pasmo.cpp
