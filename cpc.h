#ifndef INCLUDE_CPCCRC_H
#define INCLUDE_CPCCRC_H

// cpc.h

#include <stdlib.h>

#include "pasmotypes.h"

class Asm;

namespace cpc
{

unsigned short crc (const unsigned char * data, size_t size);

class Header
{
public:
    //Header ();
    Header (const std::string & filename);

    enum Type { Basic, Binary };

    void clear ();
    void setfilename (const std::string & filename);
    void settype (Type type);
    void setblock (byte n);
    void firstblock (bool isfirst);
    void lastblock (bool islast);
    void setlength (address len);
    void setblocklength (address blen);
    void setloadaddress (address load);
    void setentry (address entry);
    void write (std::ostream & out);
private:
    static const size_t headsize= 64;
    byte data [headsize];
};

#if 0
class AmsdosHeader
{
public:
    //AmsdosHeader ();
    AmsdosHeader (const std::string & filename);

    void clear ();
    void setfilename (const std::string & filename);
    void setlength (address len);
    void setloadaddress (address load);
    void setentry (address entry);
    void write (std::ostream & out);
private:
    static const size_t headsize= 128;
    byte amsdos [headsize];
};
#endif

// CPC Locomotive Basic generation.

//extern const std::string tokHexNumber;

//extern const std::string tokCALL;
//extern const std::string tokLOAD;
//extern const std::string tokMEMORY;

//std::string number (address n);
//std::string hexnumber (address n);
//std::string basicline (address linenum, const std::string & line);

std::string cpcbasicloader(const Asm & as);

void write_cdt_code(const Asm & as, std::ostream & out);
void write_amsdos(const Asm & as, std::ostream & out);

} // namespace cpc

#endif

// End
