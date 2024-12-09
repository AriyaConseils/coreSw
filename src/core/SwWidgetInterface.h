#pragma once

#include "Object.h"

class PaintEvent;
class MouseEvent;
class KeyEvent;
class PaintEvent;
class StyleSheet;

class SwWidgetInterface : public Object {

    VIRTUAL_PROPERTY(SwFont, Font)
public:
    // Constructeur et destructeur
    SwWidgetInterface(Object* parent = nullptr) : Object(parent) {}
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
