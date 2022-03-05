#ifndef INCLUDE_TZX_H
#define INCLUDE_TZX_H

// tzx.h

#include <iostream>

#include <stdlib.h>

class Asm;

namespace tzx
{

void writefilehead(std::ostream & out);

void writestandardblockhead(std::ostream & out);

void writeturboblockhead(std::ostream & out, size_t len);

void write_tzx_code(const Asm & as, std::ostream & out);

} // namespace tzx

#endif

// End
