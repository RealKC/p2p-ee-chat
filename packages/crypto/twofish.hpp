#pragma once

#include <cstdint>

namespace twofish {
/// @brief the function to call to encrypt a specific plaintext, with a 256b key given by the user using Twofish
/// @param plaintext list of unsigned 32 bit with 4 values - the text the user wants to encrypt / the input
/// @param key list of unsigned 32 bit with 8 values - the whole key given by the user to encrypt the plaintext / the key has to be 256b
/// @param cipher list of unsigned 32 bit with 4 values - the return of the Twofish algorithm based on the plaintext and the key
void encrypt_block(std::uint32_t plaintext[4], std::uint32_t const key[8], std::uint32_t cipher[4]);

/// @brief the function to call to decrypt the cipher and get the plaintext, with a 256b key given by the user using Twofish
/// @param plaintext list of unsigned 32 bit with 4 values
/// @param key list of unsigned 32 bit with 8 values
/// @param cipher list of unsigned 32 bit with 4 values
void decrypt_block(std::uint32_t plaintext[4], std::uint32_t const key[8], std::uint32_t const cipher[4]);
}
