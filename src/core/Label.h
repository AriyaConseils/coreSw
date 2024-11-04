#pragma once

#include "Widget.h"
#include <windows.h>
#include <string>
#include <iostream>

#include "Sw.h"
#include "SwFont.h"


class Label : public Widget {

    SW_OBJECT(Label, Widget)

    CUSTOM_PROPERTY(std::wstring, Text, L"Label") {
        update();
    }

    CUSTOM_PROPERTY(DrawTextFormats, Alignment, DrawTextFormats(DrawTextFormat::Left | DrawTextFormat::VCenter)) {
        update();
    }

public:
    Label(const std::wstring& text, Widget* parent = nullptr)
        : Widget(parent) {
        width = 200;  // Largeur par défaut
        height = 30;  // Hauteur par défaut
        REGISTER_PROPERTY(Text);
        REGISTER_PROPERTY(Alignment);

        // Définition d'un style CSS pour le Label
        std::string css = R"(
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

    Label(Widget* parent = nullptr)
        : Widget(parent) {
        width = 200;  // Largeur par défaut
        height = 30;  // Hauteur par défaut

        REGISTER_PROPERTY(Text);

        // Définition d'un style CSS pour le Label
        std::string css = R"(
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
        Widget::mouseMoveEvent(event);
    }

    virtual void mousePressEvent(MouseEvent* event) override {
        Widget::mousePressEvent(event);
    }

    virtual void mouseReleaseEvent(MouseEvent* event) override {
        Widget::mouseReleaseEvent(event);
    }
};

