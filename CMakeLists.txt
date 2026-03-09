# CMakeLists.txt - файл конфигурации сборки для Sigma Encryptor
# Поддерживает Windows (vcpkg) и Linux (pkg-config)

# Минимальная версия CMake
cmake_minimum_required(VERSION 3.15)

# Имя проекта и язык программирования
project(SigmaEncryptor VERSION 1.0 LANGUAGES CXX)

# Стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Определяем операционную систему
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(OS_WINDOWS TRUE)
    message(STATUS "Определена ОС: Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(OS_LINUX TRUE)
    message(STATUS "Определена ОС: Linux")
endif()

# ============================================================================
# НАСТРОЙКИ ДЛЯ WINDOWS (MSVC)
# ============================================================================
if(OS_WINDOWS)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# =============================================================================
# ПОИСК БИБЛИОТЕК
# =============================================================================

# OpenGL - обязателен для всех платформ
find_package(OpenGL REQUIRED)
message(STATUS "Найден OpenGL")

# ============================================================================
# WINDOWS: Поиск через vcpkg
# ============================================================================
if(OS_WINDOWS)
    find_package(glfw3 CONFIG)
    find_package(imgui CONFIG)
    find_package(OpenSSL CONFIG)
endif()

# ============================================================================
# LINUX: Поиск через pkg-config
# ============================================================================
if(OS_LINUX)
    include(FindPkgConfig)
    
    # GLFW3
    pkg_check_modules(GLFW3 REQUIRED glfw3)
    message(STATUS "Найден GLFW3: ${GLFW3_VERSION}")
    
    # OpenSSL
    pkg_check_modules(OPENSSL REQUIRED openssl)
    message(STATUS "Найден OpenSSL: ${OPENSSL_VERSION}")
    
    # GTK3 для диалогов (опционально)
    pkg_check_modules(GTK3 gtk+-3.0)
    if(GTK3_FOUND)
        message(STATUS "Найден GTK3: ${GTK3_VERSION}")
        set(HAVE_GTK3 TRUE)
    endif()
    
    # X11 для дополнительных функций
    pkg_check_modules(X11 x11)
    
    # ============================================================================
    # ImGui через FetchContent (только для Linux)
    # ============================================================================
    include(FetchContent)
    
    # ImGui
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.6
        GIT_SHALLOW TRUE
    )
    
    # Загружаем ImGui
    FetchContent_MakeAvailable(imgui)
    
    # Добавляем путь к imgui backends
    if(imgui_SOURCE_DIR)
        target_include_directories(SigmaEncryptor PRIVATE 
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
    endif()
endif()

# =============================================================================
# НАСТРОЙКИ КОМПИЛЯТОРА
# =============================================================================

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

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

# OpenGL - всегда подключаем
target_link_libraries(SigmaEncryptor PRIVATE OpenGL::GL)

# ============================================================================
# WINDOWS: Подключение через vcpkg
# ============================================================================
if(OS_WINDOWS)
    # GLFW
    if(TARGET glfw::glfw)
        target_link_libraries(SigmaEncryptor PRIVATE glfw::glfw)
    elseif(TARGET glfw)
        target_link_libraries(SigmaEncryptor PRIVATE glfw)
    endif()
    
    # ImGui
    if(TARGET imgui::imgui)
        target_link_libraries(SigmaEncryptor PUBLIC imgui::imgui)
    elseif(TARGET imgui)
        target_link_libraries(SigmaEncryptor PUBLIC imgui)
    endif()
    
    # OpenSSL
    if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
        target_link_libraries(SigmaEncryptor PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
endif()

# ============================================================================
# LINUX: Подключение через pkg-config
# ============================================================================
if(OS_LINUX)
    # GLFW3
    target_include_directories(SigmaEncryptor PRIVATE ${GLFW3_INCLUDE_DIRS})
    target_link_libraries(SigmaEncryptor PRIVATE ${GLFW3_LIBRARIES})
    target_compile_options(SigmaEncryptor PRIVATE ${GLFW3_CFLAGS_OTHER})
    
    # OpenSSL
    target_include_directories(SigmaEncryptor PRIVATE ${OPENSSL_INCLUDE_DIRS})
    target_link_libraries(SigmaEncryptor PRIVATE ${OPENSSL_LIBRARIES})
    target_compile_options(SigmaEncryptor PRIVATE ${OPENSSL_CFLAGS_OTHER})
    
    # ImGui (from FetchContent)
    if(TARGET imgui)
        target_link_libraries(SigmaEncryptor PUBLIC imgui)
    endif()
    
    # GTK3 (если доступен)
    if(HAVE_GTK3)
        target_include_directories(SigmaEncryptor PRIVATE ${GTK3_INCLUDE_DIRS})
        target_link_libraries(SigmaEncryptor PRIVATE ${GTK3_LIBRARIES})
        target_compile_options(SigmaEncryptor PRIVATE ${GTK3_CFLAGS_OTHER})
        target_compile_definitions(SigmaEncryptor PRIVATE HAVE_GTK3=1)
    endif()
    
    # X11
    if(X11_FOUND)
        target_include_directories(SigmaEncryptor PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(SigmaEncryptor PRIVATE ${X11_LIBRARIES})
    endif()
endif()

# =============================================================================
# ВКЛЮЧАЕМЫЕ ДИРЕКТОРИИ
# =============================================================================

target_include_directories(SigmaEncryptor PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Windows: добавляем vcpkg include
if(OS_WINDOWS)
    set(VCPKG_ROOT "C:/Users/HeimDall/vcpkg")
    target_include_directories(SigmaEncryptor PRIVATE 
        ${VCPKG_ROOT}/installed/x64-windows/include
    )
endif()

# Linux: добавляем системные include для ImGui
if(OS_LINUX)
    # ImGui будет искаться в системе или должен быть в third_party/
    # Проверяем локальную папку third_party/imgui
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/imgui")
        target_include_directories(SigmaEncryptor PRIVATE 
            ${CMAKE_SOURCE_DIR}/third_party/imgui
        )
    endif()
endif()

# =============================================================================
# WINDOWS: КОПИРОВАНИЕ DLL ФАЙЛОВ
# =============================================================================

if(OS_WINDOWS)
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
endif()

# =============================================================================
# СООБЩЕНИЕ ПРИ УСПЕШНОЙ КОНФИГУРАЦИИ
# =============================================================================

message(STATUS "========================================")
message(STATUS "Sigma Encryptor - Конфигурация завершена")
message(STATUS "ОС: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Компилятор: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Стандарт C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")

