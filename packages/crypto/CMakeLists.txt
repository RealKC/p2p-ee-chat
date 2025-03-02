cmake_minimum_required(VERSION 3.31)
project(CryptoLib)

add_executable(crypto-lib
    common.hpp
    demo.cpp)

if(NOT EMSCRIPTEN)
    Include(FetchContent)

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.0 # or a later release
    )

    FetchContent_MakeAvailable(Catch2)

    target_link_libraries(crypto-lib PRIVATE Catch2::Catch2WithMain)
    target_compile_definitions(crypto-lib PRIVATE IS_TESTING)

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(CTest)
    include(Catch)
    catch_discover_tests(crypto-lib)
endif()

set_target_properties(crypto-lib PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    target_link_options(
        crypto-lib
        PRIVATE
        "SHELL:-sENVIRONMENT='web'"
        -sEXPORT_ES6
        -sEXPORT_NAME=CryptoLib
        -sINVOKE_RUN=0

        # Bindings rleated stuff
        --bind
        --emit-tsd crypto-lib.d.ts

        # RelWithDebInfo build flags, enable separate dwarf
        # to reduce wasm file size
        $<$<CONFIG:RELWITHDEBINFO>:-gseparate-dwarf=${CMAKE_CURRENT_BINARY_DIR}/app.dwarf>
        $<$<CONFIG:RELWITHDEBINFO>:-g>
        $<$<CONFIG:RELWITHDEBINFO>:-O2>

        # Debug build flags
        $<$<CONFIG:DEBUG>:-fwasm-exceptions>
        $<$<CONFIG:DEBUG>:-g>
        $<$<CONFIG:DEBUG>:-O0>)
endif()
