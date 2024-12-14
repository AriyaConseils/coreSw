#pragma once
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

#include <windows.h>
#include <string>
#include <vector>
#include <cmath>


class SwPainter {
public:
    SwPainter(HDC hdc) : hdc(hdc), currentBrush(nullptr), currentPen(nullptr) {}

    // Sélectionner une brosse dans le contexte de dessin
    void selectBrush(HBRUSH brush) {
        if (currentBrush != brush) {
            SelectObject(hdc, brush);
            currentBrush = brush;  // Mémoriser la brosse courante
        }
    }

    // Sélectionner un stylo dans le contexte de dessin
    void selectPen(HPEN pen) {
        if (currentPen != pen) {
            SelectObject(hdc, pen);
            currentPen = pen;  // Mémoriser le stylo courant
        }
    }

    void setTextColor(COLORREF color) {
        ::SetTextColor(hdc, color);
    }

    // Remplir un rectangle avec une brosse
    void fillRect(int x, int y, int width, int height) {
        RECT rect = { x, y, x + width, y + height };
        FillRect(hdc, &rect, currentBrush);
    }

    // Dessiner un rectangle standard
    void drawRect(int x, int y, int width, int height) {
        SelectObject(hdc, currentBrush);
        SelectObject(hdc, currentPen);
        Rectangle(hdc, x, y, x + width, y + height);
    }

    void drawRect(const RECT& rect) {
        SelectObject(hdc, currentBrush);
        SelectObject(hdc, currentPen);
        // Dessiner le rectangle
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    }


    // Dessiner un rectangle avec des coins arrondis
    void drawRoundedRect(int x, int y, int width, int height, int radius) {
        SelectObject(hdc, currentBrush);
        SelectObject(hdc, currentPen);
        RoundRect(hdc, x, y, x + width, y + height, radius, radius);
    }

    void drawRoundedRect(const RECT& rect, int radius) {
        SelectObject(hdc, currentBrush);
        SelectObject(hdc, currentPen);
        RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    }


    // Dessiner une ellipse
    void drawEllipse(int x, int y, int width, int height) {
        SelectObject(hdc, currentBrush);
        SelectObject(hdc, currentPen);
        Ellipse(hdc, x, y, x + width, y + height);
    }

    // Dessiner une ligne
    void drawLine(int x1, int y1, int x2, int y2) {
        SelectObject(hdc, currentPen);
        MoveToEx(hdc, x1, y1, nullptr);
        LineTo(hdc, x2, y2);
    }

    // Dessiner du texte avec alignement personnalisé
	void drawText(const std::string& text, RECT& rect, int alignmentFlags = DT_CENTER | DT_VCENTER | DT_SINGLELINE) {
		SetBkMode(hdc, TRANSPARENT);  // Fond transparent
		DrawTextA(hdc, text.c_str(), -1, &rect, alignmentFlags);  // Utilise DrawTextW pour Unicode
	}


    // Charger une police de caractères
	HFONT loadFont(const std::string& fontName, int size, bool bold = false, bool italic = false) {
		return CreateFontA(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, italic, 0, 0,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, fontName.c_str());
	}


    // Sélectionner la police dans le contexte
    void selectFont(HFONT font) {
        SelectObject(hdc, font);
    }

    // Dessiner une image bitmap
    void drawImage(HBITMAP hBitmap, int x, int y, int width, int height) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        SelectObject(hdcMem, hBitmap);
        BitBlt(hdc, x, y, width, height, hdcMem, 0, 0, SRCCOPY);
        DeleteDC(hdcMem);  // Nettoyer le contexte de dessin en mémoire
    }

    // Dessiner une image avec transparence (alpha blending)
    void drawImageTransparent(HBITMAP hBitmap, int x, int y, int width, int height, COLORREF transparentColor) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        SelectObject(hdcMem, hBitmap);
        TransparentBlt(hdc, x, y, width, height, hdcMem, 0, 0, width, height, transparentColor);
        DeleteDC(hdcMem);  // Nettoyer le contexte de dessin en mémoire
    }

    // Remplir avec un dégradé linéaire
	void fillGradientRect(int x, int y, int width, int height, COLORREF colorStart, COLORREF colorEnd) {
		TRIVERTEX vertex[2] = {
			{ x, y, static_cast<COLOR16>(GetRValue(colorStart) << 8), static_cast<COLOR16>(GetGValue(colorStart) << 8), static_cast<COLOR16>(GetBValue(colorStart) << 8), 0xFF00 },
			{ x + width, y + height, static_cast<COLOR16>(GetRValue(colorEnd) << 8), static_cast<COLOR16>(GetGValue(colorEnd) << 8), static_cast<COLOR16>(GetBValue(colorEnd) << 8), 0xFF00 }
		};
		GRADIENT_RECT gRect = { 0, 1 };
		GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
	}


    // Appliquer une rotation (en radians)
    void rotate(float angle) {
        XFORM xForm;
        xForm.eM11 = static_cast<FLOAT>(std::cos(angle));
        xForm.eM12 = static_cast<FLOAT>(std::sin(angle));
        xForm.eM21 = static_cast<FLOAT>(-std::sin(angle));
        xForm.eM22 = static_cast<FLOAT>(std::cos(angle));
        xForm.eDx = 0;
        xForm.eDy = 0;
        SetWorldTransform(hdc, &xForm);
    }
    // Appliquer une mise à l'échelle
    void scale(float sx, float sy) {
        XFORM xForm;
        xForm.eM11 = sx;
        xForm.eM12 = 0;
        xForm.eM21 = 0;
        xForm.eM22 = sy;
        xForm.eDx = 0;
        xForm.eDy = 0;
        SetWorldTransform(hdc, &xForm);
    }

    // Déplacer le contexte de dessin (translation)
    void translate(int dx, int dy) {
        XFORM xForm;
        xForm.eM11 = 1.0f;
        xForm.eM12 = 0.0f;
        xForm.eM21 = 0.0f;
        xForm.eM22 = 1.0f;
        xForm.eDx = static_cast<FLOAT>(dx);
        xForm.eDy = static_cast<FLOAT>(dy);
        ModifyWorldTransform(hdc, &xForm, MWT_RIGHTMULTIPLY);
    }

    // Rétablir l'origine de la vue après une transformation
    void resetTransform() {
        // SetGraphicsMode(hdc, GM_COMPATIBLE);
        XFORM identity = { 1, 0, 0, 1, 0, 0 };
        SetWorldTransform(hdc, &identity);
    }

    // Dessiner un chemin personnalisé à partir de points
    void drawPath(const std::vector<POINT>& points, HPEN pen) {
        SelectObject(hdc, pen);
        MoveToEx(hdc, points[0].x, points[0].y, nullptr);
        for (size_t i = 1; i < points.size(); ++i) {
            LineTo(hdc, points[i].x, points[i].y);
        }
    }

    // Dessiner un masque (forme transparente)
    void drawMask(HBITMAP hBitmapMask, int x, int y, int width, int height) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        SelectObject(hdcMem, hBitmapMask);
        BitBlt(hdc, x, y, width, height, hdcMem, 0, 0, SRCAND);
        DeleteDC(hdcMem);  // Nettoyer le contexte de dessin en mémoire
    }

    // Finaliser les opérations de dessin
    void finalize() {
        // Placeholder pour tout nettoyage ou fin d'opération, si nécessaire
    }

    HDC context() {
        return hdc;
    }
    
private:
    HDC hdc;  // Handle pour le contexte de dessin
    HBRUSH currentBrush;  // Mémoriser la brosse courante
    HPEN currentPen;      // Mémoriser le stylo courant
};
