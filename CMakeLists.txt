# CMakeLists.txt - Конфигурация сборки Sigma Encryptor
# Совместим с vcpkg toolchain (portable, без hardcoded путей)

cmake_minimum_required(VERSION 3.15)
project(SigmaEncryptor VERSION 4.0 LANGUAGES CXX)

# C++17 стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC Runtime: DLL для vcpkg
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Предупреждения
if (MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

# =============================================================================
# VCPKG БИБЛИОТЕКИ (авто через toolchain)
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
    src/application.hpp
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

# Выход в build/bin
set_target_properties(SigmaEncryptor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
)

# =============================================================================
# LINK LIBRARIES
# =============================================================================
target_link_libraries(SigmaEncryptor PRIVATE 
    glfw::glfw
    imgui::imgui
    OpenGL::GL
    OpenSSL::Crypto
    OpenSSL::SSL
)

# =============================================================================
# INCLUDES (portable)
# =============================================================================
target_include_directories(SigmaEncryptor PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/crypto
)

# =============================================================================
# COPY DLLs (POST-BUILD)
# =============================================================================
if (WIN32)
    # Авто-копирование DLL из vcpkg (стандартный способ)
    find_file(GLFW_DLL glfw3.dll HINTS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin)
    find_file(LIBCRYPTO libcrypto-3-x64.dll HINTS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin)
    find_file(LIBSSL libssl-3-x64.dll HINTS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin)

    if (GLFW_DLL)
        add_custom_command(TARGET SigmaEncryptor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                "$<TARGET_FILE:glfw::glfw>" "$<TARGET_FILE_DIR:SigmaEncryptor>"
            COMMENT "Copy glfw3.dll"
        )
    endif()
    if (LIBCRYPTO AND LIBSSL)
        add_custom_command(TARGET SigmaEncryptor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBCRYPTO} "$<TARGET_FILE_DIR:SigmaEncryptor>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBSSL} "$<TARGET_FILE_DIR:SigmaEncryptor>"
            COMMENT "Copy OpenSSL DLLs"
        )
    endif()
endif()

# =============================================================================
# СТАТУС
# =============================================================================
message(STATUS "=== Sigma Encryptor 4.0 BUILD READY ===")
message(STATUS "Use: cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake")
message(STATUS "    cmake --build build --config Release")
message(STATUS "Run: build/bin/Release/SigmaEncryptor.exe")

