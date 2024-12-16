cmake_minimum_required(VERSION 3.10)
project(SwCore)

# Utiliser le standard C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Ajouter le répertoire src/core pour construire la bibliothèque statique coreSw
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/core)

# Ajouter tous les exemples dans le dossier exemples
add_subdirectory(exemples/01-SimpleWindow)
add_subdirectory(exemples/02-CoreApplication)
add_subdirectory(exemples/03-TcpServer)
add_subdirectory(exemples/04-NetworkAccesManager)
add_subdirectory(exemples/05-InteractiveConsoleApplication)
add_subdirectory(exemples/06-DebugExemple)
add_subdirectory(exemples/07-ServeurDebug)
add_subdirectory(exemples/08-RegisterType)
add_subdirectory(exemples/09-MultiRuntime)
add_subdirectory(exemples/10-SwProcessExample)



