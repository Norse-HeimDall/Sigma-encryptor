/**
 * @file file_dialog.cpp
 * @brief Реализация нативного диалога выбора файла для Windows
 */

#include "file_dialog.hpp"

// Windows API
#ifdef _WIN32
    #include <windows.h>
    #include <commdlg.h>
    #include <shlobj.h>
#else
    // Для других платформ - пустой блок
#endif

#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>

// Убираем предупреждение о convert_wchar_to_char
#pragma warning(push)
#pragma warning(disable: 4244)

namespace sigma
{

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * @brief Конвертирует wide string (UTF-16) в UTF-8 строку
 */
static std::string wstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), 
                                          nullptr, 0, nullptr, nullptr);
    if (size_needed == 0)
        return std::string();
    
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), 
                        &result[0], size_needed, nullptr, nullptr);
    return result;
}

/**
 * @brief Конвертирует UTF-8 строку в wide string (UTF-16)
 */
static std::wstring utf8ToWstring(const std::string& str)
{
    if (str.empty())
        return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), 
                                          nullptr, 0);
    if (size_needed == 0)
        return std::wstring();
    
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), 
                        &result[0], size_needed);
    return result;
}

std::string FileDialog::buildFilterString(const std::vector<std::pair<std::string, std::string>>& filters)
{
    std::string filterString;
    
    for (const auto& filter : filters)
    {
        // Добавляем описание фильтра
        filterString += filter.first;
        filterString += '\0';
        
        // Добавляем маску файла
        filterString += filter.second;
        filterString += '\0';
    }
    
    // Завершаем строку двумя нулями
    filterString += '\0';
    
    return filterString;
}

// ============================================================================
// ОТКРЫТИЕ ФАЙЛА
// ============================================================================

std::string FileDialog::openFile(const std::string& title,
                                 const std::vector<std::pair<std::string, std::string>>& filters)
{
#ifdef _WIN32
    // Структура открытия файла
    OPENFILENAMEW ofn;
    wchar_t fileName[MAX_PATH] = L"";
    
    // Очищаем память
    std::memset(&ofn, 0, sizeof(ofn));
    
    // Устанавливаем размер структуры
    ofn.lStructSize = sizeof(ofn);
    
    // Дескриктор окна (можно сделать NULL для простого диалога)
    ofn.hwndOwner = NULL;
    
    // Устанавливаем фильтры
    std::string filterStr = buildFilterString(filters);
    std::wstring wFilter = utf8ToWstring(filterStr);
    ofn.lpstrFilter = wFilter.c_str();
    
    // Индекс фильтра по умолчанию (1 = первый фильтр)
    ofn.nFilterIndex = 1;
    
    // Буфер для имени файла
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    
    // Заголовок диалога
    std::wstring wTitle = utf8ToWstring(title);
    ofn.lpstrTitle = wTitle.c_str();
    
    // Флаги
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    
    // Показываем диалог
    if (GetOpenFileNameW(&ofn))
    {
        // Конвертируем wide string в UTF-8 строку
        std::string result = wstringToUtf8(fileName);
        
        std::cout << "[ИНФО] Выбран файл: " << result << std::endl;
        return result;
    }
    
    // Получаем код ошибки
    DWORD error = CommDlgExtendedError();
    if (error != 0)
    {
        std::cerr << "[ОШИБКА] Диалог открытия файла, код ошибки: " << error << std::endl;
    }
    
    std::cout << "[ИНФО] Диалог открытия файла отменён" << std::endl;
    return "";
    
#else
    // Для других платформ возвращаем пустую строку
    std::cerr << "[ОШИБКА] Диалог открытия файла поддерживается только на Windows" << std::endl;
    return "";
#endif
}

// ============================================================================
// СОХРАНЕНИЕ ФАЙЛА
// ============================================================================

std::string FileDialog::saveFile(const std::string& title,
                                  const std::string& defaultName,
                                  const std::vector<std::pair<std::string, std::string>>& filters)
{
#ifdef _WIN32
    // Структура сохранения файла
    OPENFILENAMEW ofn;
    wchar_t fileName[MAX_PATH] = L"";
    
    // Копируем имя файла по умолчанию
    if (!defaultName.empty())
    {
        std::wstring wDefaultName = utf8ToWstring(defaultName);
        wcsncpy_s(fileName, wDefaultName.c_str(), MAX_PATH - 1);
    }
    
    // Очищаем память
    std::memset(&ofn, 0, sizeof(ofn));
    
    // Устанавливаем размер структуры
    ofn.lStructSize = sizeof(ofn);
    
    // Дескриктор окна
    ofn.hwndOwner = NULL;
    
    // Устанавливаем фильтры
    std::string filterStr = buildFilterString(filters);
    std::wstring wFilter = utf8ToWstring(filterStr);
    ofn.lpstrFilter = wFilter.c_str();
    
    // Индекс фильтра по умолчанию
    ofn.nFilterIndex = 1;
    
    // Буфер для имени файла
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    
    // Заголовок диалога
    std::wstring wTitle = utf8ToWstring(title);
    ofn.lpstrTitle = wTitle.c_str();
    
    // Флаги (не требуем существование файла для сохранения)
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    
    // Показываем диалог
    if (GetSaveFileNameW(&ofn))
    {
        // Конвертируем wide string в UTF-8 строку (правильная конвертация!)
        std::string result = wstringToUtf8(fileName);
        
        std::cout << "[ИНФО] Выбран путь для сохранения: " << result << std::endl;
        return result;
    }
    
    // Получаем код ошибки
    DWORD error = CommDlgExtendedError();
    if (error != 0)
    {
        std::cerr << "[ОШИБКА] Диалог сохранения файла, код ошибки: " << error << std::endl;
    }
    
    std::cout << "[ИНФО] Диалог сохранения файла отменён" << std::endl;
    return "";
    
#else
    // Для других платформ возвращаем пустую строку
    std::cerr << "[ОШИБКА] Диалог сохранения файла поддерживается только на Windows" << std::endl;
    return "";
#endif
}

} // namespace sigma

#pragma warning(pop)

