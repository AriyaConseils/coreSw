#pragma once

#include <windows.h>
#include <string>
#include "SwPainter.h"  // Inclusion de SwPainter
class Widget;  // Déclaration anticipée de Widget


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
    SwStyle();
    ~SwStyle();

    // Méthode drawControl
    void drawControl(WidgetStyle style, const RECT* rect, SwPainter* painter, Widget* wdgt, WidgetState state);
    void drawBackground(const RECT* rect, SwPainter* painter, COLORREF color, bool noBorder = true);
    // Méthodes pour récupérer les propriétés d'apparence
    HBRUSH getNormalBrush() const;
    HBRUSH getHoverBrush() const;
    HBRUSH getPressedBrush() const;
    HPEN getBorderPen() const;
    COLORREF getTextColor() const;

    // Méthodes pour changer les propriétés d'apparence
    void setNormalBrush(COLORREF color);
    void setHoverBrush(COLORREF color);
    void setPressedBrush(COLORREF color);
    void setBorderPen(COLORREF color);
    void setTextColor(COLORREF color);

private:
    HBRUSH normalBrush;
    HBRUSH hoverBrush;
    HBRUSH pressedBrush;
    HPEN borderPen;
    COLORREF textColor;
};
