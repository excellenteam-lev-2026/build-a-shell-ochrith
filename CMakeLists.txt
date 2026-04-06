cmake_minimum_required(VERSION 3.10)

# Nom du projet
project(build-shell)

# Utiliser C++17
set(CMAKE_CXX_STANDARD 17)

# Ajouter ton fichier source
add_executable(shell main.cpp)

