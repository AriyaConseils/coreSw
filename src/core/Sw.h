#pragma once
#include "SwFlags.h"
/***************************************************************************************************
 * This file is part of a project developed by Ariya Consulting and Eymeric O'Neill.
 *
 * Copyright (C) [year] Ariya Consulting
 * Author/Creator: Eymeric O'Neill
 * Contact: +33 6 52 83 83 31
 * Email: eymeric.oneill@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ***************************************************************************************************/

#ifndef SW_UNUSED
#define SW_UNUSED(x) (void)(x);
#endif

// Définition des types avec le préfixe "Sw"
struct SwSize {
    int width;
    int height;
};

struct SwColor {
    int r, g, b;  // Rouge, Vert, Bleu
};

struct SwRect {
    int x, y;
    int width, height;
};


enum class EntryType {
    Files = 0x1,        // Inclure les fichiers
    Directories = 0x2,  // Inclure les répertoires
    AllEntries = Files | Directories // Tout inclure
};
SW_DECLARE_FLAGS(EntryTypes, EntryType)

enum WindowFlag {
    NoFlag = 0x0,
    FramelessWindowHint = 0x1,      // Fenêtre sans bordure
    NoMinimizeButton = 0x2,         // Pas de bouton de minimisation
    NoMaximizeButton = 0x4,         // Pas de bouton de maximisation
    NoCloseButton = 0x8,            // Pas de bouton de fermeture (attention, c'est un peu plus tricky sur Windows)
    ToolWindowHint = 0x10,          // Fenêtre outil (petite barre titre)
    StayOnTopHint = 0x20            // Toujours au-dessus (topmost)
};
SW_DECLARE_FLAGS(WindowFlags, WindowFlag)


enum class CursorType {
    Arrow,
    Hand,
    IBeam,
    Cross,
    Wait,
    SizeAll,
    SizeNS,
    SizeWE,
    SizeNWSE,
    SizeNESW,
    None,
    // Ajoutez d'autres types de curseurs si nécessaire
};


enum class FocusPolicyEnum {
    Accept,
    Strong,
    NoFocus,
    // Ajoutez d'autres types de curseurs si nécessaire
};

enum class EchoModeEnum {
    NormalEcho,                 // Texte normal
    NoEcho,                     // Aucun texte affiché
    PasswordEcho,               // Masque le texte (comme un champ de mot de passe)
    PasswordEchoOnEdit,         // Masque le texte sauf pendant la modification
};

enum FontWeight {
    DontCare = 0,
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Heavy = 900
};

// Déclaration de l'énumération DrawTextFormat
enum DrawTextFormat {
    Top = 0x00000000,
    Left = 0x00000000,
    Center = 0x00000001,
    Right = 0x00000002,
    VCenter = 0x00000004,
    Bottom = 0x00000008,
    WordBreak = 0x00000010,
    SingleLine = 0x00000020,
    ExpandTabs = 0x00000040,
    TabStop = 0x00000080,
    NoClip = 0x00000100,
    ExternalLeading = 0x00000200,
    CalcRect = 0x00000400,
    NoPrefix = 0x00000800,
    Internal = 0x00001000
};
SW_DECLARE_FLAGS(DrawTextFormats, DrawTextFormat)

