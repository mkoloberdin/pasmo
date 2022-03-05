#ifndef INCLUDE_PASMOTYPES_H
#define INCLUDE_PASMOTYPES_H

// pasmotypes.h

#include <string>
#include <iostream>

#include <limits.h>
#include <stdlib.h>


#if USHRT_MAX == 65535

typedef unsigned short address;

#else

// Using the C99 types, hoping they will be available.

#include <stdint.h>

typedef uint16_t address:

#endif

#if UCHAR_MAX == 255

typedef unsigned char byte;

#else

#include <stdint.h>

typedef uint8_t byte;

#endif

byte lobyte(address n);
byte hibyte(address n);
address makeword(byte low, byte high);

void putword(std::ostream & os, address word);

std::string hex2str(byte b);
std::string hex4str(address n);
std::string hex8str(size_t nn);

class Hex2
{
public:
    Hex2(byte b);
    std::string str() const;
private:
    byte b;
};

class Hex4
{
public:
    Hex4(address n);
    std::string str() const;
private:
    address n;
};

Hex2 hex2(byte b);
Hex4 hex4(address n);

std::ostream & operator << (std::ostream & os, const Hex2 & h2);
std::ostream & operator << (std::ostream & os, const Hex4 & h4);

#endif

// End
