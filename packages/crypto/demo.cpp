#include "common.hpp"

float add(float a, float b)
{
    return a + b;
}

#ifdef IS_WASM
EMSCRIPTEN_BINDINGS(CryptoLib)
{
    using namespace emscripten;

    function("add", &add);
}
#endif
