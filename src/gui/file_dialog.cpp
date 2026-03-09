/**
 * @file file_dialog.cpp
 * @brief Реализация нативного диалога выбора файла
 */

#include "file_dialog.hpp"

// Windows API
#ifdef _WIN32
    #include <windows.h>
    #include <commdlg.h>
    #include <shlobj.h>
#endif

// Linux API
#ifdef __linux__
    #include <gtk/gtk.h>
#endif

#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>

// Убираем предупреждение о convert_wchar_to_char (только для MSVC)
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4244)
#endif

namespace sigma
{

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (Windows)
// ============================================================================

#ifdef _WIN32
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
#endif

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
// ОТКРЫТИЕ ФАЙЛА
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
    
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    
    if (GetOpenFileNameW(&ofn))
    {
        std::string result = wstringToUtf8(fileName);
        std::cout << "[INFO] Selected file: " << result << std::endl;
        return result;
    }
    
    DWORD error = CommDlgExtendedError();
    if (error != 0)
    {
        std::cerr << "[ERROR] File open dialog, error code: " << error << std::endl;
    }
    
    std::cout << "[INFO] File open dialog cancelled" << std::endl;
    return "";

#elif defined(__linux__)
    if (!gtk_init_check(0, nullptr))
    {
        std::cerr << "[ERROR] Failed to initialize GTK" << std::endl;
        return "";
    }
    
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        title.c_str(),
        nullptr,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        nullptr
    );
    
    for (const auto& filter : filters)
    {
        GtkFileFilter* gtkFilter = gtk_file_filter_new();
        gtk_file_filter_set_name(gtkFilter, filter.first.c_str());
        
        std::string pattern = filter.second;
        if (pattern == "*")
        {
            gtk_file_filter_add_pattern(gtkFilter, "*");
        }
        else
        {
            std::replace(pattern.begin(), pattern.end(), '*', ' ');
            std::string finalPattern = "*.";
            finalPattern += pattern;
            finalPattern.erase(std::remove(finalPattern.begin(), finalPattern.end(), ' '), finalPattern.end());
            gtk_file_filter_add_pattern(gtkFilter, finalPattern.c_str());
        }
        
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), gtkFilter);
    }
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    std::string result;
    if (response == GTK_RESPONSE_ACCEPT)
    {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename)
        {
            result = filename;
            std::cout << "[INFO] Selected file: " << result << std::endl;
            g_free(filename);
        }
    }
    else
    {
        std::cout << "[INFO] File open dialog cancelled" << std::endl;
    }
    
    gtk_widget_destroy(dialog);
    while (gtk_events_pending())
        gtk_main_iteration();
    
    return result;

#else
    std::cerr << "[ERROR] File dialog not supported on this platform" << std::endl;
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
    
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileNameW(&ofn))
    {
        std::string result = wstringToUtf8(fileName);
        std::cout << "[INFO] Selected save path: " << result << std::endl;
        return result;
    }
    
    DWORD error = CommDlgExtendedError();
    if (error != 0)
    {
        std::cerr << "[ERROR] File save dialog, error code: " << error << std::endl;
    }
    
    std::cout << "[INFO] File save dialog cancelled" << std::endl;
    return "";

#elif defined(__linux__)
    if (!gtk_init_check(0, nullptr))
    {
        std::cerr << "[ERROR] Failed to initialize GTK" << std::endl;
        return "";
    }
    
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        title.c_str(),
        nullptr,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        nullptr
    );
    
    if (!defaultName.empty())
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), defaultName.c_str());
    }
    
    for (const auto& filter : filters)
    {
        GtkFileFilter* gtkFilter = gtk_file_filter_new();
        gtk_file_filter_set_name(gtkFilter, filter.first.c_str());
        
        std::string pattern = filter.second;
        if (pattern == "*")
        {
            gtk_file_filter_add_pattern(gtkFilter, "*");
        }
        else
        {
            std::replace(pattern.begin(), pattern.end(), '*', ' ');
            std::string finalPattern = "*.";
            finalPattern += pattern;
            finalPattern.erase(std::remove(finalPattern.begin(), finalPattern.end(), ' '), finalPattern.end());
            gtk_file_filter_add_pattern(gtkFilter, finalPattern.c_str());
        }
        
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), gtkFilter);
    }
    
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    std::string result;
    if (response == GTK_RESPONSE_ACCEPT)
    {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename)
        {
            result = filename;
            std::cout << "[INFO] Selected save path: " << result << std::endl;
            g_free(filename);
        }
    }
    else
    {
        std::cout << "[INFO] File save dialog cancelled" << std::endl;
    }
    
    gtk_widget_destroy(dialog);
    while (gtk_events_pending())
        gtk_main_iteration();
    
    return result;

#else
    std::cerr << "[ERROR] File dialog not supported on this platform" << std::endl;
    return "";
#endif
}

} // namespace sigma

#ifdef _WIN32
#pragma warning(pop)
#endif

