/**
 * @file chacha20.hpp
 * @brief Интерфейс шифрования ChaCha20-Poly1305.
 * 
 * Данный модуль реализует алгоритм AEAD (Authenticated Encryption with Associated Data),
 * объединяющий потоковый шифр ChaCha20 и имитовставку Poly1305. 
 * Это современная и безопасная альтернатива блочным шифрам вроде AES.
 * 
 * @author heimdall
 */

#ifndef SIGMA_CHACHA20_ENCRYPTOR_HPP
#define SIGMA_CHACHA20_ENCRYPTOR_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "encryptor.hpp" // Содержит общие коды возврата проекта

/**
 * Forward declaration для OpenSSL структур.
 * 
 * Мы объявляем EVP_CIPHER_CTX здесь, чтобы избежать включения <openssl/evp.h> 
 * в глобальную область видимости. Это сокращает время компиляции проекта и 
 * предотвращает засорение пространства имен сторонними библиотеками.
 */
extern "C" {
    typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
}

namespace sigma
{

/**
 * @class ChaCha20Encryptor
 * @brief Высокопроизводительная реализация шифрования ChaCha20-Poly1305.
 * 
 * Класс инкапсулирует логику работы с контекстом OpenSSL, обеспечивая автоматическое
 * управление памятью через деструктор (паттерн RAII). Потокобезопасен на уровне 
 * отдельных экземпляров.
 */
class ChaCha20Encryptor
{
public:
    /**
     * @name Параметры алгоритма
     * Согласно RFC 8439, данные размеры являются фиксированными для данного шифра.
     */
    ///@{
    static constexpr size_t KEY_SIZE = 32; ///< Ключ 256 бит для максимальной безопасности.
    static constexpr size_t IV_SIZE  = 12; ///< 96-битный Nonce (число, используемое один раз).
    static constexpr size_t TAG_SIZE = 16; ///< 128-битный тег аутентификации Poly1305.
    ///@}

    /**
     * @brief Конструктор: выделяет память под криптографический контекст.
     */
    ChaCha20Encryptor();

    /**
     * @brief Деструктор: безопасно затирает и освобождает контекст шифрования.
     */
    ~ChaCha20Encryptor();

    /**
     * @brief Запрет копирования.
     * 
     * Копирование объекта привело бы к тому, что два объекта пытались бы управлять 
     * одним и тем же указателем m_context, что вызвало бы крах программы (double-free).
     */
    ChaCha20Encryptor(const ChaCha20Encryptor&) = delete;
    ChaCha20Encryptor& operator=(const ChaCha20Encryptor&) = delete;

    /**
     * @brief Поддержка семантики перемещения.
     * Позволяет эффективно возвращать объект из функций или хранить в контейнерах.
     */
    ChaCha20Encryptor(ChaCha20Encryptor&& other) noexcept;
    ChaCha20Encryptor& operator=(ChaCha20Encryptor&& other) noexcept;

    /**
     * @brief Шифрует данные и вычисляет проверочный тег.
     * 
     * @param[in] plaintext Исходные данные.
     * @param[in] key Ключ длиной KEY_SIZE.
     * @param[in] iv Уникальный вектор инициализации длиной IV_SIZE.
     * @param[out] ciphertext Результирующий массив: [зашифрованные данные][тег 16 байт].
     * @return EncryptionResult::Success при успешном завершении.
     */
    [[nodiscard]] EncryptionResult encrypt(
        const std::vector<unsigned char>& plaintext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv,
        std::vector<unsigned char>& ciphertext
    );

    /**
     * @brief Дешифрует данные и проверяет целостность через Poly1305.
     * 
     * В процессе работы алгоритм сравнивает вычисленный тег с тем, что хранится 
     * в конце ciphertext. Если они не совпадают, данные считаются поврежденными.
     * 
     * @return EncryptionResult::InvalidPassword при нарушении целостности или неверном ключе.
     */
    [[nodiscard]] EncryptionResult decrypt(
        const std::vector<unsigned char>& ciphertext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv,
        std::vector<unsigned char>& plaintext
    );

private:
    /**
     * @brief Обработка и вывод детальной информации об ошибках OpenSSL.
     */
    void handleError(const std::string& operation) const;

    /// Внутренний контекст OpenSSL для работы с шифром.
    EVP_CIPHER_CTX* m_context = nullptr;
};

} // namespace sigma

#endif // SIGMA_CHACHA20_ENCRYPTOR_HPP