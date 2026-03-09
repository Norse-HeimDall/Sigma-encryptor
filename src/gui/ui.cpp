/**
 * @file ui.cpp
 * @brief Реализация пользовательского интерфейса Sigma Encryptor
 */

#include "ui.hpp"
#include "theme.hpp"
#include "file_dialog.hpp"

#include <iostream>
#include <cstring>

// ImGui - библиотека GUI
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
    std::cout << "[ИНФО] UI создан" << std::endl;
}

UI::~UI()
{
    std::cout << "[ИНФО] UI уничтожен" << std::endl;
}

// =============================================================================
// ИНИЦИАЛИЗАЦИЯ
// =============================================================================

void UI::initialize()
{
    // Применяем тему Obsidian
    Theme::applyObsidianTheme();
    std::cout << "[ИНФО] UI инициализирован" << std::endl;
}

void UI::reset()
{
    m_filePath.clear();
    std::memset(m_password, 0, sizeof(m_password));
    std::memset(m_passwordConfirm, 0, sizeof(m_passwordConfirm));
    std::memset(m_filePathBuffer, 0, sizeof(m_filePathBuffer));
    m_selectedOperation = 0;
    m_operationComplete = false;
    m_lastResult = EncryptionResult::Success;
    m_statusText = "Выберите файл для работы";
}

void UI::setFilePath(const std::string& path)
{
    m_filePath = path;
    std::strncpy(m_filePathBuffer, path.c_str(), sizeof(m_filePathBuffer) - 1);
}

// =============================================================================
// ОТРИСОВКА ИНТЕРФЕЙСА
// =============================================================================

void UI::render()
{
    // Устанавливаем позицию и размер окна на весь экран
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    
    // Начинаем главное окно
    if (ImGui::Begin("Sigma Encryptor", nullptr, 
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
    {
        // Заголовок по центру - большой и жирный
        float windowWidth = ImGui::GetWindowWidth();
        
        // Увеличиваем масштаб для заголовка
        ImGui::SetWindowFontScale(1.8f);
        
        // Получаем размер текста заголовка
        ImVec2 titleSize = ImGui::CalcTextSize("Sigma Encryptor");
        
        // Центрируем заголовок правильно
        float centerX = (windowWidth - titleSize.x) * 0.5f;
        ImGui::SetCursorPosX(centerX);
        ImGui::SetCursorPosY(25);
        
        // Заголовок крупным шрифтом с красивым цветом
        ImGui::TextColored(ImVec4(0.75f, 0.85f, 0.95f, 1.0f), "Sigma Encryptor");
        
        // Возвращаем нормальный размер шрифта
        ImGui::SetWindowFontScale(1.0f);
        
        ImGui::Separator();
        ImGui::Spacing();
        
        // ========== ВЫБОР ОПЕРАЦИИ ==========
        float labelWidth = 100.0f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20); // Опустить ещё ниже
        ImGui::Text("Операция:");
        ImGui::SameLine(labelWidth);
        ImGui::RadioButton("Шифрование##encrypt", &m_selectedOperation, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Дешифрование##decrypt", &m_selectedOperation, 1);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ========== ПАНЕЛЬ ФАЙЛА ==========
        ImGui::Text("Файл:");
        ImGui::SameLine(labelWidth);
        ImGui::PushItemWidth(windowWidth - labelWidth - 120);
        ImGui::InputText("##filepath", m_filePathBuffer, sizeof(m_filePathBuffer), 
            ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Обзор"))
        {
            std::vector<std::pair<std::string, std::string>> filters = {
                {"All Files", "*.*"}
            };
            std::string selectedFile = FileDialog::openFile("Выберите файл", filters);
            if (!selectedFile.empty())
            {
                setFilePath(selectedFile);
                addLogMessage("Выбран файл: " + selectedFile);
            }
        }
        
        ImGui::Spacing();
        
        // ========== ПАНЕЛЬ ПАРОЛЯ (на одной строке) ==========
        ImGui::Text("Пароль:");
        ImGui::SameLine(labelWidth);
        ImGui::PushItemWidth(280);
        ImGui::InputText("##password", m_password, sizeof(m_password), 
            ImGuiInputTextFlags_Password);
        ImGui::PopItemWidth();
        
        // Подтверждение пароля только при шифровании
        if (m_selectedOperation == 0)
        {
            ImGui::SameLine();
            ImGui::Text("Подтвердить:");
            ImGui::SameLine();
            ImGui::PushItemWidth(200);
            ImGui::InputText("##passwordconfirm", m_passwordConfirm, sizeof(m_passwordConfirm), 
                ImGuiInputTextFlags_Password);
            ImGui::PopItemWidth();
            
            // Проверка совпадения паролей
            if (strlen(m_passwordConfirm) > 0 && strcmp(m_password, m_passwordConfirm) != 0)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Пароли не совпадают!");
            }
        }
        
        ImGui::Spacing();
        
        // ========== ПАНЕЛЬ АЛГОРИТМА (только для шифрования) ==========
        if (m_selectedOperation == 0)
        {
            ImGui::Text("Алгоритм:");
            ImGui::SameLine(labelWidth);
            
            static int currentItem = 0;
            const char* items[] = { "AES-256-GCM", "ChaCha20-Poly1305" };
            
            ImGui::PushItemWidth(180);
            if (ImGui::Combo("##cipher", &currentItem, items, IM_ARRAYSIZE(items)))
            {
                m_selectedCipher = (currentItem == 0) ? CipherType::AES_256_GCM : CipherType::ChaCha20_Poly1305;
            }
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), 
                currentItem == 0 ? "(рекомендуется)" : "(современный)");
            
            ImGui::Spacing();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ========== КНОПКА ДЕЙСТВИЯ ==========
        float buttonWidth = 200.0f;
        float buttonX = (windowWidth - buttonWidth) * 0.5f;
        ImGui::SetCursorPosX(buttonX);
        
        bool canExecute = canPerformOperation();
        
        if (!canExecute)
        {
            ImGui::BeginDisabled();
        }
        
        std::string buttonText = (m_selectedOperation == 0) ? "Зашифровать файл" : "Расшифровать файл";
        
        if (ImGui::Button(buttonText.c_str(), ImVec2(buttonWidth, 40)))
        {
            if (m_selectedOperation == 0)
                performEncryption();
            else
                performDecryption();
        }
        
        if (!canExecute)
        {
            ImGui::EndDisabled();
        }
        
        ImGui::Spacing();
        
        // ========== ЛОГ ==========
        ImGui::Text("Лог:");
        ImGui::Separator();
        
        ImGui::BeginChild("LogPanel", ImVec2(0, 130), true);
        for (const auto& msg : m_logMessages)
        {
            ImGui::TextWrapped("%s", msg.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
        {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        
        // Статус
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "%s", m_statusText.c_str());
    }
    ImGui::End();
}

// =============================================================================
// ПАНЕЛИ ИНТЕРФЕЙСА
// =============================================================================

void UI::renderFilePanel()
{
    ImGui::Text("Файл:");
    ImGui::SameLine();
    
    // Поле ввода пути к файлу (только для чтения)
    ImGui::InputText("##filepath", m_filePathBuffer, sizeof(m_filePathBuffer), 
        ImGuiInputTextFlags_ReadOnly);
    
    ImGui::SameLine();
    
    // Кнопка выбора файла - открывает нативный диалог Windows
    if (ImGui::Button("Обзор##browse"))
    {
        // Фильтры для диалога: все файлы
        std::vector<std::pair<std::string, std::string>> filters = {
            {"All Files", "*.*"}
        };
        
        // Открываем диалог выбора файла
        std::string selectedFile = FileDialog::openFile("Выберите файл", filters);
        
        // Если файл выбран (не пустая строка)
        if (!selectedFile.empty())
        {
            setFilePath(selectedFile);
            addLogMessage("Выбран файл: " + selectedFile);
        }
    }
}

void UI::renderPasswordPanel()
{
    ImGui::Text("Пароль:");
    ImGui::SameLine();
    ImGui::InputText("##password", m_password, sizeof(m_password), 
        ImGuiInputTextFlags_Password);
    
    // Подтверждение пароля только при шифровании
    if (m_selectedOperation == 0)
    {
        ImGui::Text("Подтвердите пароль:");
        ImGui::SameLine();
        ImGui::InputText("##passwordconfirm", m_passwordConfirm, sizeof(m_passwordConfirm), 
            ImGuiInputTextFlags_Password);
        
        // Проверка совпадения паролей
        if (strlen(m_passwordConfirm) > 0 && strcmp(m_password, m_passwordConfirm) != 0)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Пароли не совпадают!");
        }
    }
}

void UI::renderAlgorithmPanel()
{
    ImGui::Text("Алгоритм:");
    ImGui::SameLine();
    
    static int currentItem = 0;
    const char* items[] = { "AES-256-GCM", "ChaCha20-Poly1305" };
    
    if (ImGui::Combo("##cipher", &currentItem, items, IM_ARRAYSIZE(items)))
    {
        m_selectedCipher = (currentItem == 0) ? CipherType::AES_256_GCM : CipherType::ChaCha20_Poly1305;
    }
    
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
        currentItem == 0 ? "(рекомендуется)" : "(современный)");
}

void UI::renderActionButtons()
{
    float windowWidth = ImGui::GetWindowWidth();
    float buttonWidth = 160.0f;
    
    bool canExecute = canPerformOperation();
    
    if (!canExecute)
    {
        ImGui::BeginDisabled();
    }
    
    ImGui::SetCursorPosX((windowWidth - buttonWidth) / 2.0f);
    
    std::string buttonText = (m_selectedOperation == 0) ? "Зашифровать" : "Расшифровать";
    
    if (ImGui::Button(buttonText.c_str(), ImVec2(buttonWidth, 0)))
    {
        if (m_selectedOperation == 0)
            performEncryption();
        else
            performDecryption();
    }
    
    if (!canExecute)
    {
        ImGui::EndDisabled();
    }
}

void UI::renderLogPanel()
{
    // Область лога с фиксированной высотой
    ImGui::BeginChild("LogPanel", ImVec2(0, 120), true);
    
    ImGui::Text("Лог:");
    ImGui::Separator();
    
    // Выводим все сообщения
    for (const auto& msg : m_logMessages)
    {
        ImGui::TextWrapped("%s", msg.c_str());
    }
    
    // Прокрутка к последнему сообщению
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
    {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    // Статус
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", m_statusText.c_str());
}

// =============================================================================
// ОПЕРАЦИИ ШИФРОВАНИЯ
// =============================================================================

bool UI::canPerformOperation() const
{
    if (strlen(m_filePathBuffer) == 0)
        return false;
    
    if (strlen(m_password) == 0)
        return false;
    
    if (m_selectedOperation == 0) // Шифрование
    {
        if (strlen(m_passwordConfirm) == 0 || strcmp(m_password, m_passwordConfirm) != 0)
            return false;
    }
    
    return true;
}

void UI::performEncryption()
{
    if (!canPerformOperation())
        return;
    
    addLogMessage("Начало шифрования...");
    
    std::string inputPath = m_filePathBuffer;
    std::string outputPath = inputPath + ".sge";
    
    EncryptionResult result = m_encryptor->encryptFile(
        inputPath, outputPath, m_password, m_selectedCipher);
    
    m_lastResult = result;
    m_operationComplete = true;
    
    if (result == EncryptionResult::Success)
    {
        addLogMessage("Успех! Файл зашифрован: " + outputPath);
        m_statusText = "Шифрование завершено";
    }
    else
    {
        addLogMessage("Ошибка шифрования");
        m_statusText = "Ошибка";
    }
}

void UI::performDecryption()
{
    if (!canPerformOperation())
        return;
    
    addLogMessage("Начало дешифрования...");
    
    std::string inputPath = m_filePathBuffer;
    std::string outputPath = inputPath;
    
    // Убираем .sge если есть
    if (outputPath.size() > 4 && outputPath.substr(outputPath.size() - 4) == ".sge")
    {
        outputPath = outputPath.substr(0, outputPath.size() - 4);
    }
    else
    {
        outputPath = outputPath + ".dec";
    }
    
    EncryptionResult result = m_encryptor->decryptFile(
        inputPath, outputPath, m_password);
    
    m_lastResult = result;
    m_operationComplete = true;
    
    if (result == EncryptionResult::Success)
    {
        addLogMessage("Успех! Файл дешифрован: " + outputPath);
        m_statusText = "Дешифрование завершено";
    }
    else
    {
        addLogMessage("Ошибка дешифрования");
        m_statusText = (result == EncryptionResult::InvalidPassword) ? 
            "Неверный пароль" : "Ошибка";
    }
}

void UI::addLogMessage(const std::string& message)
{
    m_logMessages.push_back(message);
    // Ограничиваем количество сообщений
    if (m_logMessages.size() > 100)
    {
        m_logMessages.erase(m_logMessages.begin());
    }
}

} // namespace sigma
