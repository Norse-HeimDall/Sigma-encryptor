# CMakeLists.txt - Sigma Encryptor (Windows Static Build)
cmake_minimum_required(VERSION 3.15)

# Указываем vcpkg использовать статический триплет до вызова project()
if (NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static")
endif()

project(SigmaEncryptor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем политику для управления рантаймом MSVC
cmake_policy(SET CMP0091 NEW)

# /MT (статический рантайм)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Специфичные настройки компилятора MSVC
if (MSVC)
    add_compile_options(/W4 /utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# =============================================================================
# ЗАВИСИМОСТИ
# =============================================================================
find_package(GLEW REQUIRED)
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

#GLFW_STATIC, чтобы заголовки понимали статический режим
target_compile_definitions(SigmaEncryptor PUBLIC SIGMA_STATIC GLFW_STATIC)

set_target_properties(SigmaEncryptor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# =============================================================================
# LINK LIBRARIES
# =============================================================================
target_link_libraries(SigmaEncryptor PRIVATE 
    OpenGL::GL
    OpenSSL::Crypto
    OpenSSL::SSL
    imgui::imgui
    glfw
    ${GLEW_LIBRARIES}  # Используем переменную вместо цели
    ws2_32 crypt32 advapi32 user32 gdi32 shell32 ucrt.lib vcruntime.lib msvcrt.lib
)

# профилактика конфликта LNK4098 (заставляем линкер забыть про динамический рантайм)
set_target_properties(SigmaEncryptor PROPERTIES 
    LINK_FLAGS "/NODEFAULTLIB:MSVCRT"
)

# =============================================================================
# INCLUDES
# =============================================================================
target_include_directories(SigmaEncryptor PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/crypto
    # Это позволит компилятору находить файлы через #include "imgui_impl_glfw.h"
    "C:/Users/HeimDall/vcpkg/installed/x64-windows-static/include/imgui"
)

# Блок копирования DLL теперь фактически не нужен при полной статике
add_custom_command(TARGET SigmaEncryptor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        "$<TARGET_RUNTIME_DLLS:SigmaEncryptor>"
        "$<TARGET_FILE_DIR:SigmaEncryptor>"
    COMMAND_EXPAND_LISTS
    COMMENT "Checking for runtime dependencies..."
)

message(STATUS "=== Sigma Encryptor 1.0: Windows STATIC Build Configured ===")