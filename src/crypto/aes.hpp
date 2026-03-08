/**
 * @file aes.hpp
 * @brief Заголовочный файл для AES-256-GCM шифрования
 * 
 * @details
 * AES-256-GCM - симметричный блочный шифр с аутентификацией.
 * 
 * @author Sigma Team
 * @version 1.0
 */

#ifndef SIGMA_AES_ENCRYPTOR_HPP
#define SIGMA_AES_ENCRYPTOR_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "encryptor.hpp"

#include <openssl/evp.h>

namespace sigma
{

class AESEncryptor
{
public:
    AESEncryptor();
    ~AESEncryptor();

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

#endif // SIGMA_AES_ENCRYPTOR_HPP
