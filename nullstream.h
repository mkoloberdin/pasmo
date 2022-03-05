// nullstream.h

#include <ostream>

class Nullbuff : public std::streambuf
{
public:
    Nullbuff();
protected:
    int overflow(int);
};

class Nullostream : public std::ostream
{
public:
    Nullostream();
private:
    Nullbuff buff;
};

// End
