/**
 * @file aes.hpp
 * @brief Интерфейс модуля AES-256-GCM.
 * 
 * Данный файл описывает класс-обертку над OpenSSL EVP для обеспечения
 * аутентифицированного шифрования. Выбранный режим GCM (Galois/Counter Mode)
 * является стандартом в современной криптографии (TLS 1.3, SSH).
 * 
 * @author heimdall
 */

#ifndef SIGMA_AES_ENCRYPTOR_HPP
#define SIGMA_AES_ENCRYPTOR_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "encryptor.hpp" // Содержит общее перечисление EncryptionResult

/**
 * Forward declaration для структуры OpenSSL.
 * 
 * Вместо того чтобы заставлять каждый файл, включающий aes.hpp, парсить тысячи строк
 * из <openssl/evp.h>, мы просто говорим, что такая структура существует.
 * Это ускоряет компиляцию и предотвращает конфликты.
 */
extern "C" {
    typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
}

namespace sigma
{

/**
 * @class AESEncryptor
 * @brief Реализация стандарта AES (Advanced Encryption Standard) с длиной ключа 256 бит.
 * 
 * Класс реализует концепцию RAII (Resource Acquisition Is Initialization): 
 * создание объекта выделяет криптографический контекст, а деструктор — гарантированно 
 * очищает его, предотвращая утечки памяти в долгоживущих приложениях.
 */
class AESEncryptor
{
public:
    /**
     * @name Криптографические параметры
     * Значения фиксированы согласно спецификации AES-256 и рекомендациям NIST для GCM.
     */
    ///@{
    static constexpr size_t KEY_SIZE = 32; ///< 256 бит: максимальная стойкость в гражданской криптографии.
    static constexpr size_t IV_SIZE  = 12; ///< 96 бит: рекомендуемый размер IV для предотвращения коллизий в GCM.
    static constexpr size_t TAG_SIZE = 16; ///< 128 бит: тег аутентификации для проверки целостности данных.
    ///@}

    /**
     * @brief Конструктор: инициализирует внутренние структуры OpenSSL.
     * @throws std::runtime_error если не удалось выделить системные ресурсы.
     */
    AESEncryptor();

    /**
     * @brief Деструктор: безопасно уничтожает контекст шифрования.
     */
    ~AESEncryptor();

    /**
     * @brief Запрет копирования.
     * 
     * Мы запрещаем копирование, так как каждый объект владеет уникальным указателем m_context.
     * Это защищает от ошибки "double free" (двойного освобождения памяти).
     */
    AESEncryptor(const AESEncryptor&) = delete;
    AESEncryptor& operator=(const AESEncryptor&) = delete;

    /**
     * @brief Семантика перемещения (Move semantics).
     * Позволяет эффективно передавать объект (например, из фабричного метода) без копирования.
     */
    AESEncryptor(AESEncryptor&& other) noexcept;
    AESEncryptor& operator=(AESEncryptor&& other) noexcept;

    /**
     * @brief Шифрует сырые данные.
     * 
     * @param[in] plaintext Данные для защиты.
     * @param[in] key Ключ шифрования (должен быть KEY_SIZE).
     * @param[in] iv Вектор инициализации (должен быть IV_SIZE).
     * @param[out] ciphertext Буфер, куда будет записан результат + тег аутентификации в конце.
     * @return EncryptionResult::Success в случае успеха.
     */
    [[nodiscard]] EncryptionResult encrypt(
        const std::vector<unsigned char>& plaintext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv,
        std::vector<unsigned char>& ciphertext
    );

    /**
     * @brief Расшифровывает данные и проверяет их подлинность.
     * 
     * Метод сначала извлекает тег из конца ciphertext, проверяет его соответствие
     * данным и ключу, и только при успехе возвращает расшифрованные данные.
     * 
     * @return EncryptionResult::InvalidPassword при ошибке тега (неверный ключ или битые данные).
     */
    [[nodiscard]] EncryptionResult decrypt(
        const std::vector<unsigned char>& ciphertext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv,
        std::vector<unsigned char>& plaintext
    );

private:
    /**
     * @brief Внутренняя обработка системных ошибок крипто-стека.
     */
    void handleError(const std::string& operation) const;

    /// Указатель на непрозрачную структуру контекста OpenSSL.
    EVP_CIPHER_CTX* m_context = nullptr;
};

} // namespace sigma

#endif // SIGMA_AES_ENCRYPTOR_HPP