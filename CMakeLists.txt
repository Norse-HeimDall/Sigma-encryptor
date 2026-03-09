# CMakeLists.txt - файл конфигурации сборки для Sigma Encryptor
# Этот файл говорит CMake как компилировать проект

# Минимальная версия CMake (3.15 - современная версия с поддержкой многих фичей)
cmake_minimum_required(VERSION 3.15)

# Имя проекта и язык программирования
project(SigmaEncryptor VERSION 1.0 LANGUAGES CXX)

# Стандарт C++ (C++17 - современный стандарт с многими возможностями)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем политику CMP0091 для нового способа управления MSVC Runtime
# Это важно для корректной работы с vcpkg
cmake_policy(SET CMP0091 NEW)

# Устанавливаем MSVC Runtime на статический (/MT) или динамическую (/MD) линковку
# Для vcpkg лучше использовать динамическую линковку
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# =============================================================================
# ПОИСК БИБЛИОТЕК ЧЕРЕЗ VCPKG
# =============================================================================

# Ищем пакет GLFW3 - библиотека для создания окон и обработки ввода
find_package(glfw3 CONFIG)
if(glfw3_FOUND)
    message(STATUS "Найден glfw3: ${glfw3_FOUND}")
endif()

# Ищем пакет ImGui - библиотека для графического интерфейса
find_package(imgui CONFIG)
if(imgui_FOUND)
    message(STATUS "Найден imgui: ${imgui_FOUND}")
endif()

# Ищем OpenSSL - библиотека для шифрования
find_package(OpenSSL CONFIG)
if(OPENSSL_FOUND)
    message(STATUS "Найден OpenSSL: ${OPENSSL_FOUND}")
endif()

# Ищем OpenGL - для рендеринга графики
find_package(OpenGL REQUIRED)

# =============================================================================
# НАСТРОЙКИ КОМПИЛЯТОРА
# =============================================================================

# Включаем все предупреждения компилятора
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Для MSVC включаем предупреждения как ошибки (/W4)
if(MSVC)
    add_compile_options(/W4)
endif()

# =============================================================================
# ИСХОДНЫЕ ФАЙЛЫ
# =============================================================================

set(SOURCES
    src/main.cpp
    src/application.cpp
    src/gui/renderer.cpp
    src/gui/ui.cpp
    src/gui/theme.cpp
    src/gui/file_dialog.cpp
    src/crypto/encryptor.cpp
    src/crypto/aes.cpp
    src/crypto/chacha20.cpp
)

# =============================================================================
# СОЗДАНИЕ ИСПОЛНЯЕМОГО ФАЙЛА
# =============================================================================

add_executable(SigmaEncryptor ${SOURCES})

set_target_properties(SigmaEncryptor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib
)

# =============================================================================
# ПОДКЛЮЧЕНИЕ БИБЛИОТЕК
# =============================================================================

# Подключаем GLFW
if(TARGET glfw::glfw)
    target_link_libraries(SigmaEncryptor PRIVATE glfw::glfw)
    message(STATUS "Подключено: glfw::glfw")
elseif(TARGET glfw)
    target_link_libraries(SigmaEncryptor PRIVATE glfw)
    message(STATUS "Подключено: glfw")
else()
    target_link_libraries(SigmaEncryptor PRIVATE ${GLFW3_LIBRARIES})
    message(STATUS "Подключено: GLFW3_LIBRARIES")
endif()

# Подключаем OpenGL
target_link_libraries(SigmaEncryptor PRIVATE OpenGL::GL)

# Подключаем ImGui
if(TARGET imgui::imgui)
    target_link_libraries(SigmaEncryptor PUBLIC imgui::imgui)
    message(STATUS "Подключено: imgui::imgui")
elseif(TARGET imgui)
    target_link_libraries(SigmaEncryptor PUBLIC imgui)
    message(STATUS "Подключено: imgui")
else()
    target_link_libraries(SigmaEncryptor PUBLIC ${IMGUI_LIBRARIES})
    message(STATUS "Подключено: IMGUI_LIBRARIES")
endif()

# Подключаем OpenSSL
if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    target_link_libraries(SigmaEncryptor PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "Подключено: OpenSSL::SSL + OpenSSL::Crypto")
elseif(TARGET OpenSSL::SSL)
    target_link_libraries(SigmaEncryptor PRIVATE OpenSSL::SSL)
    message(STATUS "Подключено: OpenSSL::SSL")
else()
    target_link_libraries(SigmaEncryptor PRIVATE ${OPENSSL_LIBRARIES})
    message(STATUS "Подключено: OPENSSL_LIBRARIES")
endif()

# =============================================================================
# ВКЛЮЧАЕМЫЕ ДИРЕКТОРИИ
# =============================================================================

set(VCPKG_ROOT "C:/Users/HeimDall/vcpkg")

target_include_directories(SigmaEncryptor PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${VCPKG_ROOT}/installed/x64-windows/include
)

# =============================================================================
# КОПИРОВАНИЕ DLL ФАЙЛОВ
# =============================================================================

find_file(GLFW_DLL glfw3.dll PATHS 
    "C:/Users/HeimDall/vcpkg/installed/x64-windows/bin" 
    NO_DEFAULT_PATH
)

if(GLFW_DLL)
    add_custom_command(TARGET SigmaEncryptor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GLFW_DLL}
            "$<TARGET_FILE_DIR:SigmaEncryptor>"
        COMMENT "Копирование glfw3.dll"
    )
endif()

find_file(LIBCRYPTO_DLL libcrypto-3-x64.dll PATHS 
    "C:/Users/HeimDall/vcpkg/installed/x64-windows/bin" 
    NO_DEFAULT_PATH
)

find_file(LIBSSL_DLL libssl-3-x64.dll PATHS 
    "C:/Users/HeimDall/vcpkg/installed/x64-windows/bin" 
    NO_DEFAULT_PATH
)

if(LIBCRYPTO_DLL)
    add_custom_command(TARGET SigmaEncryptor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LIBCRYPTO_DLL}
            "$<TARGET_FILE_DIR:SigmaEncryptor>"
        COMMENT "Копирование libcrypto-3-x64.dll"
    )
endif()

if(LIBSSL_DLL)
    add_custom_command(TARGET SigmaEncryptor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LIBSSL_DLL}
            "$<TARGET_FILE_DIR:SigmaEncryptor>"
        COMMENT "Копирование libssl-3-x64.dll"
    )
endif()

# =============================================================================
# СООБЩЕНИЕ ПРИ УСПЕШНОЙ КОНФИГУРАЦИИ
# =============================================================================

message(STATUS "========================================")
message(STATUS "Sigma Encryptor - Конфигурация завершена")
message(STATUS "Компилятор: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Стандарт C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
