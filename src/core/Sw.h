#pragma once
#include "SwFlags.h"


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

