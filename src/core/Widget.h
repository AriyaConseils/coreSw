#pragma once

#include "Object.h"
#include <iostream>
#include <vector>
#include <gdiplus.h>
#include "SwFont.h"
#include "SwStyle.h"
#include "StyleSheet.h"
#include "SwString.h"
#include "Sw.h"

class SwStyle;



// Enum pour les types d'événements
enum class EventType {
    Paint,
    Resize,
    Move,
    KeyPress,
    MousePress,
    MouseDoubleClick,
    MouseRelease,
    Show,
    Hide
};

// Classe de base pour un événement
class Event {
public:
    Event(EventType type) : eventType(type), accepted(false) {}

    EventType type() const { return eventType; }

    // Marquer l'événement comme accepté
    void accept() { accepted = true; }

    // Marquer l'événement comme refusé
    void ignore() { accepted = false; }

    // Vérifier si l'événement a été accepté
    bool isAccepted() const { return accepted; }

private:
    EventType eventType;
    bool accepted;  // Nouveau membre pour suivre si l'événement est accepté ou non
};


// Classe pour l'événement de redimensionnement
class ResizeEvent : public Event {
public:
    ResizeEvent(int w, int h) : Event(EventType::Resize), newWidth(w), newHeight(h) {}

    int width() const { return newWidth; }
    int height() const { return newHeight; }

private:
    int newWidth, newHeight;
};

// Classe pour l'événement de dessin
class PaintEvent : public Event {
public:
    // Constructeur prenant en paramètre le HDC et le RECT
    PaintEvent(HDC hdc, const RECT& paintRect)
        : Event(EventType::Paint), hdc(hdc), paintRect(paintRect) {}

    // Retourner le contexte de périphérique HDC
    HDC context() const { return hdc; }

    // Retourner la zone à redessiner
    const RECT& getPaintRect() const { return paintRect; }

private:
    HDC hdc;         // Contexte de périphérique pour le dessin
    RECT paintRect;  // Zone de la fenêtre à redessiner
};

// Classe pour un clic de souris
class MouseEvent : public Event {
public:
    MouseEvent(EventType type, int xPos, int yPos)
        : Event(type), xPosition(xPos), yPosition(yPos), deltaX(0), deltaY(0), speedX(0), speedY(0) {}

    int x() const { return xPosition; }
    int y() const { return yPosition; }
    void setX(int xPos) { xPosition = xPos; }
    void setY(int yPos) { yPosition = yPos; }
    int getDeltaX() const { return deltaX; }
    void setDeltaX(int dx) { deltaX = dx; }
    int getDeltaY() const { return deltaY; }
    void setDeltaY(int dy) { deltaY = dy; }
    double getSpeedX() const { return speedX; }
    void setSpeedX(double sx) { speedX = sx; }
    double getSpeedY() const { return speedY; }
    void setSpeedY(double sy) { speedY = sy; }

private:
    int xPosition, yPosition;
    int deltaX, deltaY;
    double speedX, speedY;
};

// Classe pour un événement de clavier
class KeyEvent : public Event {
public:
    KeyEvent(int keyCode, bool ctrl = false, bool shift = false, bool alt = false)
        : Event(EventType::KeyPress), keyPressed(keyCode), ctrlPressed(ctrl), shiftPressed(shift), altPressed(alt) {}

    int key() const { return keyPressed; }
    bool isCtrlPressed() const { return ctrlPressed; }
    bool isShiftPressed() const { return shiftPressed; }
    bool isAltPressed() const { return altPressed; }

private:
    int keyPressed;
    bool ctrlPressed;
    bool shiftPressed;
    bool altPressed;
};



// Classe Widget héritant de Object, simulant QWidget
class Widget : public Object {

    SW_OBJECT(Widget, Object)

    PROPERTY(FocusPolicyEnum, FocusPolicy, FocusPolicyEnum::Accept)

    PROPERTY(SwString, ToolTips, "")

    CUSTOM_PROPERTY(bool, Enable, true) {
        update();
    }
    CUSTOM_PROPERTY(bool, Focus, false) {
        update();
    }
    CUSTOM_PROPERTY(bool, Hover, false) {
        update();
    }
    CUSTOM_PROPERTY(bool, Visible, true) {
        invalidateRect();
    }
    CUSTOM_PROPERTY(CursorType, Cursor, CursorType::None) {
        switch (m_Cursor) {
        case CursorType::Arrow:
            winPtrCursor = LoadCursor(NULL, IDC_ARROW);
            break;
        case CursorType::Hand:
            winPtrCursor = LoadCursor(NULL, IDC_HAND);
            break;
        case CursorType::IBeam:
            winPtrCursor = LoadCursor(NULL, IDC_IBEAM);
            break;
        case CursorType::Cross:
            winPtrCursor = LoadCursor(NULL, IDC_CROSS);
            break;
        case CursorType::Wait:
            winPtrCursor = LoadCursor(NULL, IDC_WAIT);
            break;
        case CursorType::SizeAll:
            winPtrCursor = LoadCursor(NULL, IDC_SIZEALL);
            break;
        case CursorType::SizeNS:
            winPtrCursor = LoadCursor(NULL, IDC_SIZENS);
            break;
        case CursorType::SizeWE:
            winPtrCursor = LoadCursor(NULL, IDC_SIZEWE);
            break;
        case CursorType::SizeNWSE:
            winPtrCursor = LoadCursor(NULL, IDC_SIZENWSE);
            break;
        case CursorType::SizeNESW:
            winPtrCursor = LoadCursor(NULL, IDC_SIZENESW);
            break;
        default:
            winPtrCursor = nullptr;  // Aucun curseur par défaut
            break;
        }
    }
    CUSTOM_PROPERTY(SwFont, Font, SwFont()) {
        update();
    }

    CUSTOM_PROPERTY(SwString, StyleSheet, "") {
        m_ComplexSheet.parseStyleSheet(m_StyleSheet);
        update();
    }

public:
    Widget(Widget* parent = nullptr)
        : Object(parent), width(100), height(100), x(0), y(0), m_style(new SwStyle()) {
        REGISTER_PROPERTY(Font);
        REGISTER_PROPERTY(Cursor);
        REGISTER_PROPERTY(Enable);
        REGISTER_PROPERTY(ToolTips);


        setCursor(CursorType::Arrow);
        if (Object::parent()) {
            this->newParentEvent(Object::parent());
        }
    }

    virtual ~Widget() {
        for (auto child : findChildren<Widget>()) {
            delete child;
        }
    }

    // Ajouter un enfant
    virtual void addChild(Widget* child) {
        Object::addChild(child);
    }

    // Méthode pour afficher le widget
    virtual void show() {
        std::cout << "Widget montré (" << x << ", " << y << ") taille: (" << width << "x" << height << ")" << std::endl;
        setVisible(true);
    }

    // Méthode pour cacher le widget
    virtual void hide() {
        std::cout << "Widget caché" << std::endl;
        setVisible(false);
    }

    // Méthode pour redessiner le widget
    virtual void update() {
        if (!getVisible()) {
            return;
        }
        invalidateRect();

        // Propagation de l'événement de dessin aux enfants
        for (Widget* child : findChildren<Widget>()) {
            child->update();
        }
    }

    // Méthode pour déplacer le widget
    virtual void move(int newX, int newY) {
        x = newX;
        y = newY;
        std::cout << "Widget déplacé à (" << x << ", " << y << ")" << std::endl;
        emit moved(x, y);
        if (getVisible()) {
            update();
        }
        // Propagation de l'événement de déplacement aux enfants
        for (Widget* child : findChildren<Widget>()) {
            child->move(newX, newY);  // Déplace les enfants relativement au parent
        }
    }

    // Méthode pour redimensionner le widget
    virtual void resize(int newWidth, int newHeight) {
        width = newWidth;
        height = newHeight;
        ResizeEvent event(width, height);
        resizeEvent(&event);
        emit resized(width, height);
    }

    RECT getRect() {
        RECT rectVal;
        rectVal.left = x;
        rectVal.top = y;
        rectVal.right = x + width;
        rectVal.bottom = y + height;
        return rectVal;
    }



    Widget* getChildUnderCursor(int x, int y) {
        Widget* deepestWidget = nullptr;

        // Parcourir tous les enfants
        for (Widget* child : findChildren<Widget>()) {
            // Vérifier si le pointeur est à l'intérieur de l'enfant
            if (child->isPointInside(x, y)) {
                // Appeler récursivement getChildUnderCursor pour trouver l'enfant le plus profond
                Widget* deepChild = child->getChildUnderCursor(x, y);
                if (deepChild) {
                    deepestWidget = deepChild;  // Si on trouve un enfant plus profond, on le garde
                }
                else {
                    deepestWidget = child;  // Sinon, on garde cet enfant
                }
            }
        }

        return deepestWidget;
    }



    StyleSheet* getToolSheet() {
        return &m_ComplexSheet;
    }

signals:
    DECLARE_SIGNAL(resized);
    DECLARE_SIGNAL(moved);
    DECLARE_SIGNAL(visibilityChanged);

protected:
    virtual void newParentEvent(Object* parent) override {
        Widget *ui = dynamic_cast<Widget*>(parent);
        hwnd = ui->hwnd;
        this->setVisible(ui->getVisible());
        Object::newParentEvent(parent);
    }

    virtual void paintEvent(PaintEvent* event) {
        if (!getVisible()) {
            return;
        }

        SwPainter painter(event->context());
        RECT rect = getRect();
        COLORREF bgColor = RGB(100, 149, 237); 
        this->m_style->drawBackground(&rect, &painter, bgColor);


        RECT paintRect = event->getPaintRect();
        for (Widget* child : findChildren<Widget>()) {
            RECT childRect = child->getRect();

            // Vérifier si l'enfant est visible et si son rectangle intersecte la zone de peinture
            if (child->getVisible() && rectsIntersect(paintRect, childRect)) {
                child->paintEvent(event);
            }
        }
    }

    // Fonction utilitaire pour vérifier si deux RECT Windows s'intersectent
    bool rectsIntersect(const RECT& r1, const RECT& r2) {
        return !(r1.right < r2.left ||   // r1 est complètement à gauche de r2
            r1.left > r2.right ||   // r1 est complètement à droite de r2
            r1.bottom < r2.top ||   // r1 est complètement au-dessus de r2
            r1.top > r2.bottom);    // r1 est complètement en dessous de r2
    }


    virtual void keyPressEvent(KeyEvent* event) {
        std::cout << "Widget redimensionné à (" << std::endl;

        for (Widget* child : findChildren<Widget>()) {
            if (event->isAccepted()) {
                return;
            }
            child->keyPressEvent(event);
        }
    }



    virtual void resizeEvent(ResizeEvent* event) {
        std::cout << "Widget redimensionné à (" << event->width() << "x" << event->height() << ")" << std::endl;
        this->update();
        // Est ce que les enfants on besoin d'etre au courant que le parent a changé de taille??
        // Peut être mettre un mecanisme pour s'abonner a ce genre que truc
        //for (Widget* child : findChildren<Widget>()) {
        //    child->resizeEvent(event);
        //}
    }


    // Gestion de l'événement de clic de souris
    virtual void mousePressEvent(MouseEvent* event) {
        // Trouver l'enfant le plus profond sous le pointeur de la souris
        Widget* targetWidget = getChildUnderCursor(event->x(), event->y());
        if (targetWidget && targetWidget->getFocusPolicy() != FocusPolicyEnum::NoFocus) {
            for (Widget* child : findChildren<Widget>()) {
                if (targetWidget != child) {
                    child->setFocus(false);
                }
            }
            // Si un enfant est trouvé sous le pointeur de la souris, on lui donne le focus
            if (targetWidget) {
                // Donner le focus au widget trouvé
                targetWidget->setFocus(true);
                targetWidget->mousePressEvent(event);
            }
        }
        for (Widget* child : findChildren<Widget>()) {
            if (targetWidget != child) {
                if (event->isAccepted()) {
                    return;
                }
                if (child->isPointInside(event->x(), event->y())) {
                    child->mousePressEvent(event);
                }
            }
        }
        event->accept();  // Marquer l'événement comme traité
    }


    // Gestion de l'événement de clic de souris
    virtual void mouseReleaseEvent(MouseEvent* event) {
        for (Widget* child : findChildren<Widget>()) {
            if (event->isAccepted()) {
                return;
            }
            child->mouseReleaseEvent(event);
        }
    }

    virtual void mouseDoubleClickEvent(MouseEvent* event) {
        std::cout << "Mouse double-clicked at (" << event->x() << ", " << event->y() << ") in MainWindow" << std::endl;
        for (Widget* child : findChildren<Widget>()) {
            if (event->isAccepted()) {
                return;
            }
            if (child->isPointInside(event->x(), event->y())) {
                child->mouseDoubleClickEvent(event);
            }
        }
    }

    // Gestion de l'événement de déplacement de la souris
    virtual void mouseMoveEvent(MouseEvent* event) {
        if (!getVisible()) {
            return;
        }

        
        //c'est l'enfant qui decide donc c'est lui qui parlera le derneir
        for (Widget* child : findChildren<Widget>()) {
            child->mouseMoveEvent(event);
        }

        this->setHover(isPointInside(event->x(), event->y()));

        if (!event->isAccepted() && this->getHover()) {
            SetCursor(winPtrCursor);
            event->accept();
        }
    }




protected:
    // Vérifier si un point (x, y) est à l'intérieur du widget
    bool isPointInside(int px, int py) const {
        return (px >= x && px <= x + width && py >= y && py <= y + height);
    }

    virtual void invalidateRect() {
        std::cout << "*************invalidateRect***********" << this->getObjectName() << std::endl;

        RECT rect;
        rect.left = x;
        rect.top = y;
        rect.right = x + width;
        rect.bottom = y + height;
        InvalidateRect(hwnd, &rect, TRUE);
    }


    HWND hwnd;
    int width, height;
    int x, y;
    SwStyle *m_style;

private:
    HCURSOR winPtrCursor;
    StyleSheet m_ComplexSheet;

};
