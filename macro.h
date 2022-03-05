// macro.h
// Revision 22-dec-2021

#include <vector>

namespace pasmo_impl
{

class MacroBase
{
protected:
    MacroBase();
    explicit MacroBase(std::vector <std::string> & param);
    explicit MacroBase(const std::string & sparam);
public:
    size_t getparam(const std::string & name) const;
    std::string getparam(size_t n) const;
    static const size_t noparam = size_t(-1);
private:
    std::vector <std::string> param;
};

class Macro : public MacroBase
{
public:
    Macro(std::vector <std::string> & param,
            size_t linen, size_t endlinen);
    size_t getline() const;
    //size_t getendline() const;
private:
    const size_t line;
    const size_t endline;
};

class MacroIrp : public MacroBase
{
public:
    MacroIrp(const std::string & sparam);
};

class MacroIrpc : public MacroBase
{
public:
    MacroIrpc(const std::string & sparam);
};


class MacroRept : public MacroBase
{
public:
    MacroRept();
};

} // namespace pasmo_impl

// End
