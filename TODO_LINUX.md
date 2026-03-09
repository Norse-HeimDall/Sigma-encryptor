# TODO - Адаптация Sigma Encryptor под Linux

## ПЛАН РАЗРАБОТКИ

### Этап 1: CMakeLists.txt - Адаптация сборки ✓
- [x] 1.1 Удалить MSVC-специфичные флаги (CMP0091, CMAKE_MSVC_RUNTIME_LIBRARY)
- [x] 1.2 Добавить pkg-config для поиска библиотек (GLFW, OpenSSL)
- [x] 1.3 Удалить копирование DLL файлов (только для Windows)
- [x] 1.4 Обновить include директории для Linux
- [x] 1.5 Добавить FetchContent для ImGui

### Этап 2: main.cpp - Адаптация кодировки консоли ✓
- [x] 2.1 Удалить включение windows.h
- [x] 2.2 Заменить SetConsoleCP на настройку UTF-8 для Linux
- [x] 2.3 Использовать std::setlocale для Linux

### Этап 3: file_dialog.cpp - Нативные диалоги Linux ✓
- [x] 3.1 Реализовать GTK dialogs для Linux
- [x] 3.2 Добавить проверку платформы #ifdef __linux__
- [x] 3.3 Сохранить Windows реализацию

### Этап 4: renderer.cpp - Шрифты Linux ✓
- [x] 4.1 Добавить поиск системных шрифтов Linux
- [x] 4.2 Проверить пути: /usr/share/fonts, ~/.fonts
- [x] 4.3 Добавить fallback на FreeType (DejaVu, Liberation)

### Этап 5: Тестирование сборки
- [x] 5.1 Создать скрипт установки зависимостей (scripts/install_deps_linux.sh)
- [ ] 5.2 Проверить сборку через CMake
- [ ] 5.3 Исправить ошибки компиляции

