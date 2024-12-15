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


#include "SwWidget.h"
#include <windows.h>
#include <iostream>



class PushButton : public SwWidget {

    SW_OBJECT(PushButton, SwWidget)

    CUSTOM_PROPERTY(SwString, Text, "PushButton") {
        update();
    }
    CUSTOM_PROPERTY(bool, Pressed, false) {
        update();
    }
    CUSTOM_PROPERTY(DrawTextFormats, Alignment, DrawTextFormats(DrawTextFormat::Center | DrawTextFormat::VCenter | DrawTextFormat::SingleLine)) {
        update();
    }
public:
    PushButton(const SwString& text, SwWidget* parent = nullptr)
        : SwWidget(parent){
        width = 150;
        height = 50;

        setText(text);

        SwString css = R"(
            PushButton {
                background-color: #4CAF50;  /* Vert */
                border-color: #565456;      /* Vert foncé */
                color: #FF0000;             /* Blanc */
                border-radius: 10px;        /* Coins arrondis */
                padding: 10px 20px;         /* Espace interne */
                border-width: 2px;          /* Épaisseur de la bordure */
            }

            SwWidget {
                background-color: #FFFFFF;
                color: #000000;
            }
        )";
        this->setStyleSheet(css);
    }

    virtual void paintEvent(PaintEvent* event) override {
        SwPainter painter(event->context());

        RECT rect = getRect();

        WidgetState state = WidgetState::Normal;
        if (getPressed()) {
            state = WidgetStateHelper::setState(state, WidgetState::Pressed);
        }
        if (getHover()) {
            state = WidgetStateHelper::setState(state, WidgetState::Hovered);
        }
        //if (getEnable()) {
        //    state = WidgetStateHelper::setState(state, WidgetState::Disabled);
        //}
        //if (getFocus()) {
        //    state = WidgetStateHelper::setState(state, WidgetState::Focused);
        //}

        RECT paintRect = event->getPaintRect();

        m_style->drawControl(WidgetStyle::PushButtonStyle, &rect, &painter, this, state);

    }




    // Gérer le survol de la souris
    virtual void mouseMoveEvent(MouseEvent* event) override {
        SwWidget::mouseMoveEvent(event);
    }

    // Gérer le clic sur le bouton
    virtual void mousePressEvent(MouseEvent* event) override {
        if (isPointInside(event->x(), event->y())) {
            setPressed(true);
            event->accept();
        }
        SwWidget::mousePressEvent(event);
    }

    // Gérer le relâchement du bouton
    virtual void mouseReleaseEvent(MouseEvent* event) override {
        if (getPressed() && isPointInside(event->x(), event->y())) {
            emit clicked();  // Émettre le signal 'clicked'
            event->accept();
        }
        setPressed(false);
        SwWidget::mouseReleaseEvent(event);
    }

signals:
    DECLARE_SIGNAL(clicked);

private:

};
