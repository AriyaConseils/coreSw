cmake_minimum_required(VERSION 3.10)
project(RegisterType)

# Utiliser le standard C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ajouter l'exécutable RegisterType
add_executable(RegisterType RegisterType.cpp)

# Inclure le répertoire de src/core pour les en-têtes
target_include_directories(RegisterType PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
