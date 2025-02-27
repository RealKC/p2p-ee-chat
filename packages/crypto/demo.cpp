#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#endif

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
