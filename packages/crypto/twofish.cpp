#include "twofish.hpp"

#include "common.hpp"
#include <bit>
#include <stdexcept>

// NOLINTBEGIN(readability-isolate-declaration): The code is readable as is
// NOLINTBEGIN(readability-math-missing-parentheses): It's pretty clear what the intended precedence is where this lint fires
// NOLINTBEGIN(bugprone-implicit-widening-of-multiplication-result)

#define ROR4(x) ((x >> 1) | ((x << 3) & 0xf))

#define MDS_MUL 0x69 // 0x169 - polynom for MDS
#define RS_MUL 0x4D  // 14D - polynom fro RS

// #ifndef PRINT_LEVEL
// #define PRINT_LEVEL 0
// #endif
//
// #if PRINT_LEVEL > 0
//     #define debug_print(...) printf(__VA_ARGS__)
// #endif

// the code actually takes a key of 256bit and does all the operations
// it does not take a smaller one

#define byte0(x) ((uint8_t)((x)))
#define byte1(x) ((uint8_t)((x) >> 8))
#define byte2(x) ((uint8_t)((x) >> 16))
#define byte3(x) ((uint8_t)((x) >> 24))

#define z0(x) ((uint32_t)((x)))
#define z1(x) ((uint32_t)((x) << 8))
#define z2(x) ((uint32_t)((x) << 16))
#define z3(x) ((uint32_t)((x) << 24))

// todo: write some clear prints where the user can see what the hell is happening
// with param from command line || 0 - fara printf || 1 - cu printf

// RS matrix
uint8_t const RS[] = {
    0x01, 0xA4, 0x55, 0x87, 0x5A, 0x58, 0xDB, 0x9E,
    0xA4, 0x56, 0x82, 0xF3, 0x1E, 0xC6, 0x68, 0xE5,
    0x02, 0xA1, 0xFC, 0xC1, 0x47, 0xAE, 0x3D, 0x19,
    0xA4, 0x55, 0x87, 0x5A, 0x58, 0xDB, 0x9E, 0x03
};

// MDS matrix
uint8_t const MDS[] = {
    0x01, 0xEF, 0x5B, 0x5B,
    0x5B, 0xEF, 0xEF, 0x01,
    0xEF, 0x5B, 0x01, 0xEF,
    0xEF, 0x01, 0xEF, 0x5B
};

// first permutation values
uint8_t const t_q0[] = {
    0x8, 0x1, 0x7, 0xD, 0x6, 0xF, 0x3, 0x2, 0x0, 0xB, 0x5, 0x9, 0xE, 0xC, 0xA, 0x4,
    0xE, 0xC, 0xB, 0x8, 0x1, 0x2, 0x3, 0x5, 0xF, 0x4, 0xA, 0x6, 0x7, 0x0, 0x9, 0xD,
    0xB, 0xA, 0x5, 0xE, 0x6, 0xD, 0x9, 0x0, 0xC, 0x8, 0xF, 0x3, 0x2, 0x4, 0x7, 0x1,
    0xD, 0x7, 0xF, 0x4, 0x1, 0x2, 0x6, 0xE, 0x9, 0xB, 0x3, 0x0, 0x8, 0x5, 0xC, 0xA
};

// second permutation values
uint8_t const t_q1[] = {
    0x2, 0x8, 0xB, 0xD, 0xF, 0x7, 0x6, 0xE, 0x3, 0x1, 0x9, 0x4, 0x0, 0xA, 0xC, 0x5,
    0x1, 0xE, 0x2, 0xB, 0x4, 0xC, 0x3, 0x7, 0x6, 0xD, 0xA, 0x5, 0xF, 0x9, 0x0, 0x8,
    0x4, 0xC, 0x7, 0x5, 0x1, 0x6, 0x9, 0xA, 0x0, 0xE, 0xD, 0x8, 0x2, 0xB, 0x3, 0xF,
    0xB, 0x9, 0x5, 0x1, 0xC, 0x3, 0xD, 0xE, 0x6, 0x4, 0x7, 0xF, 0x2, 0x0, 0x8, 0xA
};

/// @brief for MDS : x^8 + x^6 + x^5 + x^3 + x^0
///
/// for RS : x^8 + x^6 + x^3 + x^2 + x^0
/// @param a unsigned 8 bit value to do the mul
/// @param b unsigned 8 bit value to do the mul
/// @param poly the polynomial used of degree 8 over GF(2)
/// @return unsigned 8 bit value, the result of the multiplication of "a" and "b" over the polynom
static uint8_t galois_mul(uint8_t a, uint8_t b, uint8_t poly)
{
    uint8_t result = 0;
    while (a > 0) {
        if (a & 1) {
            result ^= b;
        }
        a >>= 1;
        // it verifies if it has carry, if the second most semnificative bit is 1
        if (b & 0x80) {
            b = (b << 1) ^ poly;
        } else {
            b <<= 1;
        }
    }
    return result;
}

/// @brief this is a fixed permutation where there are used rotations and XOR's and then the value is passed through an fixed S-box
/// @param x unsigned 8 bit value
/// @return 8 unsigned bit value
static uint8_t permutation_q0(uint8_t x)
{
    uint8_t a0 = x / 16;
    uint8_t b0 = x % 16;

    uint8_t a1 = a0 ^ b0;
    uint8_t b1 = (a0 ^ ROR4(b0) ^ (8 * a0)) % 16;

    uint8_t a2 = t_q0[a1];
    uint8_t b2 = t_q0[b1 + 16];

    uint8_t a3 = a2 ^ b2;
    uint8_t b3 = (a2 ^ ROR4(b2) ^ (8 * a2)) % 16;

    uint8_t a4 = t_q0[a3 + 16 * 2];
    uint8_t b4 = t_q0[b3 + 16 * 3];

    return 16 * b4 + a4; // y
}

/// @brief this is a fixed permutation where there are used rotations and XOR's and then the value is passed through an fixed S-box
/// @param x unsigned 8 bit value
/// @return 8 unsigned bit value
static uint8_t permutation_q1(uint8_t x)
{
    uint8_t a0 = x / 16;
    uint8_t b0 = x % 16;

    uint8_t a1 = a0 ^ b0;
    uint8_t b1 = (a0 ^ ROR4(b0) ^ (8 * a0)) % 16;

    uint8_t a2 = t_q1[a1];
    uint8_t b2 = t_q1[b1 + 16];

    uint8_t a3 = a2 ^ b2;
    uint8_t b3 = (a2 ^ ROR4(b2) ^ (8 * a2)) % 16;

    uint8_t a4 = t_q1[a3 + 16 * 2];
    uint8_t b4 = t_q1[b3 + 16 * 3];

    return 16 * b4 + a4; // y
}

#define PACK(a, b, c, d) (d << 24 | c << 16 | b << 8 | a)

// the multiplication between a value and the matrix MDS
static uint32_t mds_column_mul(uint8_t x, int column)
{
    auto x5b = galois_mul(0x5b, x, MDS_MUL);
    auto xef = galois_mul(0xef, x, MDS_MUL);

    switch (column) {
    case 0:
        return PACK(x, x5b, xef, xef);
    case 1:
        return PACK(xef, xef, x5b, x);
    case 2:
        return PACK(x5b, xef, x, xef);
    case 3:
        return PACK(x5b, x, xef, x5b);
    default:
        throw std::runtime_error("Invalid column in mds_column_mul");
    }
}

/// @brief key-dependent S-box substitution and mixing function used for Key schedule generation
/// @param X unsigned 32 bit value
/// @param L pointer to a list of an unsigned 8 bit values
/// @param k int value used for the dimension of the key || k = 2 - 128b key || k = 3 - 192b value || k = 4 - 256b key
/// @param offset for Me is 0 and for Mo is 1
/// @return unsigned 32 bit value
static uint32_t h(uint32_t X, uint8_t const* L, int k, int offset)
{
    uint8_t y[] = { byte0(X), byte1(X), byte2(X), byte3(X) };

    if (k == 4) {
        y[0] = permutation_q1(y[0]) ^ L[4 * (6 + offset)];
        y[1] = permutation_q0(y[1]) ^ L[4 * (6 + offset) + 1];
        y[2] = permutation_q0(y[2]) ^ L[4 * (6 + offset) + 2];
        y[3] = permutation_q1(y[3]) ^ L[4 * (6 + offset) + 3];
    }

    if (k >= 3) {
        y[0] = permutation_q1(y[0]) ^ L[4 * (4 + offset)];
        y[1] = permutation_q1(y[1]) ^ L[4 * (4 + offset) + 1];
        y[2] = permutation_q0(y[2]) ^ L[4 * (4 + offset) + 2];
        y[3] = permutation_q0(y[3]) ^ L[4 * (4 + offset) + 3];
    }

    int a = 4 * (2 + offset);
    int b = 4 * offset;
    y[0] = permutation_q1(permutation_q0(permutation_q0(y[0]) ^ L[a + 0]) ^ L[b + 0]);
    y[1] = permutation_q0(permutation_q0(permutation_q1(y[1]) ^ L[a + 1]) ^ L[b + 1]);
    y[2] = permutation_q1(permutation_q1(permutation_q0(y[2]) ^ L[a + 2]) ^ L[b + 2]);
    y[3] = permutation_q0(permutation_q1(permutation_q1(y[3]) ^ L[a + 3]) ^ L[b + 3]);

    return mds_column_mul(y[0], 0) ^ mds_column_mul(y[1], 1) ^ mds_column_mul(y[2], 2) ^ mds_column_mul(y[3], 3);
}

/// @brief the keys are generated here and then used in the algorithm for whitening and the 16 rounds
/// @param X pointer to an list of unsigned 32 bit values - the whole key given by the user
/// @param k pointer to an list of unsigned 32 bit values - the values are stored here, this is the return
static void generation_subkeys(uint32_t X[8], uint32_t k[40])
{
    uint32_t Ai, Bi; // k ii 2 pt 128b, 4 256b
    uint32_t ro = 0x1010101;

    for (uint32_t index = 0; index < 20; index += 1) {
        Ai = h(2 * index * ro, (uint8_t*)X, 4, 0);
        Bi = h((2 * index + 1) * ro, (uint8_t*)X, 4, 1);

        Bi = std::rotl(Bi, 8);

        k[2 * index] = Ai + Bi;

        k[2 * index + 1] = std::rotl(Ai + 2 * Bi, 9);
    }
}

/// @brief the keys generated are used in g function
/// @param key a list of unsigned 8 bit - the whole key given by the user
/// @param S a list of unsigned 32 bit value with 4 elements
static void key_schedule(uint8_t const key[32], uint32_t S[4])
{
    for (int i = 0; i < 4; ++i) {
        uint8_t s[4] = { 0 };
        for (int j = 0; j < 8; ++j) {
            uint8_t key_byte = key[8 * i + j];
            for (int k = 0; k < 4; ++k) {
                s[k] ^= galois_mul(RS[8 * k + j], key_byte, RS_MUL);
            }
        }
        S[i] = s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
    }
}

/// @brief is the core part of the round function used to transform the 32 bit input to 32 bit output during each encryption and decryption round
/// @param X unsigned 32 bit value - part of the plaintext
/// @param L list of unsigned 8 bit with 16 values - the list of the subkeys S0, S1
/// @param k the value based on the size of the key || k = 2 - 128b key || k = 3 - 192b value || k = 4 - 256b key
/// @return unsigned 32 bit value
static uint32_t g(uint32_t X, uint8_t const L[16], int k)
{
    uint8_t y[] = { byte0(X), byte1(X), byte2(X), byte3(X) };

    if (k == 4) {
        y[0] = permutation_q1(y[0]) ^ L[4 * (3)];
        y[1] = permutation_q0(y[1]) ^ L[4 * (3) + 1];
        y[2] = permutation_q0(y[2]) ^ L[4 * (3) + 2];
        y[3] = permutation_q1(y[3]) ^ L[4 * (3) + 3];
    }

    if (k >= 3) {
        y[0] = permutation_q1(y[0]) ^ L[4 * (2)];
        y[1] = permutation_q1(y[1]) ^ L[4 * (2) + 1];
        y[2] = permutation_q0(y[2]) ^ L[4 * (2) + 2];
        y[3] = permutation_q0(y[3]) ^ L[4 * (2) + 3];
    }

    y[0] = permutation_q1(permutation_q0(permutation_q0(y[0]) ^ L[4 + 0]) ^ L[0]);
    y[1] = permutation_q0(permutation_q0(permutation_q1(y[1]) ^ L[4 + 1]) ^ L[1]);
    y[2] = permutation_q1(permutation_q1(permutation_q0(y[2]) ^ L[4 + 2]) ^ L[2]);
    y[3] = permutation_q0(permutation_q1(permutation_q1(y[3]) ^ L[4 + 3]) ^ L[3]);

    return mds_column_mul(y[0], 0) ^ mds_column_mul(y[1], 1) ^ mds_column_mul(y[2], 2) ^ mds_column_mul(y[3], 3);
}

/// @brief also called the round function is the main transformation applied to half of the data block in each encryption round
/// @param R list of unsigned 32 bit with 2 values - the input of each round
/// @param S list of unsigned 32 bit with 4 values - the S0 and S1 keys from the key_schedule
/// @param k list of unsigned 32 bit with 2 values - the specific round key from the generation_subkeys
/// @param F list of unsigned 32 bit with 2 values - the return of each round
static void F(uint32_t R[2], uint32_t S[4], uint32_t const k[2], uint32_t F[2])
{
    uint32_t T0, T1;

    T0 = g(R[0], (uint8_t*)S, 4);

    T1 = g(std::rotl(R[1], 8), (uint8_t*)S, 4);

    F[0] = T0 + T1 + k[0];
    F[1] = T0 + 2 * T1 + k[1];
}

namespace twofish {
void encrypt_block(std::uint32_t plaintext[4], std::uint32_t const key[8], std::uint32_t cipher[4])
{
    uint32_t k[40];
    uint32_t temp[8];
    uint32_t R0, R1, R2, R3, R10, R11, R12, R13;

    uint32_t S[4], tempa[4];
    uint32_t R[2], Fr[2];

    for (size_t i = 0; i < 4; ++i) {
        plaintext[i] = z3(byte0(plaintext[i])) | z2(byte1(plaintext[i])) | z1(byte2(plaintext[i])) | z0(byte3(plaintext[i]));
    }

    for (size_t i = 0; i < 8; ++i) {
        temp[i] = z3(byte0(key[i])) | z2(byte1(key[i])) | z1(byte2(key[i])) | z0(byte3(key[i]));
    }

    generation_subkeys(temp, k);

    key_schedule((uint8_t*)temp, S);

    tempa[0] = S[3];
    tempa[1] = S[2];
    tempa[2] = S[1];
    tempa[3] = S[0];

    R0 = plaintext[0] ^ k[0];
    R1 = plaintext[1] ^ k[1];
    R2 = plaintext[2] ^ k[2];
    R3 = plaintext[3] ^ k[3];

    for (size_t index = 0; index < 16; ++index) {
        R[0] = R0;
        R[1] = R1;

        F(R, tempa, &k[2 * index + 8], Fr);
        if (index == 0) {
            R10 = std::rotr(R2 ^ Fr[0], 1);
            R11 = std::rotl(R3, 1) ^ Fr[1];
        } else {
            R10 = std::rotr(R12 ^ Fr[0], 1);
            R11 = std::rotl(R13, 1) ^ Fr[1];
        }
        R12 = R0;
        R13 = R1;
        R0 = R10;
        R1 = R11;
    }

    cipher[0] = R12 ^ k[4];
    cipher[1] = R13 ^ k[5];
    cipher[2] = R10 ^ k[6];
    cipher[3] = R11 ^ k[7];

    for (size_t i = 0; i < 4; ++i) {
        cipher[i] = z3(byte0(cipher[i])) | z2(byte1(cipher[i])) | z1(byte2(cipher[i])) | z0(byte3(cipher[i]));
    }
}

void decrypt_block(std::uint32_t plaintext[4], std::uint32_t const key[8], std::uint32_t const cipher[4])
{
    uint32_t k[40], temp[8];
    uint32_t S[4], tempa[4];
    uint32_t R[2], Fr[2];
    uint32_t R0, R1, R2, R3, R10, R11, R12, R13;

    for (int i = 0; i < 8; ++i) {
        temp[i] = z3(byte0(key[i])) | z2(byte1(key[i])) | z1(byte2(key[i])) | z0(byte3(key[i]));
    }

    generation_subkeys(temp, k);

    key_schedule((uint8_t*)temp, S);

    tempa[0] = S[3];
    tempa[1] = S[2];
    tempa[2] = S[1];
    tempa[3] = S[0];

    uint32_t c0 = z3(byte0(cipher[0])) | z2(byte1(cipher[0])) | z1(byte2(cipher[0])) | z0(byte3(cipher[0]));
    uint32_t c1 = z3(byte0(cipher[1])) | z2(byte1(cipher[1])) | z1(byte2(cipher[1])) | z0(byte3(cipher[1]));
    uint32_t c2 = z3(byte0(cipher[2])) | z2(byte1(cipher[2])) | z1(byte2(cipher[2])) | z0(byte3(cipher[2]));
    uint32_t c3 = z3(byte0(cipher[3])) | z2(byte1(cipher[3])) | z1(byte2(cipher[3])) | z0(byte3(cipher[3]));

    R10 = c2 ^ k[6];
    R11 = c3 ^ k[7];
    R12 = c0 ^ k[4];
    R13 = c1 ^ k[5];

    for (int round = 15; round >= 0; --round) {
        R[0] = R12;
        R[1] = R13;

        F(R, tempa, &k[8 + 2 * round], Fr);

        uint32_t newR2 = std::rotl(R10, 1) ^ Fr[0];
        uint32_t newR3 = std::rotr(R11 ^ Fr[1], 1);

        R10 = R12;
        R11 = R13;
        R12 = newR2;
        R13 = newR3;
    }

    R0 = R10 ^ k[0];
    R1 = R11 ^ k[1];
    R2 = R12 ^ k[2];
    R3 = R13 ^ k[3];

    plaintext[0] = R0;
    plaintext[1] = R1;
    plaintext[2] = R2;
    plaintext[3] = R3;
}
}

// NOLINTEND(bugprone-implicit-widening-of-multiplication-result)
// NOLINTEND(readability-math-missing-parentheses)
// NOLINTEND(readability-isolate-declaration)

#if 0 // NOLINT(readability-avoid-unconditional-preprocessor-if)
int main()
{
    uint32_t Key[8] = { 0xD43BB755, 0x6EA32E46, 0xF2A282B7, 0xD45B4E0D, 0x57FF739D, 0x4DC92C1B, 0xD7FC0170, 0x0CC8216F};

    uint32_t plaintext[4] = { 0x90AFE91B, 0xB288544F, 0x2C32DC23, 0x9B2635E6};

    uint32_t cipher[4];

    for(int i = 0; i < 4;++i){
        printf("\nplaintext = 0x%x", plaintext[i]);
    }
    printf("\n");
    block_encryption(plaintext, Key, cipher);
    for(int i = 0; i < 4;++i){
        printf("\ncipher = 0x%x", cipher[i]);
    }
    printf("\n");
    block_decryption(plaintext, Key, cipher);

    for(int i = 0; i < 4;++i){
        printf("\nplaintext = 0x%x", plaintext[i]);
    }

    return 0;
}
#endif

#if IS_TESTING

TEST_CASE("I=1/pg67")
{
    uint32_t key[8] = { 0x0 };
    uint32_t plaintext_initial[4] = { 0x0 };
    uint32_t plaintext_final[4];
    uint32_t cipher[4];
    uint32_t cipher_test[] = { 0x57FF739D, 0x4DC92C1B, 0xD7FC0170, 0x0CC8216F };

    twofish::encrypt_block(plaintext_initial, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(cipher[i] == cipher_test[i]);
    }
    twofish::decrypt_block(plaintext_final, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(plaintext_initial[i] == plaintext_final[i]);
    }
}

TEST_CASE("I=2/pg67")
{
    uint32_t key[8] = { 0x0 };
    uint32_t plaintext_initial[4] = { 0x57FF739D, 0x4DC92C1B, 0xD7FC0170, 0x0CC8216F };
    uint32_t plaintext_final[4];
    uint32_t cipher[4];
    uint32_t cipher_test[4] = { 0xD43BB755, 0x6EA32E46, 0xF2A282B7, 0xD45B4E0D };

    twofish::encrypt_block(plaintext_initial, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(cipher[i] == cipher_test[i]);
    }
    twofish::decrypt_block(plaintext_final, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(plaintext_initial[i] == plaintext_final[i]);
    }
}

TEST_CASE("I=3/pg67")
{
    uint32_t key[8] = { 0x57FF739D, 0x4DC92C1B, 0xD7FC0170, 0x0CC8216F, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
    uint32_t plaintext_initial[4] = { 0xD43BB755, 0x6EA32E46, 0xF2A282B7, 0xD45B4E0D };
    uint32_t plaintext_final[4];
    uint32_t cipher[4];
    uint32_t cipher_test[4] = { 0x90AFE91B, 0xB288544F, 0x2C32DC23, 0x9B2635E6 };

    twofish::encrypt_block(plaintext_initial, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(cipher[i] == cipher_test[i]);
    }
    twofish::decrypt_block(plaintext_final, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(plaintext_initial[i] == plaintext_final[i]);
    }
}

TEST_CASE("I=4/pg67")
{
    uint32_t key[8] = { 0xD43BB755, 0x6EA32E46, 0xF2A282B7, 0xD45B4E0D, 0x57FF739D, 0x4DC92C1B, 0xD7FC0170, 0x0CC8216F };
    uint32_t plaintext_initial[4] = { 0x90AFE91B, 0xB288544F, 0x2C32DC23, 0x9B2635E6 };
    uint32_t plaintext_final[4];
    uint32_t cipher[4];
    uint32_t cipher_test[4] = { 0x6CB4561C, 0x40BF0A97, 0x05931CB6, 0xD408E7FA };
    twofish::encrypt_block(plaintext_initial, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(cipher[i] == cipher_test[i]);
    }
    twofish::decrypt_block(plaintext_final, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(plaintext_initial[i] == plaintext_final[i]);
    }
}

TEST_CASE("I=5/pg67")
{
    uint32_t key[8] = { 0x90AFE91B, 0xB288544F, 0x2C32DC23, 0x9B2635E6, 0xD43BB755, 0x6EA32E46, 0xF2A282B7, 0xD45B4E0D };
    uint32_t plaintext_initial[4] = { 0x6CB4561C, 0x40BF0A97, 0x05931CB6, 0xD408E7FA };
    uint32_t plaintext_final[4];
    uint32_t cipher[4];
    uint32_t cipher_test[4] = { 0x3059D6D6, 0x1753B958, 0xD92F4781, 0xC8640E58 };

    twofish::encrypt_block(plaintext_initial, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(cipher[i] == cipher_test[i]);
    }
    twofish::decrypt_block(plaintext_final, key, cipher);
    for (size_t i = 0; i < 4; ++i) {
        REQUIRE(plaintext_initial[i] == plaintext_final[i]);
    }
}

#endif
