// asm.h
// Revision 6-dec-2004

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>

class Asm {
public:
	Asm ();

	// This is not a copy constructor, it creates a new
	// instance copying only the options.
	//explicit Asm (const Asm & a);

	~Asm ();

	void verbose ();
	enum DebugType { NoDebug, DebugSecondPass, DebugAll };
	DebugType debugtype;
	void setdebugtype (DebugType type);
	void errtostdout ();
	void setbase (unsigned int addr);
	void caseinsensitive ();
	void autolocal ();
	void bracketonly ();
	void warn8080 ();
	void set86 ();

	void addincludedir (const std::string & dirname);
	void addpredef (const std::string & predef);

	void loadfile (const std::string & filename);
	void processfile ();

	void emitobject (std::ostream & out);
	void emitplus3dos (std::ostream & out);

	void emittap (std::ostream & out, const std::string & filename);
	void emittzx (std::ostream & out, const std::string & filename);
	void emitcdt (std::ostream & out, const std::string & filename);

	void emittapbas (std::ostream & out, const std::string & filename);
	void emittzxbas (std::ostream & out, const std::string & filename);
	void emitcdtbas (std::ostream & out, const std::string & filename);

	void emithex (std::ostream & out);
	void emitamsdos (std::ostream & out, const std::string & filename);

	//void emitprl (std::ostream & out, Asm & asmoff);
	void emitprl (std::ostream & out);
	void emitcmd (std::ostream & out);

	void emitmsx (std::ostream & out);
	void dumppublic (std::ostream & out);
	void dumpsymbol (std::ostream & out);
private:
	Asm (const Asm & a); // Forbidden
	void operator = (const Asm &); // Forbidden
public:
	// Make it public to simplify implementation.
	class In;
	friend class In;
private:
	In * pin;
};

// End of asm.h
