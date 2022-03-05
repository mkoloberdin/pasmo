// macro.cxx

#include <string>
#include <vector>

#include "macro.h"

namespace pasmo_impl
{

//--------------------------------------------------------------

MacroBase::MacroBase()
{ }

MacroBase::MacroBase(std::vector <std::string> & param) :
    param(param)
{ }

MacroBase::MacroBase(const std::string & sparam) :
    param(1)
{
    param [0] = sparam;
}

size_t MacroBase::getparam(const std::string & name) const
{
    for (size_t i = 0; i < param.size(); ++i)
    {
        if (name == param[i])
            return i;
    }
    return noparam;
}

std::string MacroBase::getparam(size_t n) const
{
    if (n >= param.size() )
        return "(none)";
    return param[n];
}

//--------------------------------------------------------------

Macro::Macro(std::vector <std::string> & param,
        size_t linen, size_t endlinen) :
    MacroBase(param),
    line(linen),
    endline(endlinen)
{ }

size_t Macro::getline() const { return line; }

//size_t Macro::getendline() const { return endline; }

//--------------------------------------------------------------

MacroIrp::MacroIrp(const std::string & sparam) :
    MacroBase(sparam)
{ }

//--------------------------------------------------------------

MacroIrpc::MacroIrpc(const std::string & sparam) :
    MacroBase(sparam)
{ }

//--------------------------------------------------------------

MacroRept::MacroRept() :
    MacroBase()
{ }

//--------------------------------------------------------------

} // namespace pasmo_impl

// End
