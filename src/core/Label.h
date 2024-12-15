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

