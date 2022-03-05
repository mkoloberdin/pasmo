// pasmotypes.cxx

#include "pasmotypes.h"

namespace
{

const char hexdigit[] =
{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

} // namespace

byte lobyte(address n)
{
    return static_cast<byte>(n & 0xFF);
}

byte hibyte(address n)
{
    return static_cast<byte>(n >> 8);
}

address makeword(byte low, byte high)
{
    return low | (address(high) << 8);
}

void putword(std::ostream & os, address word)
{
    os.put(lobyte(word) );
    os.put(hibyte(word) );
}

std::string hex2str(byte b)
{
    return std::string(1, hexdigit[ (b >> 4) & 0x0F] ) +
        hexdigit[b & 0x0F];
}

std::string hex4str(address n)
{
    return hex2str(hibyte(n) ) + hex2str(lobyte(n) );
}

std::string hex8str(size_t nn)
{
    return hex4str( (nn >> 16) & 0xFFFF) + hex4str(nn & 0xFFFF);
}

//--------------------------------------------------------------

Hex2::Hex2(byte b) :
    b(b)
{ }

std::string Hex2::str() const
{
    return hex2str(b);
}

Hex4::Hex4(address n) :
    n(n)
{ }

std::string Hex4::str() const
{
    return hex4str(n);
}

//--------------------------------------------------------------

Hex2 hex2(byte b) { return Hex2(b); }
Hex4 hex4(address n) { return Hex4(n); }

std::ostream & operator << (std::ostream & os, const Hex2 & h2)
{
    os << h2.str();
    return os;
}

std::ostream & operator << (std::ostream & os, const Hex4 & h4)
{
    os << h4.str();
    return os;
}

// End
