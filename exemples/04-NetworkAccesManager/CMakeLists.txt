cmake_minimum_required(VERSION 3.10)
project(NetworkAccesManager)

# Utiliser le standard C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ajouter l'exécutable NetworkAccesManager
add_executable(NetworkAccesManager NetworkAccesManager.cpp)

# Lier la bibliothèque statique ws2_32 
target_link_libraries(NetworkAccesManager PRIVATE ws2_32)

# Inclure le répertoire de src/core pour les en-têtes
target_include_directories(NetworkAccesManager PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
