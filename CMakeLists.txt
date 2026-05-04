# CMakeLists.txt - Sigma Encryptor (Windows Stable)
cmake_minimum_required(VERSION 3.15)
project(SigmaEncryptor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройка MSVC Runtime для корректной работы с vcpkg и DLL
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Специфичные настройки компилятора MSVC
if (MSVC)
    add_compile_options(/W4 /utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# =============================================================================
# ЗАВИСИМОСТИ (vcpkg автоматизирует поиск этих пакетов)
# =============================================================================
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# =============================================================================
# ИСХОДНИКИ
# =============================================================================
set(SOURCES
    src/main.cpp
    src/application.cpp
    src/gui/renderer.cpp
    src/gui/theme.cpp
    src/gui/ui.cpp
    src/gui/file_dialog.cpp
    src/crypto/encryptor.cpp
    src/crypto/aes.cpp
    src/crypto/chacha20.cpp
)

# =============================================================================
# EXECUTABLE
# =============================================================================
add_executable(SigmaEncryptor ${SOURCES})

# Важно: это сопоставляется с твоим макросом в encryptor.hpp
target_compile_definitions(SigmaEncryptor PUBLIC SIGMA_STATIC)

# Собираем всё в папку bin для удобства запуска
set_target_properties(SigmaEncryptor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# =============================================================================
# LINK LIBRARIES (Windows-specific targets)
# =============================================================================
target_link_libraries(SigmaEncryptor PRIVATE 
    OpenGL::GL
    OpenSSL::Crypto
    OpenSSL::SSL
    imgui::imgui
    glfw
)

# =============================================================================
# INCLUDES
# =============================================================================
target_include_directories(SigmaEncryptor PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/crypto
)

# =============================================================================
# АВТОМАТИЧЕСКОЕ КОПИРОВАНИЕ DLL
# Это магия, которая подтянет libcrypto-3-x64.dll и другие рядом с .exe
# =============================================================================
add_custom_command(TARGET SigmaEncryptor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        "$<TARGET_RUNTIME_DLLS:SigmaEncryptor>"
        "$<TARGET_FILE_DIR:SigmaEncryptor>"
    COMMAND_EXPAND_LISTS
    COMMENT "Copying runtime dependencies (DLLs) to build folder..."
)

message(STATUS "=== Sigma Encryptor 1.0: Windows Build Configured ===")