#include "common.hpp"

float add(float a, float b)
{
    return a + b;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(CryptoLib)
{
    using namespace emscripten;

    function("add", &add);
}
#endif
