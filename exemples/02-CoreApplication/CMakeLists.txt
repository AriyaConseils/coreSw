cmake_minimum_required(VERSION 3.10)
project(SimpleConsoleExample)

# Utiliser le standard C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ajouter l'exécutable ConsoleApplication
add_executable(ConsoleApplication ConsoleApplication.cpp)

# Lier la bibliothèque statique SwCore
# target_link_libraries(ConsoleApplication PRIVATE SwCore)

# Inclure le répertoire de src/core pour les en-têtes
target_include_directories(ConsoleApplication PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
