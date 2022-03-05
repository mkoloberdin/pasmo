// asmfile.cxx

#include "asmfile.h"
#include "asmerror.h"

#include <vector>
#include <stdexcept>
#include <algorithm>

using std::runtime_error;

#include <assert.h>

#define ASSERT assert

#define DEBUG_LOAD 0

//**************************************************************

static const size_t LINE_BEGIN = static_cast <size_t>(-1);

namespace
{

AsmError FileNotFound(size_t linepos, const std::string & filename)
{
    return AsmError(linepos, "File '" + filename + "' not found");
}

AsmError InvalidInclude(size_t linepos, const Token & tok)
{
    return AsmError(linepos, "Unexpected " + tok.str() +
            " after INCLUDE file name");
}

//**************************************************************

class FileLine
{
    const std::string text;
    Tokenizer tkz;
    const size_t linenum;
public:
    FileLine(const std::string & text_n, const Tokenizer & tkz_n,
        size_t linenum_n);
    bool empty() const;
    size_t numline() const;
    Tokenizer & gettkz();
    const std::string & getstrline() const;
};

typedef std::vector <FileLine> filelines_t;

//--------------------------------------------------------------

class FileRef
{
    const std::string filename;
    filelines_t lines;
    const size_t l_begin;
    size_t l_end;

    FileLine & line(size_t n);
    const FileLine & line(size_t n) const;
public:
    FileRef(const std::string & name, size_t linebeg);
    void setend(size_t n);

    size_t linebegin() const;
    size_t lineend() const;
    std::string name() const;

    bool lineempty(size_t n) const;
    size_t numline(size_t n) const;
    Tokenizer & gettkz(size_t n);
    const std::string & getstrline(size_t n) const;

    void pushline(const std::string & text, const Tokenizer & tkz,
        size_t realnumline);
};

//--------------------------------------------------------------

class LineContent
{
    const size_t filenum;
    const size_t linenum;
public:
    LineContent(size_t linenumn, size_t filenumn);
    size_t getfileline() const;
    size_t getfilenum() const;
};

LineContent::LineContent(size_t filenumn, size_t linenumn) :
    filenum(filenumn),
    linenum(linenumn)
{ }

size_t LineContent::getfileline() const
{
    return linenum;
}

size_t LineContent::getfilenum() const
{
    return filenum;
}

//**************************************************************

FileLine::FileLine(const std::string & text_n, const Tokenizer & tkz_n,
        size_t linenum_n) :
    text(text_n),
    tkz(tkz_n),
    linenum(linenum_n)
{
}

bool FileLine::empty() const
{
    return tkz.empty();
}

size_t FileLine::numline() const
{
    return linenum;
}

Tokenizer & FileLine::gettkz()
{
    return tkz;
}

const std::string & FileLine::getstrline() const
{
    return text;
}

//--------------------------------------------------------------

FileRef::FileRef(const std::string & name, size_t linebeg) :
    filename(name),
    l_begin(linebeg)
{ }

void FileRef::setend(size_t n)
{
    l_end = n;
}

size_t FileRef::linebegin() const
{
    return l_begin;
}

size_t FileRef::lineend() const
{
    return l_end;
}

std::string FileRef::name() const
{
    return filename;
}

FileLine & FileRef::line(size_t n)
{
    return lines.at(n);
}

const FileLine & FileRef::line(size_t n) const
{
    return lines.at(n);
}

bool FileRef::lineempty(size_t n) const
{
    return line(n).empty();
}

size_t FileRef::numline(size_t n) const
{
    return line(n).numline();
}

Tokenizer & FileRef::gettkz(size_t n)
{
    return line(n).gettkz();
}

const std::string & FileRef::getstrline(size_t n) const
{
    return line(n).getstrline();
}

void FileRef::pushline(const std::string & text, const Tokenizer & tkz,
    size_t realnumline)
{
    FileLine fl(text, tkz, realnumline);
    lines.push_back(fl);
}

} // namespace

//**************************************************************

class AsmFile::In
{
public:
    In();
    void addref();
    void delref();

    size_t numlines() const;
    //size_t numfiles() const;

    bool lineempty(size_t n) const;
    Tokenizer & gettkz(size_t n);
    const std::string & getstrline(size_t n) const;

    void addincludedir(const std::string & dirname);
    void openis(size_t linepos, std::ifstream & is,
        const std::string & filename, std::ios::openmode mode) const;
    void copyfile(FileRef & fr, std::ostream & outverb);
    void loadfile(size_t linepos, const std::string & filename, bool nocase,
        std::ostream & outverb, std::ostream& outerr);

    void showerrorinfo(std::ostream & os,
        size_t nline, const std::string message) const;
    void showlineinfo(std::ostream & os, size_t nline) const;
    void showwarning(std::ostream & os,
        size_t nline, const std::string message) const;
private:
    In(const In &); // Forbidden.
    In & operator = (const In &); // Forbidden.

    const FileRef & getfile(size_t n) const;
    FileRef & getfile(size_t n);

    const LineContent & getline(size_t n) const;
    LineContent & getline(size_t n);

    size_t numrefs;

    typedef std::vector <LineContent> vlinecont_t;
    vlinecont_t vlinecont;

    std::vector <FileRef> vfileref;

    void pushline(size_t linenum, size_t file);

    // ******** Paths for include ************

    std::vector <std::string> includepath;
};

//--------------------------------------------------------------

AsmFile::In::In()
{
    numrefs = 1;
}

void AsmFile::In::addref()
{
    ++numrefs;
}

void AsmFile::In::delref()
{
    --numrefs;
    if (numrefs == 0)
        delete this;
}

size_t AsmFile::In::numlines() const
{
    return vlinecont.size();
}

#if 0
size_t AsmFile::In::numfiles() const
{
    return vfileref.size();
}
#endif

const FileRef & AsmFile::In::getfile(size_t n) const
{
    ASSERT(n < vfileref.size());
    return vfileref [n];
}

FileRef & AsmFile::In::getfile(size_t n)
{
    ASSERT(n < vfileref.size());
    return vfileref [n];
}

const LineContent & AsmFile::In::getline(size_t n) const
{
    ASSERT(n < numlines() );
    return vlinecont [n];
}

LineContent & AsmFile::In::getline(size_t n)
{
    ASSERT(n < numlines() );
    return vlinecont [n];
}

bool AsmFile::In::lineempty(size_t n) const
{
    ASSERT(n < numlines() );

    //return vlinecont [n].empty();
    const LineContent & lc = getline(n);
    return getfile(lc.getfilenum() ).lineempty(lc.getfileline() );
}

Tokenizer & AsmFile::In::gettkz(size_t n)
{
    ASSERT(n < numlines() );

    //return vlinecont [n].gettkz();
    LineContent & lc = getline(n);
    return getfile(lc.getfilenum() ).gettkz(lc.getfileline() );
}

const std::string & AsmFile::In::getstrline(size_t n) const
{
    ASSERT(n < numlines() );

    const LineContent & lc = getline(n);
    return getfile(lc.getfilenum() ).getstrline(lc.getfileline() );
}

void AsmFile::In::addincludedir(const std::string & dirname)
{
    if (const std::string::size_type l = dirname.size())
    {
        std::string aux(dirname);
        char c = aux [l - 1];
        if (c != '\\' && c != '/')
            aux+= '/';
        includepath.push_back(aux);
    }
}

void AsmFile::In::openis(size_t linepos, std::ifstream & is,
    const std::string & filename, std::ios::openmode mode) const
{
    ASSERT(! is.is_open() );
    is.open(filename.c_str(), mode);
    if (is.is_open() )
        return;
    for (size_t i = 0; i < includepath.size(); ++i)
    {
        std::string path(includepath [i] );
        path+= filename;
        is.clear();
        is.open(path.c_str(), mode);
        if (is.is_open() )
            return;
    }
    throw FileNotFound(linepos, filename);
}

void AsmFile::In::pushline(size_t filenum, size_t linenum)
{
    ASSERT(filenum < vfileref.size() );

    vlinecont.push_back(LineContent(filenum, linenum) );
}

void AsmFile::In::copyfile(FileRef & fr, std::ostream & outverb)
{
    outverb << "Reloading file: " << fr.name() <<
        " in " << numlines() << '\n';

    const size_t linebegin = fr.linebegin();
    const size_t lineend = fr.lineend();

    for (size_t i = linebegin; i < lineend; ++i)
    {
        LineContent l = getline(i);
        vlinecont.push_back(l);
    }

    outverb << "Finished reloading file: " << fr.name() <<
        " in " << numlines() << '\n';
}

void AsmFile::In::loadfile(size_t linepos, const std::string & filename,
    bool nocase, std::ostream & outverb, std::ostream & outerr)
{
    #if DEBUG_LOAD
    std::cerr << "loadfile in " <<
            ( (linepos != size_t(-1)) ? linepos : 0) <<
            ": " << filename << '\n';
    #endif

    auto oldref = std::find_if(vfileref.begin(), vfileref.end(),
            [& filename] (const auto & ref) { return ref.name() == filename; } );
    if (oldref != vfileref.end())
    {
        copyfile(*oldref, outverb);
        return;
    }

    // Load the file in memory.

    outverb << "Loading file: " << filename <<
        " in " << numlines() << '\n';

    std::ifstream file;
    openis(linepos, file, filename, std::ios::in);

    vfileref.push_back(FileRef(filename, numlines() ) );
    const size_t filenum = vfileref.size() - 1;

    std::string line;
    size_t linenum;
    size_t realnum;

    try
    {
        for (linenum = 0, realnum = 0;
            std::getline(file, line);
            ++linenum, ++realnum)
        {
            Tokenizer tz(line, nocase);
            Token tok = tz.gettoken();
            getfile(filenum).pushline(line, tz, realnum);
            pushline(filenum, linenum);
            if (tok.type() == TypeINCLUDE)
            {
                const size_t curposline = numlines() - 1;
                std::string includefile = tz.getincludefile();
                tok = tz.gettoken();
                if (tok.type() != TypeEndLine)
                {
                    //throw InvalidInclude(linepos, tok);
                    throw InvalidInclude(curposline, tok);
                }

                //loadfile(linenum, includefile, nocase, outverb, outerr);
                loadfile(curposline, includefile, nocase, outverb, outerr);

                Tokenizer tzaux(TypeEndOfInclude, "");
                getfile(filenum).pushline("", tzaux, 0);
                ++linenum;
                pushline(filenum, linenum);
            }
        }
        getfile(filenum).setend(numlines() );
    }
    catch (AsmError & err)
    {
        showerrorinfo(outerr, err.getline(), err.message());
        throw ErrorAlreadyReported();
    }
    outverb << "Finished loading file: " << filename <<
        " in " << numlines() << '\n';
}

void AsmFile::In::showerrorinfo(std::ostream & os,
    size_t nline, const std::string message) const
{
    os << "ERROR: " << message << ' ';
    showlineinfo(os, nline);
    os << '\n';
}

void AsmFile::In::showlineinfo(std::ostream & os, size_t nline) const
{
    if (nline >= numlines())
        os << " detected after end of file";
    else
    {
    const LineContent & linf = getline(nline);
    const FileRef & fileref = getfile(linf.getfilenum() );

    os << " on line " << fileref.numline(linf.getfileline() ) + 1 <<
        " of file " << fileref.name();
    }
}

void AsmFile::In::showwarning(std::ostream & os,
    size_t nline, const std::string message) const
{
    os << "WARNING: " << message;
    showlineinfo(os, nline);
    os << '\n';
}

//*******************************************************************

AsmFile::AsmFile() :
    pin(new In)
{
}

AsmFile::AsmFile(const AsmFile & af) :
    pin(af.pin)
{
    pin->addref();
}

AsmFile::~AsmFile()
{
    pin->delref();
}

// These functions are for propagate constness to the internal class.

inline AsmFile::In & AsmFile::in()
{
    return * pin;
}

inline const AsmFile::In & AsmFile::in() const
{
    return * pin;
}

void AsmFile::addincludedir(const std::string & dirname)
{
    in().addincludedir(dirname);
}

void AsmFile::openis(size_t linepos, std::ifstream & is,
    const std::string & filename, std::ios::openmode mode) const
{
    in().openis(linepos, is, filename, mode);
}

void AsmFile::loadfile(size_t linepos, const std::string & filename,
    bool nocase, std::ostream & outverb, std::ostream& outerr)
{
    in().loadfile(linepos, filename, nocase, outverb, outerr);
}

bool AsmFile::getvalidline()
{
    for (;;)
    {
        if (currentline >= in().numlines() )
            return false;
        //if (! in().getlinecont(currentline).empty() )
        if (! in().lineempty(currentline) )
            return true;
        ++currentline;
    }
}

bool AsmFile::passeof() const
{
    return currentline >= in().numlines();
}

size_t AsmFile::getline() const
{
    return currentline;
}

Tokenizer & AsmFile::getcurrentline()
{
    ASSERT(! passeof() );
    Tokenizer & tz = in().gettkz(currentline);
    tz.reset();
    return tz;
}

const std::string & AsmFile::getcurrenttext() const
{
    ASSERT(! passeof() );
    //return in().getlinecont(currentline).getstrline();
    return in().getstrline(currentline);
}

void AsmFile::setline(size_t line)
{
    currentline = line;
}

void AsmFile::setendline()
{
    currentline = in().numlines();
}

void AsmFile::beginline()
{
    currentline = LINE_BEGIN;
}

bool AsmFile::nextline()
{
    if (currentline == LINE_BEGIN)
        currentline = 0;
    else
    {
        if (passeof() )
            return false;
        ++currentline;
    }
    if (! getvalidline() )
        return false;
    return true;
}

void AsmFile::prevline()
{
    ASSERT(currentline > 0);
    --currentline;
}

void AsmFile::showerrorinfo(std::ostream & os,
    size_t nline, const std::string message) const
{
    in().showerrorinfo(os, nline, message);
}

void AsmFile::showwarning(std::ostream & os,
    size_t nline, const std::string message) const
{
    in().showwarning(os, nline, message);
}

// End
