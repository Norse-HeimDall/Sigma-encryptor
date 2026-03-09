# Sigma Encryptor - Инструкция по сборке

## Требования

1. **Visual Studio 2019 или новее** (с компонентами C++)
2. **CMake 3.15+**
3. **vckg** - установленный и настроенный

## Предварительная настройка

### 1. Установка зависимостей через vcpkg

```powershell
# Установите vcpkg если ещё не установлен
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# Установите необходимые библиотеки
vcpkg install glfw3:x64-windows
vcpkg install imgui[x64-windows]:x64-windows
vcpkg install openssl:x64-windows
```

### 2. Настройка переменных окружения

Добавьте путь к vcpkg в переменную `VCPKG_ROOT` или используйте путь по умолчанию:
- `C:\Users\ВашеИмя\vcpkg`

## Сборка через CMake (рекомендуется)

### Вариант 1: CMake GUI

1. Откройте CMake GUI
2. Укажите путь к исходникам: `c:\dev\repos\C++\Sigma encyptor 4.0`
3. Укажите путь для сборки: `c:\dev\repos\C++\Sigma encyptor 4.0\build`
4. Нажмите "Configure"
5. Выберите генератор: **Visual Studio 16 2019** (или 17 2022) и платформу **x64**
6. Укажите путь к toolchain файлу:
   ```
   C:\Users\ВашеИмя\vcpkg\scripts\buildsystems\vcpkg.cmake
   ```
7. Нажмите "Configure" снова
8. Нажмите "Generate"

### Вариант 2: Командная строка

```powershell
cd "c:\dev\repos\C++\Sigma encyptor 4.0"

# Создайте папку для сборки
mkdir build
cd build

# Настройка CMake
cmake .. -G "Visual Studio 16 2019" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE=C:\Users\HeimDall\vcpkg\scripts\buildsystems\vcpkg.cmake `
    -DCMAKE_BUILD_TYPE=Release

# Сборка
cmake --build . --config Release
```

## Запуск

После успешной сборки исполняемый файл находится здесь:
```
build\bin\Release\SigmaEncryptor.exe
```

## Структура проекта

```
Sigma Encryptor/
├── src/
│   ├── main.cpp              # Точка входа
│   ├── application.cpp/hpp   # Главный класс приложения
│   ├── gui/
│   │   ├── renderer.cpp/hpp  # OpenGL рендерер
│   │   ├── ui.cpp/hpp        # Элементы UI
│   │   ├── theme.cpp/hpp     # Тема Obsidian
│   │   └── file_dialog.cpp/hpp # Диалог выбора файла
│   └── crypto/
│       ├── encryptor.cpp/hpp # Главный класс шифрования
│       ├── aes.cpp/hpp       # AES-256-GCM
│       └── chacha20.cpp/hpp  # ChaCha20-Poly1305
├── CMakeLists.txt            # Конфигурация сборки
└── BUILD.md                  # Этот файл
```

## Использование

1. Запустите `SigmaEncryptor.exe`
2. Выберите операцию: шифрование или дешифрование
3. Нажмите "Обзор" и выберите файл
4. Введите пароль и подтвердите его
5. Нажмите "Зашифровать" или "Расшифровать"

## Формат зашифрованного файла

Зашифрованные файлы имеют расширение `.sge` и содержат:
- 4 байта: Magic bytes "SGE1"
- 1 байт: Версия формата (0x01)
- 1 байт: Тип шифрования
- 16 байт: Соль для PBKDF2
- 12 байт: IV/Nonce
- N байт: Зашифрованные данные + тег аутентификации

## Устранение проблем

### Ошибка: "Не найдены библиотеки"

Убедитесь что vcpkg установлен и библиотеки собраны:
```powershell
vcpkg list
```

### Ошибка: "Cannot find glfw3"

Проверьте что путь к toolchain файлу верный.

### Ошибка компиляции

Убедитесь что установлена Visual Studio с компонентами C++.

