cmake_minimum_required(VERSION 3.10)
project(ConsoleApplication)

# Utiliser le standard C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ajouter le répertoire src/core pour construire la bibliothèque statique coreSw
add_subdirectory(src/core)

# Ajouter tous les exemples dans le dossier exemples
add_subdirectory(exemples/01-SimpleWindow)
