/**
 * @file ui.cpp
 * @brief Реализация пользовательского интерфейса Sigma Encryptor
 * 
 * @details
 * Этот файл содержит полную реализацию класса UI.
 * UI рисует ImGui интерфейс: радио-кнопки операций, поле файла,
 * поля пароля, выбор алгоритма, главную кнопку, лог операций.
 * 
 * Ключевые особенности:
 * - Нативный диалог Windows для выбора файлов (UTF-8)
 * - Валидация пароля (подтверждение + совпадение)
 * - Лог с автопрокруткой (лимит 100 строк)
 * - Безопасная очистка пароля из памяти после операций
 * - Статус операции внизу
 * 
 * @author heimdall
 * @version 4.0
 */

#include "ui.hpp"
#include "theme.hpp"
#include "file_dialog.hpp"

#include <iostream>
#include <cstring>  // Для strlen, strcmp, memset
#include <chrono>   // Замер времени операций

// ImGui для GUI
#include "imgui.h"

namespace sigma
{

// =============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// =============================================================================

/**
 * @brief Конструктор UI
 * Инициализирует Encryptor и поля состояния.
 */
UI::UI()
    : m_encryptor(std::make_unique<Encryptor>())
    , m_operationComplete(false)
    , m_lastResult(EncryptionResult::Success)
{
    std::cout << "[ИНФО] UI создан" << std::endl;
}

/**
 * @brief Деструктор UI
 */
UI::~UI()
{
    std::cout << "[ИНФО] UI уничтожен" << std::endl;
}

// =============================================================================
// ИНИЦИАЛИЗАЦИЯ И СБРОС
// =============================================================================

/**
 * @brief Инициализация UI
 * Применяет тёмную тему Obsidian для профессионального вида.
 */
void UI::initialize()
{
    Theme::applyObsidianTheme();
    std::cout << "[ИНФО] Тема Obsidian применена к UI" << std::endl;
}

/**
 * @brief Сброс формы
 * Очищает все поля для новой операции.
 */
void UI::reset()
{
    m_filePath.clear();
    std::memset(m_password, 0, sizeof(m_password));             // Безопасная очистка
    std::memset(m_passwordConfirm, 0, sizeof(m_passwordConfirm)); // ^
    std::memset(m_filePathBuffer, 0, sizeof(m_filePathBuffer));
    m_selectedOperation = 0;
    m_operationComplete = false;
    m_lastResult = EncryptionResult::Success;
    m_statusText = "Выберите файл для работы";
    m_logMessages.clear();
}

/**
 * @brief Установка пути к файлу из диалога
 * Копирует в буфер для ImGui InputText.
 */
void UI::setFilePath(const std::string& path)
{
    m_filePath = path;
    std::strncpy(m_filePathBuffer, path.c_str(), sizeof(m_filePathBuffer) - 1);
    m_filePathBuffer[sizeof(m_filePathBuffer) - 1] = '\0';  // Null-terminate
}

// =============================================================================
// ОСНОВНОЙ РЕНДЕРИНГ (вызывается каждый кадр)
// =============================================================================

/**
 * @brief Отрисовка полного интерфейса
 * Создаёт фиксированное окно 900x600 с центрированным заголовком.
 */
void UI::render()
{
    // Фиксируем размер/позицию окна на весь экран (no resize/move)
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    
    if (ImGui::Begin("Sigma Encryptor", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
    {
        // ========================================
        // ЦЕНТРИРОВАННЫЙ ЗАГОЛОВОК
        // ========================================
        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SetWindowFontScale(1.8f);  // Увеличиваем шрифт для заголовка
        
        ImVec2 titleSize = ImGui::CalcTextSize("Sigma Encryptor");
        float centerX = (windowWidth - titleSize.x) * 0.5f;
        ImGui::SetCursorPosX(centerX);
        ImGui::SetCursorPosY(25);
        
        // Крупный заголовок синим цветом
        ImGui::TextColored(ImVec4(0.75f, 0.85f, 0.95f, 1.0f), "Sigma Encryptor");
        ImGui::SetWindowFontScale(1.0f);  // Возврат нормального размера
        
        ImGui::Separator();
        ImGui::Spacing();

        // ========================================
        // ВЫБОР ОПЕРАЦИИ (радио-кнопки)
        // ========================================
        float labelWidth = 100.0f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
        ImGui::Text("Операция:");
        ImGui::SameLine(labelWidth);
        ImGui::RadioButton("Шифрование##encrypt", &m_selectedOperation, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Дешифрование##decrypt", &m_selectedOperation, 1);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ========================================
        // ПОЛЕ ФАЙЛА + КНОПКА ОБЗОР
        // ========================================
        ImGui::Text("Файл:");
        ImGui::SameLine(labelWidth);
        ImGui::PushItemWidth(windowWidth - labelWidth - 120);
        ImGui::InputText("##filepath", m_filePathBuffer, sizeof(m_filePathBuffer), 
                         ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Обзор"))
        {
            // Нативный диалог Windows с фильтром "Все файлы"
            std::vector<std::pair<std::string, std::string>> filters = {{"All Files", "*.*"}};
            std::string selectedFile = FileDialog::openFile("Выберите файл", filters);
            if (!selectedFile.empty())
            {
                setFilePath(selectedFile);
                addLogMessage("Выбран файл: " + selectedFile);
            }
        }
        
        ImGui::Spacing();

        // ========================================
        // ПОЛЯ ПАРОЛЯ (с подтверждением для шифрования)
        // ========================================
        ImGui::Text("Пароль:");
        ImGui::SameLine(labelWidth);
        ImGui::PushItemWidth(280);
        ImGui::InputText("##password", m_password, sizeof(m_password), 
                         ImGuiInputTextFlags_Password);
        ImGui::PopItemWidth();
        
        // Подтверждение только для шифрования
        if (m_selectedOperation == 0)
        {
            ImGui::SameLine();
            ImGui::Text("Подтвердить:");
            ImGui::SameLine();
            ImGui::PushItemWidth(200);
            ImGui::InputText("##passwordconfirm", m_passwordConfirm, sizeof(m_passwordConfirm), 
                             ImGuiInputTextFlags_Password);
            ImGui::PopItemWidth();
            
            // Красная подсветка если не совпадают
            if (strlen(m_passwordConfirm) > 0 && strcmp(m_password, m_passwordConfirm) != 0)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Пароли не совпадают!");
            }
        }
        
        ImGui::Spacing();

        // ========================================
        // ВЫБОР АЛГОРИТМА (только шифрование)
        // ========================================
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

        // ========================================
        // ГЛАВНАЯ КНОПКА ДЕЙСТВИЯ (центрирована)
        // ========================================
        float buttonWidth = 200.0f;
        float buttonX = (windowWidth - buttonWidth) * 0.5f;
        ImGui::SetCursorPosX(buttonX);
        
        bool canExecute = canPerformOperation();
        if (!canExecute)
        {
            ImGui::BeginDisabled();  // Серый цвет если нельзя
        }
        
        std::string buttonText = (m_selectedOperation == 0) ? "🔒 Зашифровать файл" : "🔓 Расшифровать файл";
        if (ImGui::Button(buttonText.c_str(), ImVec2(buttonWidth, 45)))
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

        // ========================================
        // ЛОГ ОПЕРАЦИЙ (Child окно с прокруткой)
        // ========================================
        ImGui::Text("Лог операций:");
        ImGui::Separator();
        
        ImGui::BeginChild("LogPanel", ImVec2(0, 140), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& msg : m_logMessages)
        {
            ImGui::TextWrapped("%s", msg.c_str());
        }
        // Автопрокрутка к последнему сообщению
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        
        // ========================================
        // СТАТУС (цветной текст снизу)
        // ========================================
        ImVec4 statusColor(0.6f, 0.8f, 1.0f, 1.0f);
        if (m_lastResult != EncryptionResult::Success)
        {
            statusColor = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);  // Красный для ошибок
        }
        ImGui::TextColored(statusColor, "%s", m_statusText.c_str());
    }
    ImGui::End();
}

// =============================================================================
// ЛОГИКА ОПЕРАЦИЙ
// =============================================================================

/**
 * @brief Проверка готовности операции
 * Файл + пароль + (для шифр: подтверждение совпадает).
 */
bool UI::canPerformOperation() const
{
    if (m_filePath.empty() || strlen(m_password) == 0)
        return false;
    
    if (m_selectedOperation == 0)  // Шифрование
    {
        return strlen(m_passwordConfirm) > 0 && strcmp(m_password, m_passwordConfirm) == 0;
    }
    return true;
}

/**
 * @brief Шифрование файла
 * 1. Замер времени. 2. Формирует .sge. 3. encryptFile(). 4. Лог + очистка пароля.
 */
void UI::performEncryption()
{
    if (!canPerformOperation()) return;
    
    addLogMessage("═══════════════════════════════════════════");
    addLogMessage("🚀 Начало шифрования...");
    
    std::string inputPath = m_filePath;
    std::string outputPath = inputPath + ".sge";
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    EncryptionResult result = m_encryptor->encryptFile(inputPath, outputPath, m_password, m_selectedCipher);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double durationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    // БЕЗОПАСНАЯ ОЧИСТКА ПАРОЛЯ ИЗ ПАМЯТИ
    std::memset(m_password, 0, sizeof(m_password));
    std::memset(m_passwordConfirm, 0, sizeof(m_passwordConfirm));
    
    m_lastResult = result;
    m_operationComplete = true;
    
    if (result == EncryptionResult::Success)
    {
        addLogMessage("✅ Файл успешно зашифрован!");
        addLogMessage("   📁 Выходной файл: " + outputPath);
        addLogMessage("   ⏱️  Время: " + std::to_string(static_cast<int>(durationMs)) + " мс");
        m_statusText = "Шифрование завершено успешно";
    }
    else
    {
        addLogMessage("❌ Ошибка шифрования (код: " + std::to_string(static_cast<int>(result)) + ")");
        m_statusText = "Ошибка шифрования";
    }
    
    addLogMessage("═══════════════════════════════════════════");
}

/**
 * @brief Дешифрование файла
 * 1. Замер времени. 2. Формирует output (убирает .sge). 3. decryptFile(). 4. Лог.
 */
void UI::performDecryption()
{
    if (!canPerformOperation()) return;
    
    addLogMessage("═══════════════════════════════════════════");
    addLogMessage("🔓 Начало дешифрования...");
    
    std::string inputPath = m_filePath;
    std::string outputPath = inputPath;
    
    // Убираем .sge если есть, иначе добавляем .dec
    if (outputPath.size() > 4 && outputPath.substr(outputPath.size() - 4) == ".sge")
    {
        outputPath = outputPath.substr(0, outputPath.size() - 4);
    }
    else
    {
        outputPath += ".dec";
    }
    
    EncryptionResult result = m_encryptor->decryptFile(inputPath, outputPath, m_password);
    
    // ОЧИСТКА ПАРОЛЯ
    std::memset(m_password, 0, sizeof(m_password));
    
    m_lastResult = result;
    m_operationComplete = true;
    
    if (result == EncryptionResult::Success)
    {
        addLogMessage("✅ Файл дешифрован: " + outputPath);
        m_statusText = "Дешифрование завершено";
    }
    else
    {
        std::string errorMsg = (result == EncryptionResult::InvalidPassword) ? 
            "Неверный пароль" : "Ошибка дешифрования";
        addLogMessage("❌ " + errorMsg);
        m_statusText = errorMsg;
    }
    
    addLogMessage("═══════════════════════════════════════════");
}

/**
 * @brief Добавление в лог
 * Автоматически ограничивает 100 строк, прокручивает вниз.
 */
void UI::addLogMessage(const std::string& message)
{
    m_logMessages.push_back("[" + std::string(__TIME__) + "] " + message);
    if (m_logMessages.size() > 100)
    {
        m_logMessages.erase(m_logMessages.begin());  // Удаляем старое
    }
}

} // namespace sigma

