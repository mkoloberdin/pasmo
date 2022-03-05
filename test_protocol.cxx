// test_protocol.cxx

#include "pasmotypes.h"
#include "test_protocol.h"

namespace Pasmo
{
namespace Test
{

static int count = 0;

void plan(int n)
{
    std::cout << "1.." << n << '\n';
    count = 0;
}

void diag(std::string msg)
{
    std::cout << "# " << msg << '\n';
}

void ok(bool success, const char * msg)
{
    if (! success)
        std::cout << "not ";
    std::cout << "ok " << ++count;
    if (msg)
        std::cout << " - " << msg;
    std::cout << '\n';
}

void nok(bool fail, const char * msg)
{
    ok(!fail, msg);
}

void is(address s1, address s2, const char * msg)
{
    const bool r = s1 == s2;
    if (! r)
        std::cout << "# " << "expected " << s2 << " but " << s1 << " found\n";
    ok(r, msg);
}

void is(std::string s1, std::string s2, const char * msg)
{
    const bool r = s1 == s2;
    if (! r)
        diag("expected " + s2 + " but " + s1 + " found");
    ok(r, msg);
}

void throws_logic(const char * msg, void (*fn) (void))
{
    bool thr = false;
    try
    {
        fn();
    }
    catch (const std::logic_error & ex)
    {
        diag(std::string("caught ") + ex.what());
        thr = true;
    }
    ok(thr, msg);
}

void throws_runtime(const char * msg, void (*fn) (void))
{
    bool thr = false;
    try
    {
        fn();
    }
    catch (const std::runtime_error & ex)
    {
        diag(std::string("caught ") + ex.what());
        thr = true;
    }
    ok(thr, msg);
}

} // namespace Test
} // namespace Pasmo

// End
