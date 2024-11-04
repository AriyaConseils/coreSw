#include "SwStyle.h"
#include "Widget.h"






SwStyle::SwStyle() {
    // Initialisation des valeurs par défaut
    normalBrush = CreateSolidBrush(RGB(70, 130, 180));  // Couleur normale
    hoverBrush = CreateSolidBrush(RGB(100, 149, 237));  // Couleur au survol
    pressedBrush = CreateSolidBrush(RGB(50, 50, 200));  // Couleur enfoncée
    borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));   // Bordure noire
    textColor = RGB(255, 255, 255);  // Texte blanc
}

SwStyle::~SwStyle() {
    DeleteObject(normalBrush);
    DeleteObject(hoverBrush);
    DeleteObject(pressedBrush);
    DeleteObject(borderPen);
}



void SwStyle::drawBackground(const RECT* rect, SwPainter* painter, COLORREF color, bool noBorder) {
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



void SwStyle::drawControl(WidgetStyle style, const RECT* rect, SwPainter* painter, Widget* wdgt, WidgetState state) {
    if (!wdgt) {
        // Si le widget est nul, ne rien faire
        return;
    }

    // Récupérer la hiérarchie des classes pour le widget
    std::vector<std::string> hierarchy = wdgt->classHierarchy();

    // Obtenir le style via styleSheet
    StyleSheet* sheet = wdgt->getToolSheet(); // On suppose que Widget a une méthode getToolSheet()

    // Variables pour les styles
    HBRUSH brush = nullptr;
    HPEN pen = nullptr;
    COLORREF textColor = RGB(0, 0, 0);
    std::string borderColor, bgColor, textStyle;
    int borderWidth = 1; // Par défaut, on met une bordure de 1 pixel

    // Appliquer les styles spécifiques trouvés dans la hiérarchie
    for (int i = hierarchy.size() - 1; i >= 0; --i) {
        const std::string& className = hierarchy[i];

        // Récupérer la propriété CSS "background-color"
        std::string tempBgColor = sheet->getStyleProperty(className, "background-color");
        if (!tempBgColor.empty()) {
            bgColor = tempBgColor;
        }

        // Récupérer la propriété CSS "border-color"
        std::string tempBorderColor = sheet->getStyleProperty(className, "border-color");
        if (!tempBorderColor.empty()) {
            borderColor = tempBorderColor;
        }

        // Récupérer la propriété CSS "border-width"
        std::string tempBorderWidth = sheet->getStyleProperty(className, "border-width");
        if (!tempBorderWidth.empty()) {
            borderWidth = std::stoi(tempBorderWidth);  // Convertir en entier
        }

        // Récupérer la propriété CSS "color" (couleur du texte)
        std::string tempTextColor = sheet->getStyleProperty(className, "color");
        if (!tempTextColor.empty()) {
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
        if (!bgColor.empty()) {
            brush = CreateSolidBrush(sheet->parseColor(bgColor));  // Convertir la couleur CSS en COLORREF
        }
    }

    if (borderWidth == 0) {
        pen = (HPEN)GetStockObject(NULL_PEN);
    } else {
        pen = CreatePen(PS_SOLID, borderWidth, sheet->parseColor(borderColor));  // Créer la bordure avec la couleur et l'épaisseur
    }

    if (!textStyle.empty()) {
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
        std::string borderRadius = sheet->getStyleProperty("border-radius", "border-radius");
        if (!borderRadius.empty()) {
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

        std::wstring text;
        if (wdgt->propertyExist("Text")) { 
            text = wdgt->property("Text").get<std::wstring>();
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



// Méthodes pour récupérer les propriétés d'apparence
HBRUSH SwStyle::getNormalBrush() const {
    return normalBrush;
}

HBRUSH SwStyle::getHoverBrush() const {
    return hoverBrush;
}

HBRUSH SwStyle::getPressedBrush() const {
    return pressedBrush;
}

HPEN SwStyle::getBorderPen() const {
    return borderPen;
}

COLORREF SwStyle::getTextColor() const {
    return textColor;
}

// Méthodes pour changer les propriétés d'apparence
void SwStyle::setNormalBrush(COLORREF color) {
    DeleteObject(normalBrush);  // Supprimer l'ancienne brosse
    normalBrush = CreateSolidBrush(color);
}

void SwStyle::setHoverBrush(COLORREF color) {
    DeleteObject(hoverBrush);
    hoverBrush = CreateSolidBrush(color);
}

void SwStyle::setPressedBrush(COLORREF color) {
    DeleteObject(pressedBrush);
    pressedBrush = CreateSolidBrush(color);
}

void SwStyle::setBorderPen(COLORREF color) {
    DeleteObject(borderPen);
    borderPen = CreatePen(PS_SOLID, 1, color);
}

void SwStyle::setTextColor(COLORREF color) {
    textColor = color;
}
