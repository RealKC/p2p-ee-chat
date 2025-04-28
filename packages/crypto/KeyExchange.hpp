#pragma once

#include <boost/multiprecision/cpp_int.hpp>

using u2048 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<2048, 2048, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;

struct PublicParams {
    u2048 modulus;
    u2048 base;

    PublicParams();
};

u2048 generate_private_key();

u2048 compute_public_key(u2048 const& private_key);
u2048 compute_common_secret(u2048 const& public_key, u2048 const& private_key);
