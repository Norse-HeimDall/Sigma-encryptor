/**
 * @file file_dialog.hpp
 * @brief Интерфейс нативного диалога выбора файла
 */

#ifndef SIGMA_FILE_DIALOG_HPP
#define SIGMA_FILE_DIALOG_HPP

#include <string>
#include <vector>

namespace sigma
{

class FileDialog
{
public:
    static std::string openFile(const std::string& title = "Open File",
                                const std::vector<std::pair<std::string, std::string>>& filters = 
                                {{"All Files", "*.*"}});

    static std::string saveFile(const std::string& title = "Save File",
                                const std::string& defaultName = "",
                                const std::vector<std::pair<std::string, std::string>>& filters =
                                {{"All Files", "*.*"}});

private:
    static std::string buildFilterString(const std::vector<std::pair<std::string, std::string>>& filters);
};

} // namespace sigma

#endif // SIGMA_FILE_DIALOG_HPP