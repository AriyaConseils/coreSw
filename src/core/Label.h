#pragma once
#include <windows.h>
#include "Sw.h"

#include "SwWidget.h"
#include "SwString.h"

class Label : public SwWidget {

    SW_OBJECT(Label, SwWidget)

    CUSTOM_PROPERTY(SwString, Text, "Label") {
        update();
    }

    CUSTOM_PROPERTY(DrawTextFormats, Alignment, DrawTextFormats(DrawTextFormat::Left | DrawTextFormat::VCenter)) {
        update();
    }

public:
    Label(const SwString& text, SwWidget* parent = nullptr)
        : SwWidget(parent) {
        width = 200;  // Largeur par défaut
        height = 30;  // Hauteur par défaut
        REGISTER_PROPERTY(Text);
        REGISTER_PROPERTY(Alignment);

        // Définition d'un style CSS pour le Label
        SwString css = R"(
            Label {
                background-color: #FFFFFF;  /* Blanc */
                color: #000000;             /* Noir */
                border-width: 0px;          /* Pas de bordure */
                padding: 5px;               /* Padding pour le texte */
            }
        )";
        this->setStyleSheet(css);
        this->setFocusPolicy(FocusPolicyEnum::NoFocus);
        this->setText(text);
    }

    Label(SwWidget* parent = nullptr)
        : SwWidget(parent) {
        width = 200;  // Largeur par défaut
        height = 30;  // Hauteur par défaut

        REGISTER_PROPERTY(Text);

        // Définition d'un style CSS pour le Label
        SwString css = R"(
            Label {
                background-color: rgb(100, 149, 237);  /* Blanc */
                color: #000000;             /* Noir */
                border-width: 0px;          /* Pas de bordure */
                padding: 5px;               /* Padding pour le texte */
            }
        )";
        this->setStyleSheet(css);
        this->setFocusPolicy(FocusPolicyEnum::NoFocus);
    }

    // Surcharge de la méthode paintEvent pour dessiner le Label
    virtual void paintEvent(PaintEvent* event) override {
        SwPainter painter(event->context());

        RECT rect = getRect();

        WidgetState state = WidgetState::Normal;
        //if (getHover()) {
        //    state = WidgetStateHelper::setState(state, WidgetState::Hovered);
        //}
        //if (getFocus()) {
        //    state = WidgetStateHelper::setState(state, WidgetState::Focused);
        //}

        // Dessiner le label avec le style CSS et l'état défini
        m_style->drawControl(WidgetStyle::LabelStyle, &rect, &painter, this, WidgetState::Normal);
    }

    virtual void mouseMoveEvent(MouseEvent* event) override {
        SwWidget::mouseMoveEvent(event);
    }

    virtual void mousePressEvent(MouseEvent* event) override {
        SwWidget::mousePressEvent(event);
    }

    virtual void mouseReleaseEvent(MouseEvent* event) override {
        SwWidget::mouseReleaseEvent(event);
    }
};

