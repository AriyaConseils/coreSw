cmake_minimum_required(VERSION 3.10)
project(SimpleWindowExample)

# Utiliser le standard C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ajouter l'exécutable GuiApplication
add_executable(GuiApplication GuiApplication.cpp)

# Lier la bibliothèque statique SwCore
target_link_libraries(GuiApplication PRIVATE gdiplus)

# Inclure le répertoire de src/core pour les en-têtes
target_include_directories(GuiApplication PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
