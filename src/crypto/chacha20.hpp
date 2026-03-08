/**
 * @file chacha20.hpp
 * @brief Заголовочный файл для ChaCha20-Poly1305 шифрования
 * 
 * ChaCha20-Poly1305 - современный потоковый шифр с аутентификацией.
 * 
 * @author Sigma Team
 * @version 1.0
 */

#ifndef SIGMA_CHACHA20_ENCRYPTOR_HPP
#define SIGMA_CHACHA20_ENCRYPTOR_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "encryptor.hpp"

#include <openssl/evp.h>

namespace sigma
{

class ChaCha20Encryptor
{
public:
    ChaCha20Encryptor();
    ~ChaCha20Encryptor();

    EncryptionResult encrypt(
        const std::vector<unsigned char>& plaintext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv,
        std::vector<unsigned char>& ciphertext
    );

    EncryptionResult decrypt(
        const std::vector<unsigned char>& ciphertext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv,
        std::vector<unsigned char>& plaintext
    );

private:
    void handleError(const std::string& operation) const;
    EVP_CIPHER_CTX* m_context = nullptr;
    static constexpr size_t TAG_SIZE = 16;
};

} // namespace sigma

#endif // SIGMA_CHACHA20_ENCRYPTOR_HPP
