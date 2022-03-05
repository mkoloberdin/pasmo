#ifndef INCLUDE_ASMFILE_H
#define INCLUDE_ASMFILE_H

// asmfile.h

#include "token.h"

#include <iostream>
#include <fstream>
#include <string>

class AsmFile
{
public:
    AsmFile();
    AsmFile(const AsmFile & af);
    ~AsmFile();
    void addincludedir(const std::string & dirname);
    void loadfile(size_t linepos, const std::string & filename, bool nocase,
        std::ostream & outverb, std::ostream & outerr);
    size_t getline() const;
    void showerrorinfo(std::ostream & os,
        size_t nline, const std::string message) const;
protected:
    void openis(size_t linepos, std::ifstream & is,
        const std::string & filename, std::ios::openmode mode) const;
    void showwarning(std::ostream & os,
        size_t nline, const std::string message) const;
    bool getvalidline();
    bool passeof() const;
    Tokenizer & getcurrentline();
    const std::string & getcurrenttext() const;

    void setline(size_t line);
    void setendline();
    void beginline();
    bool nextline();
    void prevline();
private:
    class In;
    In * pin;
    In & in();
    const In & in() const;

    size_t currentline;
};

#endif

// End
