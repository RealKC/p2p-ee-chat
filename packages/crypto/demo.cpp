#include "common.hpp"

float add(float a, float b)
{
    return a + b;
}

#if IS_WASM
EMSCRIPTEN_BINDINGS(CryptoLib)
{
    using namespace emscripten;

    function("add", &add);
}
#endif

#if IS_TESTING

TEST_CASE("addition works as expected")
{
    REQUIRE(add(2, 3) == 5);
}

#endif
