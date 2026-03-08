/**
 * @file ui.hpp
 * @brief Заголовочный файл пользовательского интерфейса
 */

#ifndef SIGMA_UI_HPP
#define SIGMA_UI_HPP

#include <string>
#include <vector>
#include <memory>
#include "../crypto/encryptor.hpp"

namespace sigma
{

class UI
{
public:
    UI();
    ~UI();
    void initialize();
    void render();
    void setFilePath(const std::string& path);
    void reset();

private:
    void renderMainWindow();
    void renderFilePanel();
    void renderPasswordPanel();
    void renderAlgorithmPanel();
    void renderActionButtons();
    void renderLogPanel();
    void performEncryption();
    void performDecryption();
    bool canPerformOperation() const;

    std::unique_ptr<Encryptor> m_encryptor;
    std::string m_filePath;
    char m_password[256] = "";
    char m_passwordConfirm[256] = "";
    CipherType m_selectedCipher = CipherType::AES_256_GCM;
    int m_selectedOperation = 0;
    std::vector<std::string> m_logMessages;
    bool m_operationComplete = false;
    EncryptionResult m_lastResult = EncryptionResult::Success;
    std::string m_statusText = "Выберите файл для работы";
    char m_filePathBuffer[512] = "";

    // Добавляет сообщение в лог
    void addLogMessage(const std::string& message);
};

} // namespace sigma

#endif // SIGMA_UI_HPP
