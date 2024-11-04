#pragma once

#include <windows.h>
#include <string>
#include <iostream>
#include "Sw.h"


class SwFont {
public:
    // Constructeurs
    SwFont(const std::wstring& family = L"Segoe UI", int pointSize = 9, FontWeight weight = Normal, bool italic = false, bool underline = false)
        : familyName(family), pointSize(pointSize), weight(weight), italic(italic), underline(underline) {
    }

    SwFont(const SwFont& other)
        : familyName(other.familyName), pointSize(other.pointSize), weight(other.weight), italic(other.italic), underline(other.underline) {
    }

    // Comparaison des polices
    bool operator==(const SwFont& other) const {
        return familyName == other.familyName &&
            pointSize == other.pointSize &&
            weight == other.weight &&
            italic == other.italic &&
            underline == other.underline;
    }

    SwFont& operator=(const SwFont& other) {
        if (this != &other) {
            familyName = other.familyName;
            pointSize = other.pointSize;
            weight = other.weight;
            italic = other.italic;
            underline = other.underline;
        }
        return *this;
    }

    ~SwFont() {
        if (hFont) {
            DeleteObject(hFont);
        }
    }

    // Getters et Setters
    void setFamily(const std::wstring& family) {
        if (familyName != family) {
            familyName = family;
        }
    }

    std::wstring getFamily() const {
        return familyName;
    }

    void setPointSize(int size) {
        if (pointSize != size) {
            pointSize = size;
        }
    }

    int getPointSize() const {
        return pointSize;
    }

    void setWeight(FontWeight fontWeight) {
        if (weight != fontWeight) {
            weight = fontWeight;
        }
    }

    FontWeight getWeight() const {
        return weight;
    }

    void setItalic(bool isItalic) {
        if (italic != isItalic) {
            italic = isItalic;
        }
    }

    bool isItalic() const {
        return italic;
    }

    void setUnderline(bool isUnderline) {
        if (underline != isUnderline) {
            underline = isUnderline;
        }
    }

    bool isUnderline() const {
        return underline;
    }



    bool operator!=(const SwFont& other) const {
        return !(*this == other);
    }

    // Utilisation du handle GDI pour le dessin
    HFONT handle(HDC context) {
        m_context = context;
        updateFontHandle();
        return hFont;
    }

private:
    std::wstring familyName;
    int pointSize;
    FontWeight weight;
    bool italic;
    bool underline;
    HFONT hFont = nullptr;
    HDC m_context;

    // Mise à jour de la police GDI
    void updateFontHandle() {
        if (hFont) {
            DeleteObject(hFont);
        }

		hFont = CreateFontW(
			-MulDiv(pointSize, GetDeviceCaps(m_context, LOGPIXELSY), 72), // Convertir la taille de points en pixels
			0,                        // Largeur de la police
			0,                        // Orientation de l'angle d'écriture
			0,                        // Angle de l'axe de base
			weight,                   // Poids de la police (FW_NORMAL, FW_BOLD, etc.)
			italic,                   // Italique
			underline,                // Souligné
			FALSE,                    // Barré
			DEFAULT_CHARSET,          // Jeu de caractères
			OUT_DEFAULT_PRECIS,       // Précision de sortie
			CLIP_DEFAULT_PRECIS,      // Précision de découpage
			DEFAULT_QUALITY,          // Qualité de rendu de la police
			DEFAULT_PITCH | FF_DONTCARE, // "Pitch" et famille de caractères
			familyName.c_str()        // Nom de la police (wstring)
		);


        if (!hFont) {
            std::cerr << "Erreur lors de la création de la police." << std::endl;
        }
    }
};

// Exemple d'utilisation
/*
int main() {
    SwFont myFont(L"Times New Roman", 16, FW_BOLD, true, false);
    HDC hdc = GetDC(nullptr);
    HFONT oldFont = (HFONT)SelectObject(hdc, myFont.handle());

    TextOut(hdc, 10, 10, L"Texte avec une police personnalisée", 32);

    SelectObject(hdc, oldFont);
    ReleaseDC(nullptr, hdc);
    return 0;
}
*/