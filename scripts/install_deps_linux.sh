#!/bin/bash
# Скрипт установки зависимостей для сборки Sigma Encryptor на Linux

set -e

echo "========================================="
echo "Установка зависимостей для Sigma Encryptor"
echo "========================================="

# Определяем тип системы
if [ -f /etc/debian_version ]; then
    # Debian/Ubuntu
    echo "Определена система: Debian/Ubuntu"
    
    echo "Обновление пакетов..."
    sudo apt-get update
    
    echo "Установка основных зависимостей..."
    sudo apt-get install -y \
        build-essential \
        cmake \
        pkg-config
    
    echo "Установка графических библиотек..."
    sudo apt-get install -y \
        libgl1-mesa-dev \
        libglfw3-dev \
        libglfw3
    
    echo "Установка OpenSSL..."
    sudo apt-get install -y \
        libssl-dev
    
    echo "Установка GTK3 для нативных диалогов..."
    sudo apt-get install -y \
        libgtk-3-dev
    
    echo "Установка X11 (опционально)..."
    sudo apt-get install -y \
        libx11-dev || true

elif [ -f /etc/fedora-release ]; then
    # Fedora/RHEL/CentOS
    echo "Определена система: Fedora/RHEL/CentOS"
    
    echo "Установка основных зависимостей..."
    sudo dnf install -y \
        gcc-c++ \
        cmake \
        pkg-config
    
    echo "Установка графических библиотек..."
    sudo dnf install -y \
        mesa-libGL-devel \
        mesa-libGLU-devel \
        glfw-devel
    
    echo "Установка OpenSSL..."
    sudo dnf install -y \
        openssl-devel
    
    echo "Установка GTK3..."
    sudo dnf install -y \
        gtk3-devel

elif [ -f /etc/arch-release ]; then
    # Arch Linux
    echo "Определена система: Arch Linux"
    
    echo "Установка основных зависимостей..."
    sudo pacman -S --noconfirm \
        base-devel \
        cmake \
        pkgconf
    
    echo "Установка графических библиотек..."
    sudo pacman -S --noconfirm \
        mesa \
        libglvnd \
        glfw-wayland \
        glfw-x11
    
    echo "Установка OpenSSL..."
    sudo pacman -S --noconfirm \
        openssl
    
    echo "Установка GTK3..."
    sudo pacman -S --noconfirm \
        gtk3

else
    echo "Неопределённая система. Пожалуйста, установите зависимости вручную."
    echo "Требуемые пакеты:"
    echo "  - build-essential / gcc-c++ / base-devel"
    echo "  - cmake"
    echo "  - pkg-config"
    echo "  - libgl1-mesa-dev / mesa-libGL-devel"
    echo "  - libglfw3-dev / glfw-devel"
    echo "  - libssl-dev / openssl-devel"
    echo "  - libgtk-3-dev / gtk3-devel"
    exit 1
fi

echo ""
echo "========================================="
echo "Проверка установленных версий"
echo "========================================="
echo "GCC: $(gcc --version | head -n1)"
echo "CMake: $(cmake --version | head -n1)"
echo "OpenSSL: $(openssl version)"
echo ""
echo "Проверка библиотек..."
pkg-config --modversion glfw3 && echo "GLFW3: установлен"
pkg-config --modversion openssl && echo "OpenSSL: установлен"
pkg-config --modversion gtk+-3.0 && echo "GTK3: установлен"

echo ""
echo "========================================="
echo "Установка завершена!"
echo "========================================="
echo ""
echo "Для сборки проекта выполните:"
echo "  cd build"
echo "  cmake .."
echo "  cmake --build ."
echo ""

