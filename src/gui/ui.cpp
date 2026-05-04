/**
 * @file ui.cpp
 * @brief Реализация интерфейса Sigma Encryptor с замером производительности
 */

#include "ui.hpp"
#include "theme.hpp"
#include "file_dialog.hpp"

#include <iostream>
#include <cstring> 
#include <chrono>   // Для мониторинга скорости операций

#include "imgui.h"

namespace sigma
{

// =============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// =============================================================================

UI::UI()
    : m_encryptor(std::make_unique<Encryptor>())
    , m_operationComplete(false)
    , m_lastResult(EncryptionResult::Success)
{
    std::cout << "[ИНФО] UI инициализирован" << std::endl;
}

UI::~UI()
{
    // Финальная зачистка памяти
    std::memset(m_password, 0, sizeof(m_password));
    std::memset(m_passwordConfirm, 0, sizeof(m_passwordConfirm));
}

// =============================================================================
// СЛУЖЕБНЫЕ МЕТОДЫ
// =============================================================================

void UI::initialize()
{
    Theme::applyObsidianTheme();
}

void UI::reset()
{
    m_filePath.clear();
    std::memset(m_password, 0, sizeof(m_password));
    std::memset(m_passwordConfirm, 0, sizeof(m_passwordConfirm));
    std::memset(m_filePathBuffer, 0, sizeof(m_filePathBuffer));
    m_selectedOperation = 0;
    m_operationComplete = false;
    m_statusText = "Готов к работе";
    m_logMessages.clear();
}

void UI::setFilePath(const std::string& path)
{
    m_filePath = path;
    std::strncpy(m_filePathBuffer, path.c_str(), sizeof(m_filePathBuffer) - 1);
}

void UI::addLogMessage(const std::string& message)
{
    // Добавляем метку времени для удобства отладки
    m_logMessages.push_back("[" + std::string(__TIME__) + "] " + message);
    if (m_logMessages.size() > 100)
    {
        m_logMessages.erase(m_logMessages.begin());
    }
}

// =============================================================================
// РЕНДЕРИНГ
// =============================================================================

void UI::render()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize); // Адаптация под размер окна приложения

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    if (ImGui::Begin("Sigma Main", nullptr, flags))
    {
        // --- ЗАГОЛОВОК ---
        ImGui::SetCursorPosY(20);
        ImGui::PushFont(io.Fonts->Fonts[0]); // Предполагаем наличие кастомного шрифта
        float centerX = (ImGui::GetWindowWidth() - ImGui::CalcTextSize("SIGMA ENCRYPTOR").x) * 0.5f;
        ImGui::SetCursorPosX(centerX);
        ImGui::TextColored(ImVec4(0.46f, 0.54f, 0.63f, 1.0f), "SIGMA ENCRYPTOR");
        ImGui::PopFont();
        
        ImGui::Separator();
        ImGui::Spacing();

        // --- ВЫБОР ФАЙЛА ---
        ImGui::Text("Файл для обработки:");
        ImGui::PushItemWidth(-110);
        ImGui::InputText("##path", m_filePathBuffer, sizeof(m_filePathBuffer), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("ОБЗОР", ImVec2(100, 0)))
        {
            std::string path = FileDialog::openFile("Select File", {{"All Files", "*.*"}});
            if (!path.empty()) setFilePath(path);
        }

        ImGui::Spacing();
        
        // --- ПАРАМЕТРЫ ---
        if (ImGui::BeginTable("Settings", 2))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Режим:");
            ImGui::RadioButton("Шифрование", &m_selectedOperation, 0); ImGui::SameLine();
            ImGui::RadioButton("Дешифрование", &m_selectedOperation, 1);

            ImGui::TableSetColumnIndex(1);
            if (m_selectedOperation == 0) {
                ImGui::Text("Алгоритм:");
                const char* items[] = { "AES-256-GCM", "ChaCha20-Poly1305" };
                int currentItem = (m_selectedCipher == CipherType::AES_256_GCM) ? 0 : 1;
                if (ImGui::Combo("##cipher", &currentItem, items, IM_ARRAYSIZE(items)))
                    m_selectedCipher = (currentItem == 0) ? CipherType::AES_256_GCM : CipherType::ChaCha20_Poly1305;
            }
            ImGui::EndTable();
        }

        // --- ПАРОЛИ ---
        ImGui::Text("Криптографический ключ (пароль):");
        ImGui::InputText("##pw", m_password, sizeof(m_password), ImGuiInputTextFlags_Password);
        
        if (m_selectedOperation == 0) {
            ImGui::Text("Подтверждение ключа:");
            ImGui::InputText("##pwc", m_passwordConfirm, sizeof(m_passwordConfirm), ImGuiInputTextFlags_Password);
            if (strlen(m_password) > 0 && strcmp(m_password, m_passwordConfirm) != 0)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Внимание: ключи не совпадают!");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- КНОПКА ДЕЙСТВИЯ ---
        bool ready = canPerformOperation();
        if (!ready) ImGui::BeginDisabled();
        
        if (ImGui::Button(m_selectedOperation == 0 ? "ЗАПУСТИТЬ ШИФРОВАНИЕ" : "ЗАПУСТИТЬ ДЕШИФРОВАНИЕ", ImVec2(-1, 45)))
        {
            if (m_selectedOperation == 0) performEncryption();
            else performDecryption();
        }
        
        if (!ready) ImGui::EndDisabled();

        // --- ЛОГ ---
        ImGui::Spacing();
        ImGui::TextDisabled("ЛОГ КОНСОЛИ:");
        if (ImGui::BeginChild("LogRegion", ImVec2(0, -30), true))
        {
            for (const auto& msg : m_logMessages)
                ImGui::TextUnformatted(msg.c_str());
            
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        // --- СТАТУС ---
        ImGui::TextDisabled("Статус:"); ImGui::SameLine();
        ImGui::Text("%s", m_statusText.c_str());
    }
    ImGui::End();
}

// =============================================================================
// ЛОГИКА
// =============================================================================

bool UI::canPerformOperation() const
{
    if (m_filePath.empty() || strlen(m_password) < 4) return false;
    if (m_selectedOperation == 0 && strcmp(m_password, m_passwordConfirm) != 0) return false;
    return true;
}

void UI::performEncryption()
{
    addLogMessage(">>> Инициализация шифрования...");
    std::string out = m_filePath + ".sge";
    
    auto start = std::chrono::high_resolution_clock::now();
    EncryptionResult res = m_encryptor->encryptFile(m_filePath, out, m_password, m_selectedCipher);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Затираем пароли в буферах
    std::memset(m_password, 0, sizeof(m_password));
    std::memset(m_passwordConfirm, 0, sizeof(m_passwordConfirm));

    if (res == EncryptionResult::Success) {
        addLogMessage("УСПЕХ: Файл зашифрован за " + std::to_string(diff) + " ms");
        m_statusText = "Готово (OK)";
    } else {
        addLogMessage("ОШИБКА: Операция прервана");
        m_statusText = "Ошибка выполнения";
    }
}

void UI::performDecryption()
{
    addLogMessage(">>> Инициализация дешифрования...");
    std::string out = m_filePath;

    // Замена ends_with на проверку через длину и сравнение подстроки
    std::string suffix = ".sge";
    if (out.length() >= suffix.length() && 
        out.compare(out.length() - suffix.length(), suffix.length(), suffix) == 0) 
    {
        out.erase(out.size() - suffix.length());
    }
    else 
    {
        out += ".dec";
    }

    auto start = std::chrono::high_resolution_clock::now();
    EncryptionResult res = m_encryptor->decryptFile(m_filePath, out, m_password);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Безопасная очистка
    std::memset(m_password, 0, sizeof(m_password));

    if (res == EncryptionResult::Success) {
        addLogMessage("УСПЕХ: Данные восстановлены за " + std::to_string(diff) + " ms");
        m_statusText = "Дешифровано успешно";
    } else {
        addLogMessage("ОШИБКА: Неверный ключ или повреждение структуры");
        m_statusText = "Ошибка доступа";
    }
}

} // namespace sigma