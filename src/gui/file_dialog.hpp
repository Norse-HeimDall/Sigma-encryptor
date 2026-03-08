/**
 * @file file_dialog.hpp
 * @brief Нативный диалог выбора файла для Windows
 */

#ifndef SIGMA_FILE_DIALOG_HPP
#define SIGMA_FILE_DIALOG_HPP

#include <string>
#include <vector>

namespace sigma
{

/**
 * @class FileDialog
 * @brief Класс для работы с нативными файловыми диалогами Windows
 */
class FileDialog
{
public:
    /**
     * @brief Открывает диалог выбора файла для открытия
     * @param title Заголовок диалога
     * @param filter Фильтры файлов (например, {"All Files", "*"})
     * @return Путь к выбранному файлу или пустая строка при отмене
     */
    static std::string openFile(const std::string& title = "Open File",
                                const std::vector<std::pair<std::string, std::string>>& filters = 
                                {{"All Files", "*"}});

    /**
     * @brief Открывает диалог сохранения файла
     * @param title Заголовок диалога
     * @param defaultName Имя файла по умолчанию
     * @param filter Фильтры файлов
     * @return Путь для сохранения или пустая строка при отмене
     */
    static std::string saveFile(const std::string& title = "Save File",
                                const std::string& defaultName = "",
                                const std::vector<std::pair<std::string, std::string>>& filters =
                                {{"All Files", "*"}});

private:
    /**
     * @brief Создаёт строку фильтра для Windows диалога
     */
    static std::string buildFilterString(const std::vector<std::pair<std::string, std::string>>& filters);
};

} // namespace sigma

#endif // SIGMA_FILE_DIALOG_HPP

