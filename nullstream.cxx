// nullstream.cxx

#include "nullstream.h"

Nullbuff::Nullbuff()
{
    setbuf(0, 0);
}

int Nullbuff::overflow(int)
{
    setp(pbase(), epptr() );
    return 0;
}

Nullostream::Nullostream() : std::ostream(& buff)
{
}

// End
