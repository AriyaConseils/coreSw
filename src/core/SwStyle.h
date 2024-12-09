#pragma once

#include <windows.h>
#include <string>
#include "SwPainter.h"
#include "SwWidgetInterface.h"
#include "StyleSheet.h"

enum class WidgetState {
    Normal = 0x00,
    Hovered = 0x01,
    Pressed = 0x02,
    Disabled = 0x04,
    Focused = 0x08,
    Checked = 0x10
};


class WidgetStateHelper {
public:
    static bool isState(WidgetState state, WidgetState flag) {
        return (static_cast<int>(state) & static_cast<int>(flag)) != 0;
    }

    static WidgetState setState(WidgetState state, WidgetState flag) {
        return static_cast<WidgetState>(static_cast<int>(state) | static_cast<int>(flag));
    }

    static WidgetState clearState(WidgetState state, WidgetState flag) {
        return static_cast<WidgetState>(static_cast<int>(state) & ~static_cast<int>(flag));
    }
};


enum class WidgetStyle {
    WidgetStyle,        // Style générique pour un widget de base
    PushButtonStyle,    // Style pour un bouton
    LineEditStyle,      // Style pour un champ de saisie de texte (LineEdit)
    CheckBoxStyle,      // Style pour une case à cocher
    RadioButtonStyle,   // Style pour un bouton radio
    LabelStyle,         // Style pour une étiquette (texte statique)
    ComboBoxStyle,      // Style pour une liste déroulante
    SpinBoxStyle,       // Style pour un champ numérique avec flèches
    ProgressBarStyle,   // Style pour une barre de progression
    SliderStyle,        // Style pour un curseur
    TextEditStyle,      // Style pour un champ de texte multi-ligne
    ScrollBarStyle,     // Style pour une barre de défilement
    ToolButtonStyle,    // Style pour un bouton dans une barre d'outils
    TabWidgetStyle,     // Style pour un ensemble d'onglets
    ListViewStyle,      // Style pour une liste d'éléments
    TableViewStyle,     // Style pour un tableau de données
    TreeViewStyle,      // Style pour une vue arborescente
    DialogStyle,        // Style pour une boîte de dialogue
    MainWindowStyle,    // Style pour une fenêtre principale
    StatusBarStyle,     // Style pour une barre d'état
    MenuBarStyle,       // Style pour une barre de menu
    ToolBarStyle,       // Style pour une barre d'outils
    SplitterStyle,      // Style pour un séparateur
    ScrollAreaStyle,    // Style pour une zone défilable
    GroupBoxStyle,      // Style pour un conteneur avec bordure et titre
    CalendarStyle,      // Style pour un calendrier
    MessageBoxStyle     // Style pour une boîte de message
};

class SwStyle {
public:
    SwStyle() {
        // Initialisation des valeurs par défaut
        normalBrush = CreateSolidBrush(RGB(70, 130, 180));  // Couleur normale
        hoverBrush = CreateSolidBrush(RGB(100, 149, 237));  // Couleur au survol
        pressedBrush = CreateSolidBrush(RGB(50, 50, 200));  // Couleur enfoncée
        borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));   // Bordure noire
        textColor = RGB(255, 255, 255);  // Texte blanc
    }


    ~SwStyle() {
        DeleteObject(normalBrush);
        DeleteObject(hoverBrush);
        DeleteObject(pressedBrush);
        DeleteObject(borderPen);
    }


    // Méthode drawControl
    void drawControl(WidgetStyle style, const RECT* rect, SwPainter* painter, SwWidgetInterface* wdgt, WidgetState state) {
        if (!wdgt) {
            // Si le widget est nul, ne rien faire
            return;
        }

        // Récupérer la hiérarchie des classes pour le widget
        std::vector<SwString> hierarchy = wdgt->classHierarchy();

        // Obtenir le style via styleSheet
        StyleSheet* sheet = wdgt->getToolSheet(); // On suppose que Widget a une méthode getToolSheet()

        // Variables pour les styles
        HBRUSH brush = nullptr;
        HPEN pen = nullptr;
        COLORREF textColor = RGB(0, 0, 0);
        SwString borderColor, bgColor, textStyle;
        int borderWidth = 1; // Par défaut, on met une bordure de 1 pixel

        // Appliquer les styles spécifiques trouvés dans la hiérarchie
        for (int i = hierarchy.size() - 1; i >= 0; --i) {
            const SwString& className = hierarchy[i];

            // Récupérer la propriété CSS "background-color"
            SwString tempBgColor = sheet->getStyleProperty(className, "background-color");
            if (!tempBgColor.isEmpty()) {
                bgColor = tempBgColor;
            }

            // Récupérer la propriété CSS "border-color"
            SwString tempBorderColor = sheet->getStyleProperty(className, "border-color");
            if (!tempBorderColor.isEmpty()) {
                borderColor = tempBorderColor;
            }

            // Récupérer la propriété CSS "border-width"
            SwString tempBorderWidth = sheet->getStyleProperty(className, "border-width");
            if (!tempBorderWidth.isEmpty()) {
                borderWidth = std::stoi(tempBorderWidth);  // Convertir en entier
            }

            // Récupérer la propriété CSS "color" (couleur du texte)
            SwString tempTextColor = sheet->getStyleProperty(className, "color");
            if (!tempTextColor.isEmpty()) {
                textStyle = tempTextColor;
            }
        }

        // Appliquer les propriétés en fonction de l'état
        if (WidgetStateHelper::isState(state, WidgetState::Pressed)) {
            brush = CreateSolidBrush(RGB(200, 200, 200)); // Couleur plus foncée si pressé
        }
        else if (WidgetStateHelper::isState(state, WidgetState::Hovered)) {
            brush = CreateSolidBrush(RGB(220, 220, 220)); // Couleur plus claire si survolé
        }
        else {
            if (!bgColor.isEmpty()) {
                brush = CreateSolidBrush(sheet->parseColor(bgColor));  // Convertir la couleur CSS en COLORREF
            }
        }

        if (borderWidth == 0) {
            pen = (HPEN)GetStockObject(NULL_PEN);
        } else {
            pen = CreatePen(PS_SOLID, borderWidth, sheet->parseColor(borderColor));  // Créer la bordure avec la couleur et l'épaisseur
        }

        if (!textStyle.isEmpty()) {
            textColor = sheet->parseColor(textStyle);  // Convertir la couleur CSS en COLORREF
        }

        // Si aucun style spécifique n'a été trouvé, on applique les styles par défaut
        if (!brush) {
            switch (style) {
            case WidgetStyle::PushButtonStyle:
            case WidgetStyle::CheckBoxStyle:
            case WidgetStyle::RadioButtonStyle:
                brush = getNormalBrush();
                pen = getBorderPen();
                textColor = getTextColor();
                break;
            case WidgetStyle::LabelStyle:
                brush = getNormalBrush();
                textColor = getTextColor();
                break;
            default:
                brush = getNormalBrush();
                pen = getBorderPen();
                textColor = getTextColor();
                break;
            }
        }

        // Dessiner la forme du widget avec le pinceau et la bordure appropriée
        painter->selectBrush(brush);
        painter->selectPen(pen);

        // Gestion des coins arrondis pour les boutons ou les boîtes (par exemple)
        int cornerRadius = 0;
        if (style == WidgetStyle::PushButtonStyle || style == WidgetStyle::ToolButtonStyle) {
            SwString borderRadius = sheet->getStyleProperty("border-radius", "border-radius");
            if (!borderRadius.isEmpty()) {
                cornerRadius = std::stoi(borderRadius); // Convertir en int
            }
            else {
                cornerRadius = 10; // Par défaut
            }
        }

        if (cornerRadius > 0) {
            painter->drawRoundedRect(*rect, cornerRadius);
        }
        else {
            painter->drawRect(*rect);  // Dessiner un rectangle classique
        }



        // Dessiner le texte si applicable (pour un bouton ou un label, par exemple)
        if (style == WidgetStyle::PushButtonStyle || style == WidgetStyle::LabelStyle) {
            // Récupérer la police (SwFont) depuis le widget
            SwFont myFont = wdgt->getFont();

            // Sélectionner la police dans le contexte de dessin
            HFONT hFont = myFont.handle(painter->context());  // `getHDC()` est une méthode de `SwPainter` pour obtenir le HDC
            HFONT oldFont = (HFONT)SelectObject(painter->context(), hFont);

            painter->setTextColor(sheet->parseColor(textStyle));

            SwString text;
            if (wdgt->propertyExist("Text")) {
                text = wdgt->property("Text").get<SwString>();
            }
            DrawTextFormats alignment;
            if (wdgt->propertyExist("Alignment")) {alignment = wdgt->property("Alignment").get<DrawTextFormats>();}

            RECT widgetRect = wdgt->getRect();
            painter->drawText(text, widgetRect, static_cast<int>(alignment));
            SelectObject(painter->context(), oldFont);
        }
        // Finaliser le dessin
        painter->finalize();
    }

    void drawBackground(const RECT* rect, SwPainter* painter, COLORREF color, bool noBorder = true) {
        // Créer un pinceau (brush) pour le fond avec la couleur donnée
        HBRUSH brush = CreateSolidBrush(color);

        // Sélectionner le pinceau dans le contexte de dessin
        painter->selectBrush(brush);

        // Si noBorder est vrai, on utilise un NULL_PEN pour ne pas dessiner de bordure
        if (noBorder) {
            HPEN nullPen = (HPEN)GetStockObject(NULL_PEN);  // Aucun stylo (pas de bordure)
            painter->selectPen(nullPen);
        }
        else {
            // Si on veut une bordure, on crée un stylo noir par défaut (ou une couleur que tu veux)
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));  // Bordure noire par défaut
            painter->selectPen(pen);

            // Nettoyer le stylo après utilisation
            DeleteObject(pen);
        }

        // Dessiner un rectangle plein avec les coins droits
        painter->drawRect(*rect);

        // Finaliser le dessin
        painter->finalize();

        // Nettoyer le pinceau après utilisation
        DeleteObject(brush);
    }


    // Méthodes pour récupérer les propriétés d'apparence
    HBRUSH getNormalBrush() const {
        return normalBrush;
    }

    HBRUSH getHoverBrush() const {
        return hoverBrush;
    }

    HBRUSH getPressedBrush() const {
        return pressedBrush;
    }

    HPEN getBorderPen() const {
        return borderPen;
    }

    COLORREF getTextColor() const {
        return textColor;
    }

    // Méthodes pour changer les propriétés d'apparence
    void setNormalBrush(COLORREF color) {
        DeleteObject(normalBrush);  // Supprimer l'ancienne brosse
        normalBrush = CreateSolidBrush(color);
    }

    void setHoverBrush(COLORREF color) {
        DeleteObject(hoverBrush);
        hoverBrush = CreateSolidBrush(color);
    }

    void setPressedBrush(COLORREF color) {
        DeleteObject(pressedBrush);
        pressedBrush = CreateSolidBrush(color);
    }

    void setBorderPen(COLORREF color) {
        DeleteObject(borderPen);
        borderPen = CreatePen(PS_SOLID, 1, color);
    }

    void setTextColor(COLORREF color) {
        textColor = color;
    }


private:
    HBRUSH normalBrush;
    HBRUSH hoverBrush;
    HBRUSH pressedBrush;
    HPEN borderPen;
    COLORREF textColor;
};
