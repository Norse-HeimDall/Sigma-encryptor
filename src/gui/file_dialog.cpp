/**
 * @file file_dialog.cpp
 * @brief Реализация нативного диалога выбора файла для Windows (UTF-16/Unicode)
 */

#include "file_dialog.hpp"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <commdlg.h>
#endif

#include <iostream>
#include <algorithm>
#include <cstring>

namespace sigma
{

// ============================================================================
// ВНУТРЕННИЕ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (UNICODE)
// ============================================================================

static std::string wstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &result[0], size_needed, nullptr, nullptr);
    return result;
}

static std::wstring utf8ToWstring(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &result[0], size_needed);
    return result;
}

std::string FileDialog::buildFilterString(const std::vector<std::pair<std::string, std::string>>& filters)
{
    std::string filterString;
    for (const auto& filter : filters)
    {
        filterString += filter.first;
        filterString += '\0';
        filterString += filter.second;
        filterString += '\0';
    }
    filterString += '\0';
    return filterString;
}

// ============================================================================
// РЕАЛИЗАЦИЯ МЕТОДОВ КЛАССА
// ============================================================================

std::string FileDialog::openFile(const std::string& title,
                                 const std::vector<std::pair<std::string, std::string>>& filters)
{
#ifdef _WIN32
    OPENFILENAMEW ofn;
    wchar_t fileName[MAX_PATH] = L"";
    
    std::memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    
    std::string filterStr = buildFilterString(filters);
    std::wstring wFilter = utf8ToWstring(filterStr);
    ofn.lpstrFilter = wFilter.c_str();
    
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    
    std::wstring wTitle = utf8ToWstring(title);
    ofn.lpstrTitle = wTitle.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn))
    {
        return wstringToUtf8(fileName);
    }

    DWORD error = CommDlgExtendedError();
    if (error != 0) {
        std::cerr << "[ОШИБКА] Код диалога открытия: " << error << std::endl;
    }
#endif
    return "";
}

std::string FileDialog::saveFile(const std::string& title,
                                 const std::string& defaultName,
                                 const std::vector<std::pair<std::string, std::string>>& filters)
{
#ifdef _WIN32
    OPENFILENAMEW ofn;
    wchar_t fileName[MAX_PATH] = L"";
    if (!defaultName.empty())
    {
        std::wstring wDefaultName = utf8ToWstring(defaultName);
        wcsncpy_s(fileName, wDefaultName.c_str(), MAX_PATH - 1);
    }

    std::memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    
    std::string filterStr = buildFilterString(filters);
    std::wstring wFilter = utf8ToWstring(filterStr);
    ofn.lpstrFilter = wFilter.c_str();
    
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    
    std::wstring wTitle = utf8ToWstring(title);
    ofn.lpstrTitle = wTitle.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    if (GetSaveFileNameW(&ofn))
    {
        return wstringToUtf8(fileName);
    }
#endif
    return "";
}

} // namespace sigma
