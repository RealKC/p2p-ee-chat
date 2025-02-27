cmake_minimum_required(VERSION 3.31)
project(CryptoLib)

add_executable(crypto-lib demo.cpp)
set_target_properties(crypto-lib PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

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
