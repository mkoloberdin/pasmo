// cpc.cxx
// Revision 17-jul-2021

#include "cpc.h"
#include "tzx.h"
#include "asm.h"

#include <algorithm>
#include <stdexcept>

#include <stdlib.h>

using std::fill;
using std::logic_error;

// This routine is adapted from 2CDT from Kevin Thacker
// (at his time taken from Pierre Guerrier's AIFF decoder).

unsigned short cpc::crc (const unsigned char * data, size_t size)
{
    typedef unsigned short word;

    const word crcinitial= 0xFFFF;
    const word crcpoly= 0x1021;
    const word crcfinalxor= 0xFFFF;
    const size_t cpcchunksize= 256;

    word crc= crcinitial;
    for (size_t n= 0; n < cpcchunksize; ++n)
    {
        char c= n < size ? data [n] : 0;
        crc^= (static_cast <word> (c) << 8);
        for (size_t i= 0; i < 8; ++i)
        {
            if (crc & 0x8000)
                crc= (crc << 1) ^ crcpoly;
            else
                crc<<= 1;
        }
    }
    crc^= crcfinalxor;
    return crc;
}

//**************************************************************

namespace cpc
{

class AmsdosHeader
{
public:
    AmsdosHeader (const std::string & filename);
    void setfilename (const std::string & filename);
    void setlength (address len);
    void setloadaddress (address load);
    void setentry (address entry);
    void write (std::ostream & out);
private:
    void clear ();

    static const size_t headsize= 128;
    byte amsdos [headsize];
};

} // namespace cpc

//**************************************************************

cpc::Header::Header (const std::string & filename)
{
    clear ();
    setfilename (filename);
}

void cpc::Header::clear ()
{
    //memset (data, 0, headsize);
    fill (data, data + headsize, byte (0) );
}

void cpc::Header::setfilename (const std::string & filename)
{
    std::string::size_type l= filename.size ();
    if (l > 16)
        l= 16;
    for (std::string::size_type i= 0; i < l; ++i)
        data [i]= filename [i];
    for (std::string::size_type i= l; i < 16; ++i)
        data [i]= '\0';
}

void cpc::Header::settype (Type type)
{
    byte b = 0;
    switch (type)
    {
    case Basic:
        break;
    case Binary:
        b= 2; break;
    }
    data [0x12]= b;
}

void cpc::Header::setblock (byte n)
{
    data [0x010]= n;
}

void cpc::Header::firstblock (bool isfirst)
{
    data [0x17]= isfirst ? 0xFF : 0x00;
}

void cpc::Header::lastblock (bool islast)
{
    data [0x11]= islast ? 0xFF : 0x00;
}

void cpc::Header::setlength (address len)
{
    data [0x18]= lobyte (len);
    data [0x19]= hibyte (len);
}

void cpc::Header::setblocklength (address blen)
{
    data [0x13]= lobyte (blen);
    data [0x14]= hibyte (blen);
}

void cpc::Header::setloadaddress (address load)
{
    data [0x15]= lobyte (load);
    data [0x16]= hibyte (load);
}

void cpc::Header::setentry (address entry)
{
    data [0x1A]= lobyte (entry);
    data [0x1B]= hibyte (entry);
}

void cpc::Header::write (std::ostream & out)
{
    out.put (0x2C);  // Header identifier.

    out.write (reinterpret_cast <const char *> (data), headsize);
    for (size_t i= headsize; i < 256; ++i)
        out.put ('\x00');

    address crcword= crc (data, headsize);
    out.put (hibyte (crcword) ); // CRC in hi-lo format.
    out.put (lobyte (crcword) );

    out.put ('\xFF');
    out.put ('\xFF');
    out.put ('\xFF');
    out.put ('\xFF');
}

//**************************************************************

static std::string amsdosfilename(const std::string & s)
{
    std::string::size_type pos = s.find('.');
    std::string name, ext;
    if (pos == std::string::npos)
        name = s;
    else
    {
        name = s.substr(0, pos);
        ext = s.substr(pos + 1);
    }
    name = (name + "        ").substr(0, 8);
    ext = (ext + "   ").substr(0, 3);
    return name + ext;
}

cpc::AmsdosHeader::AmsdosHeader (const std::string & filename)
{
    clear ();
    setfilename (amsdosfilename(filename));
}

void cpc::AmsdosHeader::clear ()
{
    //memset (amsdos, 0, headsize);
    fill (amsdos, amsdos + headsize, byte (0) );
    amsdos [0x12]= 2; // File type: binary.
}

void cpc::AmsdosHeader::setfilename (const std::string & filename)
{
    amsdos [0]= 0; // User number.
    // 01-0F: filename, padded with 0.
    fill (amsdos + 1, amsdos + 15, byte (0) );
    std::string::size_type l = std::min(filename.size(), std::string::size_type(15));
    for (std::string::size_type i = 0; i < l; ++i)
        amsdos[i + 1] = filename[i];
}

void cpc::AmsdosHeader::setlength (address len)
{
    // 18-19: logical length.
    amsdos [0x18]= lobyte (len);
    amsdos [0x19]= hibyte (len);
    // 40-42: real length of file.
    amsdos [0x40]= lobyte (len);
    amsdos [0x41]= hibyte (len);
    amsdos [0x42]= 0;
}

void cpc::AmsdosHeader::setloadaddress (address load)
{
    // 15-16: Load address.
    amsdos [0x15]= lobyte (load);
    amsdos [0x16]= hibyte (load);
}

void cpc::AmsdosHeader::setentry (address entry)
{
    // 1A-1B: Entry address.
    amsdos [0x1A]= lobyte (entry);
    amsdos [0x1B]= hibyte (entry);
}

void cpc::AmsdosHeader::write (std::ostream & out)
{
    // 43-44 checksum of bytes 00-42
    address check= 0;
    for (int i= 0; i < 0x43; ++i)
        check+= amsdos [i];
    amsdos [0x43]= lobyte (check);
    amsdos [0x44]= hibyte (check);

    // Write header.
    out.write (reinterpret_cast <char *> (amsdos), headsize);
}

//**************************************************************

// CPC Locomotive Basic generation.

static const char
    tokHexNumber = '\x1C',
    tokCALL = '\x83',
    tokLOAD = '\xA8',
    tokMEMORY = '\xAA';

static std::string number (address n)
{
    std::string r (1, lobyte (n) );
    r+= hibyte (n);
    return r;
}

static std::string hexnumber (address n)
{
    return tokHexNumber + number (n);
}

static std::string basicline (address linenum, const std::string & line)
{
    address linelen= static_cast <address> (line.size () + 5);
    return number (linelen) + number (linenum) + line + '\0';
}

std::string cpc::cpcbasicloader(const Asm & as)
{
    std::string basic;
    const address minused = as.getminused();

    // Line: 10 MEMORY before_min_used
    std::string line = tokMEMORY + hexnumber(minused - 1);
    basic+= basicline(10, line);

    // Line: 20 LOAD "!", minused
    line = std::string(1, tokLOAD) + "\"!\"," + hexnumber(minused);
    basic+= basicline(20, line);

    if (as.hasentrypoint())
    {
        // Line: 30 CALL entry_point
        line = tokCALL + hexnumber(as.getentrypoint());
        basic+= basicline(30, line);
    }

    // A line length of 0 marks the end of program.
    basic+= '\0';
    basic+= '\0';

    return basic;
}

void cpc::write_cdt_code(const Asm & as, std::ostream & out)
{
    const address minused = as.getminused();
    const address codesize = as.getcodesize();
    const address entry = as.hasentrypoint() ? as.getentrypoint(): 0;
    const byte * const mem = as.getmem();

    cpc::Header head(as.getheadername());
    head.settype(cpc::Header::Binary);
    head.firstblock(true);
    head.lastblock(false);
    head.setlength(codesize);
    head.setloadaddress(minused);
    head.setentry(entry);

    address pos = minused;
    address pending = codesize;

    const address maxblock = 2048;
    const address maxsubblock = 256;
    byte blocknum = 1;

    while (pending > 0)
    {
        const address block = pending < maxblock ? pending : maxblock;

        head.setblock(blocknum);
        if (blocknum > 1)
            head.firstblock(false);
        if (pending <= maxblock)
            head.lastblock(true);
        head.setblocklength(block);

        // Size of the tzx data block: type byte, code, checksums,
        // filling of last subblock and final bytes.

        size_t tzxdatalen = static_cast <size_t> (block) +
            maxsubblock - 1;
        tzxdatalen/= maxsubblock;
        tzxdatalen*= maxsubblock + 2;
        tzxdatalen+= 5;

        // Write header.

        tzx::writeturboblockhead(out, 263);

        head.write(out);

        // Write code.

        tzx::writeturboblockhead(out, tzxdatalen);

        out.put(0x16);  // Data block identifier.

        address subpos = pos;
        address blockpending = block;
        while (blockpending > 0)
        {
            const address subblock = blockpending < maxsubblock ?
                blockpending : maxsubblock;
            out.write(reinterpret_cast <const char *>
                (mem + subpos),
                subblock);
            for (size_t i = subblock; i < maxsubblock; ++i)
                out.put('\0');
            const address crc = cpc::crc(mem + subpos, subblock);
            out.put(hibyte(crc) ); // CRC in hi-lo format.
            out.put(lobyte(crc) );
            blockpending-= subblock;
            subpos+= subblock;
        }

        out.put('\xFF');
        out.put('\xFF');
        out.put('\xFF');
        out.put('\xFF');

        pos+= block;
        pending-= block;
        ++blocknum;
    }
}

void cpc::write_amsdos(const Asm & as, std::ostream & out)
{
    const address minused = as.getminused();
    const address codesize = as.getcodesize();

    cpc::AmsdosHeader head(as.getheadername());
    head.setlength(codesize);
    head.setloadaddress(minused);
    if (as.hasentrypoint())
        head.setentry(as.getentrypoint());

    head.write(out);

    // Write code.
    as.writebincode(out);
}

// End
