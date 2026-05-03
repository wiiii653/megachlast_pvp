#include "FxGovernor.h"

#include <iostream>

namespace {

int failures = 0;

void check(bool ok, const char* msg)
{
    if(!ok){
        std::cerr << "FAIL: " << msg << "\n";
        ++failures;
    }
}

} // namespace

int main()
{
    using namespace fx_governor;

    check(levelScale(0) == 1.f, "fx level 0 uses full scale");
    check(levelScale(3) == 0.40f, "fx level 3 uses reduced scale");

    State s{};

    for(int i = 0; i < 240; ++i){
        update(s, 1.f / 30.f, true, 120, 120);
        check(s.level >= 0 && s.level <= 3, "fx level stays in bounds under overload");
    }
    check(s.level >= 2, "fx governor ramps up under sustained overload");

    for(int i = 0; i < 420; ++i){
        update(s, 1.f / 120.f, true, 220, 15);
        check(s.level >= 0 && s.level <= 3, "fx level stays in bounds during recovery");
    }
    check(s.level <= 1, "fx governor recovers when load drops");

    for(int i = 0; i < 180; ++i)
        update(s, 1.f / 60.f, false, 0, 0);
    check(s.level == 0, "fx governor settles to level 0 out of gameplay");

    return failures == 0 ? 0 : 1;
}
