cmake_minimum_required(VERSION 3.10)
project(coreSw)

# Utiliser le standard C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Définir les fichiers sources
set(SOURCE_FILES
    CoreApplication.cpp
    SwAny.cpp
    SwStyle.cpp
)

set(HEADER_FILES
    SwFlags.h
    Sw.h
    SwFont.h
    CoreApplication.h
    SwAny.h
    GuiApplication.h
    EventLoop.h
    Timer.h
    Object.h
    IODevice.h
    IODescriptor.h
    SwProcess.h
    SwStyle.h
    SwPainter.h
    StyleSheet.h
    Widget.h
    PushButton.h
    Label.h
    LineEdit.h
    MainWindow.h
    # Ajouter tous les autres .h ici
)

# Créer la bibliothèque statique
add_library(coreSw STATIC ${SOURCE_FILES} ${HEADER_FILES})

# Définir les options d'inclusion
target_include_directories(coreSw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Lier la bibliothèque GDI+
target_link_libraries(coreSw PRIVATE gdiplus)

# Si le projet est configuré pour le sous-système Windows
target_link_options(coreSw PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
