// test_protocol.h

namespace Pasmo
{
namespace Test
{

void plan(int n);

void diag(std::string msg);

void ok(bool success, const char * msg);
void nok(bool fail, const char * msg);

void is(address s1, address s2, const char * msg);
void is(std::string s1, std::string s2, const char * msg);

void throws_logic(const char * msg, void (*fn) (void));
void throws_runtime(const char * msg, void (*fn) (void));

} // namespace Test
} // namespace Pasmo

// End
