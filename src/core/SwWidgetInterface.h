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

#include "SwObject.h"

class PaintEvent;
class MouseEvent;
class KeyEvent;
class PaintEvent;
class StyleSheet;

class SwWidgetInterface : public SwObject {

    VIRTUAL_PROPERTY(SwFont, Font)
public:
    // Constructeur et destructeur
    SwWidgetInterface(SwObject* parent = nullptr) : SwObject(parent) {}
    virtual ~SwWidgetInterface() = default;

    // Méthodes purement virtuelles pour définir les fonctionnalités d'un widget
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void update() = 0;
    virtual void move(int newX, int newY) = 0;
    virtual void resize(int newWidth, int newHeight) = 0;

    // Méthodes pour gérer les événements
    virtual void paintEvent(PaintEvent* event) = 0;
    virtual void mousePressEvent(MouseEvent* event) = 0;
    virtual void mouseReleaseEvent(MouseEvent* event) = 0;
    virtual void mouseDoubleClickEvent(MouseEvent* event) = 0;
    virtual void mouseMoveEvent(MouseEvent* event) = 0;
    virtual void keyPressEvent(KeyEvent* event) = 0;

    // Méthodes pour obtenir ou définir des propriétés générales
    virtual StyleSheet* getToolSheet() = 0;
    virtual RECT getRect() const = 0;

//    virtual bool isVisible() const = 0;
//    virtual int getWidth() const = 0;
//    virtual int getHeight() const = 0;
//    virtual int getX() const = 0;
//    virtual int getY() const = 0;
//    virtual void setWidth(int width) = 0;
//    virtual void setHeight(int height) = 0;
//    virtual void setX(int x) = 0;
//    virtual void setY(int y) = 0;
};
