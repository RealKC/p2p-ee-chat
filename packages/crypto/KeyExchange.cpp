#include "KeyExchange.hpp"

#include "common.hpp"
#include <array>
#include <boost/multiprecision/miller_rabin.hpp>
#include <boost/random.hpp>
#include <cstdint>
#include <stdexcept>

using namespace boost::multiprecision;
using namespace boost::random;

// See: RFC 3526 (https://datatracker.ietf.org/doc/rfc3526/)
// clang-format off
static std::uint8_t rfc3526_modulus[] = {
    255, 255, 255, 255, 255, 255, 255, 255,
    104, 170, 172, 138,  90, 142, 114,  21,
     16,   5, 250, 152,  24,  38, 210,  21,
    229, 106, 149, 234, 124,  73, 149,  57,
     24,  23,  88, 149, 246, 203,  43, 222,
    201,  82,  76, 111, 240,  93, 197, 181,
    143, 162,   7, 236, 162, 131,  39, 155,
      3, 134,  14,  24,  44, 119, 158, 227,
     59, 206,  54,  46,  70,  94, 144,  50,
    124,  33,  24, 202,   8, 108, 116, 241,
      4, 152, 188,  74,  78,  53,  12, 103,
    109, 150, 150, 112,   7,  41, 213, 158,
    187,  82, 133,  32,  86, 243,  98,  28,
    150, 173, 163, 220,  35,  93, 101, 131,
     95, 207,  36, 253, 168,  63,  22, 105,
    154, 211,  85,  28,  54,  72, 218, 152,
      5, 191,  99, 161, 184, 124,   0, 194,
     61,  91, 228, 236,  81, 102,  40,  73,
    230,  31,  75, 124,  17,  36, 159, 174,
    165, 159, 137,  90, 251, 107,  56, 238,
    237, 183,   6, 244, 182,  92, 255,  11,
    107, 237,  55, 166, 233,  66,  76, 244,
    198, 126,  94,  98, 118, 181, 133, 228,
     69, 194,  81, 109, 109,  53, 225,  79,
     55,  20,  95, 242, 109,  10,  43,  48,
     27,  67,  58, 205, 179,  25, 149, 239,
    221,   4,  52, 142, 121,   8,  74,  81,
     34, 155,  19,  59, 166, 190,  11,   2,
    116, 204, 103, 138,   8,  78,   2,  41,
    209,  28, 220, 128, 139,  98, 198, 196,
     52, 194, 104,   33, 162, 218, 15, 201,
    255, 255, 255, 255, 255, 255, 255, 255
};
// clang-format on

PublicParams::PublicParams()
    : base(2)
{
    import_bits(modulus, std::begin(rfc3526_modulus), std::end(rfc3526_modulus), 0, false);
}

static PublicParams public_params;

u2048 generate_private_key()
{
    mt11213b base_gen(clock());
    independent_bits_engine<mt11213b, 2048, u2048> generator(base_gen);
    boost::random::uniform_int_distribution<u2048> dist(u2048(0), public_params.modulus - 1);

    mt19937 test_gen(clock());

    for (int i = 0; i < 10'000; ++i) {
        auto candidate = dist(generator);

        if (miller_rabin_test(candidate, 25, test_gen)) {
            return candidate;
        }
    }

    throw std::runtime_error("failed to generate private key");
}

u2048 compute_public_key(u2048 private_key)
{
    return powm(public_params.base, private_key, public_params.modulus);
}

u2048 compute_common_secret(u2048 public_key, u2048 private_key)
{
    return powm(public_key, private_key, public_params.modulus);
}

#if IS_TESTING

TEST_CASE("correctly generates private keys")
{
    auto alice_private = generate_private_key();
    auto alice_public = compute_public_key(alice_private);

    auto bob_private = generate_private_key();
    auto bob_public = compute_public_key(bob_private);

    auto alice_secret = compute_common_secret(bob_public, alice_private);
    auto bob_secret = compute_common_secret(alice_public, bob_private);

    REQUIRE(alice_secret == bob_secret);
}

#endif
