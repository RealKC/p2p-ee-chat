#include "KeyExchange.hpp"
#include "common.hpp"
#include "twofish.hpp"
#include <array>
#include <cstdint>
#include <cstring>

static constexpr auto BLOCK_SIZE = 16;

struct DHKey {
    u2048 key;
    std::vector<std::uint8_t> as_bytes;

    DHKey(u2048 key)
        : key { key }
    {
        using namespace boost::multiprecision;

        export_bits(key, std::back_inserter(as_bytes), 8);
    }

    DHKey(std::vector<std::uint8_t> const& as_bytes)
        : as_bytes { as_bytes }
    {
        using namespace boost::multiprecision;

        import_bits(key, as_bytes.begin(), as_bytes.end(), 8);
    }

    static DHKey make_private()
    {
        return { generate_private_key() };
    }

    DHKey compute_public() const
    {
        return { compute_public_key(key) };
    }

    emscripten::val bytes() const
    {
        return emscripten::val(emscripten::typed_memory_view(as_bytes.size(), as_bytes.data()));
    }

    static DHKey from_bytes(std::vector<std::uint8_t> const& vec)
    {
        return { vec };
    }
};

struct EncryptionKey {
    std::uint32_t key[8];

    static EncryptionKey from_dh(DHKey const& mine, DHKey const& theirs)
    {
        std::cout << "mine: " << std::hex;
        for (auto k : mine.as_bytes)
            std::cout << (std::uint32_t)k;
        std::cout << std::hex << std::endl;

        std::cout << "theirs: " << std::hex;
        for (auto k : theirs.as_bytes)
            std::cout << (std::uint32_t)k;
        std::cout << std::hex << std::endl;

        using namespace boost::multiprecision;

        auto common = compute_common_secret(theirs.key, mine.key);

        std::vector<std::uint32_t> raw_key;
        export_bits(common, std::back_inserter(raw_key), 32);

        EncryptionKey key;
        for (int i = 0; i < 8; ++i) {
            key.key[i] = raw_key[i];
        }

        return key;
    }
};

static std::string decrypt_message(EncryptionKey key, std::vector<std::uint8_t> cypher_text);

static std::vector<std::uint8_t> encrypt_message(EncryptionKey key, std::string message)
{
    std::cout << "decryption key: " << std::hex;
    for (auto k : key.key)
        std::cout << k;
    std::cout << std::hex << std::endl;

    std::vector<std::uint8_t> padded_message;
    padded_message.reserve(message.length());

    std::vector<std::uint8_t> encrypted;

    std::uint32_t plaintext[4];
    std::uint32_t cypher[4];

    for (auto& ch : message) {
        padded_message.push_back(ch);
    }

    while (!(padded_message.size() % BLOCK_SIZE == 0)) {
        padded_message.push_back(0);
    }

    std::size_t i = 0;

    std::cout << "padded message length " << padded_message.size() << std::endl;

    while (i < padded_message.size()) {
        std::memset(plaintext, 0, BLOCK_SIZE);

        for (std::size_t j = 0; j < 4; ++j) {
            auto elem_idx = i + (j * 4);
            plaintext[j] |= (padded_message[elem_idx + 0] << 24);
            plaintext[j] |= (padded_message[elem_idx + 1] << 16);
            plaintext[j] |= (padded_message[elem_idx + 2] << 8);
            plaintext[j] |= (padded_message[elem_idx + 3]);
        }

        twofish::encrypt_block(plaintext, key.key, cypher);

        for (auto c : cypher) {
            encrypted.push_back((c >> 24) & 0xff);
            encrypted.push_back((c >> 16) & 0xff);
            encrypted.push_back((c >> 8) & 0xff);
            encrypted.push_back((c >> 0) & 0xff);
        }

        i += BLOCK_SIZE;
    }

    std::cout << "cypher text length: " << encrypted.size() << std::endl;
    std::cout << "encrypted: \"" << message << "\" and roundtripped into: \"" << decrypt_message(key, encrypted) << "\"\n";

    return encrypted;
}

static std::string decrypt_message(EncryptionKey key, std::vector<std::uint8_t> cypher_text)
{
    std::string decrypted;

    std::uint32_t plaintext[4];
    std::uint32_t cypher[4];

    std::size_t i = 0;

    while (i < cypher_text.size()) {
        std::memset(cypher, 0, BLOCK_SIZE);

        for (std::size_t j = 0; j < 4; ++j) {
            auto elem_idx = i + (j * 4);
            cypher[j] |= (static_cast<std::uint32_t>(cypher_text[elem_idx + 0] << 24));
            cypher[j] |= (static_cast<std::uint32_t>(cypher_text[elem_idx + 1] << 16));
            cypher[j] |= (static_cast<std::uint32_t>(cypher_text[elem_idx + 2] << 8));
            cypher[j] |= (static_cast<std::uint32_t>(cypher_text[elem_idx + 3]));
        }

        twofish::decrypt_block(plaintext, key.key, cypher);

        for (auto c : plaintext) {
            decrypted.push_back(static_cast<char>(c >> 0));
            decrypted.push_back(static_cast<char>(c >> 8));
            decrypted.push_back(static_cast<char>(c >> 16));
            decrypted.push_back(static_cast<char>(c >> 24));
        }

        i += BLOCK_SIZE;
    }

    return decrypted;
}

#if IS_WASM
EMSCRIPTEN_BINDINGS(CryptLib)
{
    using namespace emscripten;

    register_vector<std::uint8_t>("CryptoU8Vec");

    class_<DHKey>("DHKey")
        .function("computePublic", &DHKey::compute_public)
        .function("bytes", &DHKey::bytes)
        .class_function("fromBytes", &DHKey::from_bytes)
        .class_function("makePrivate", &DHKey::make_private);

    class_<EncryptionKey>("EncryptionKey")
        .class_function("fromDiffieHellman", &EncryptionKey::from_dh);

    function("encryptMessage", &encrypt_message);
    function("decryptMessage", decrypt_message);
}
#endif
