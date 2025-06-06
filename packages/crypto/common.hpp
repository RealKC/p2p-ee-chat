#pragma once

#ifdef __EMSCRIPTEN__
#    include <emscripten/bind.h>
#    include <emscripten/emscripten.h>
#    define IS_WASM 1
#else
#    define IS_WASM 0
#endif

#if IS_TESTING
#    include <catch2/catch_test_macros.hpp>
#endif
