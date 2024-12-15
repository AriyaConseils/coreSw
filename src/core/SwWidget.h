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

#include "SwWidgetInterface.h"
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



// Classe SwWidget héritant de SwObject, simulant QWidget
class SwWidget : public SwWidgetInterface {

    /**
     * @brief Macro to declare the inheritance hierarchy for the SwWidget class.
     *
     * Specifies that `SwWidget` is derived from `SwObject` class.
     */
    SW_OBJECT(SwWidget, SwObject)

    /**
     * @brief Property for the SwWidget's focus policy.
     *
     * Determines how the SwWidget handles focus events.
     *
     * @param FocusPolicy The focus policy as a `FocusPolicyEnum` value. Default is `FocusPolicyEnum::Accept`.
     */
    PROPERTY(FocusPolicyEnum, FocusPolicy, FocusPolicyEnum::Accept)

    /**
     * @brief Property for the SwWidget's tooltip text.
     *
     * Sets or retrieves the tooltip text displayed when the user hovers over the SwWidget.
     *
     * @param ToolTips The tooltip text as an `SwString`.
     */
    PROPERTY(SwString, ToolTips, "")

    /**
     * @brief Custom property for the SwWidget's enabled state.
     *
     * Sets the enabled state of the SwWidget and triggers an update to apply any visual changes.
     *
     * @param Enable The new enabled state (`true` if the SwWidget is enabled, `false` otherwise).
     */
    CUSTOM_PROPERTY(bool, Enable, true) {
        update();
    }

    /**
     * @brief Custom property for the SwWidget's focus state.
     *
     * Sets the focus state of the SwWidget and triggers an update to apply any visual changes.
     *
     * @param Focus The new focus state (`true` if the SwWidget is focused, `false` otherwise).
     */
    CUSTOM_PROPERTY(bool, Focus, false) {
        update();
    }

    /**
     * @brief Custom property for the SwWidget's hover state.
     *
     * Sets the hover state of the SwWidget and triggers an update to apply any visual changes.
     *
     * @param Hover The new hover state (`true` if the SwWidget is hovered, `false` otherwise).
     */
    CUSTOM_PROPERTY(bool, Hover, false) {
        update();
    }

    /**
     * @brief Custom property for the SwWidget's visibility.
     *
     * Sets the visibility state of the SwWidget and invalidates its rectangular area to trigger a redraw.
     *
     * @param Visible The new visibility state (`true` for visible, `false` for hidden).
     */
    CUSTOM_PROPERTY(bool, Visible, true) {
        invalidateRect();
    }

    /**
     * @brief Custom property for the SwWidget's cursor type.
     *
     * Sets the cursor type for the SwWidget and loads the corresponding Windows cursor resource.
     * If the cursor type is not recognized, no default cursor is assigned.
     *
     * @param Cursor The new cursor type as a `CursorType` enumeration value.
     */
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

    /**
     * @brief Custom override property for the SwWidget's font.
     *
     * Sets the SwWidget's font and triggers an update to apply the changes.
     *
     * @param Font The new font as an `SwFont` object.
     */
    CUSTOM_OVERRIDE_PROPERTY(SwFont, Font, SwFont()) {
        update();
    }

    /**
     * @brief Custom property for the SwWidget's style sheet.
     *
     * Sets the SwWidget's style sheet and triggers its parsing.
     * After parsing, the SwWidget is updated to apply the new style.
     *
     * @param StyleSheet The new style sheet as an `SwString`.
     */
    CUSTOM_PROPERTY(SwString, StyleSheet, "") {
        m_ComplexSheet.parseStyleSheet(m_StyleSheet);
        update();
    }

public:

    /**
     * @brief Constructor for the SwWidget class.
     *
     * Initializes a SwWidget with default dimensions, registered properties, and a default style.
     * If a parent is specified, the SwWidget will be linked to it, and a new parent event will be triggered.
     *
     * @param parent Pointer to the parent SwWidget. If nullptr, the SwWidget is standalone.
     */
    SwWidget(SwWidget* parent = nullptr)
        : SwWidgetInterface(parent), width(100), height(100), x(0), y(0), m_style(new SwStyle()) {

        setCursor(CursorType::Arrow);
        if (SwObject::parent()) {
            this->newParentEvent(SwObject::parent());
        }
    }

    /**
     * @brief Destructor for the SwWidget class.
     *
     * Cleans up the SwWidget by deleting all its child SwWidgets to ensure proper memory management.
     */
    virtual ~SwWidget() {
        for (auto child : findChildren<SwWidget>()) {
            delete child;
        }
    }

    /**
     * @brief Adds a child SwWidget to the current SwWidget.
     *
     * Links the specified SwWidget as a child of this SwWidget using the base `SwObject` implementation.
     *
     * @param child Pointer to the child SwWidget to be added.
     */
    virtual void addChild(SwObject* child) override {
        SwObject::addChild(child);
    }

    virtual void removeChild(SwObject* child) override {
        SwObject::addChild(child);
    }

    /**
     * @brief Displays the SwWidget by setting its visibility to true.
     *
     * Overrides the base implementation to make the SwWidget visible.
     */
    virtual void show() override {
        setVisible(true);
    }

    /**
     * @brief Hides the SwWidget by setting its visibility to false.
     *
     * Overrides the base implementation to make the SwWidget invisible.
     * Outputs a message to the console indicating that the SwWidget has been hidden.
     */
    virtual void hide() override {
        setVisible(false);
    }

    /**
     * @brief Redraws the SwWidget and propagates the update event to its children.
     *
     * If the SwWidget is visible, it invalidates its rectangle to trigger a redraw and
     * recursively calls the `update` method on all child SwWidgets.
     */
    virtual void update() override {
        if (!getVisible()) {
            return;
        }
        invalidateRect();

        // Propagation de l'événement de dessin aux enfants
        for (SwWidget* child : findChildren<SwWidget>()) {
            child->update();
        }
    }

    /**
     * @brief Moves the SwWidget to a new position and propagates the move event to its children.
     *
     * Updates the SwWidget's position, emits the `moved` signal, and triggers a redraw if the SwWidget is visible.
     * The movement is also propagated to all child SwWidgets relative to the new position.
     *
     * @param newX The new X-coordinate of the SwWidget.
     * @param newY The new Y-coordinate of the SwWidget.
     */
    virtual void move(int newX, int newY) override {
        x = newX;
        y = newY;
        emit moved(x, y);
        if (getVisible()) {
            update();
        }
        // Propagation de l'événement de déplacement aux enfants
        for (SwWidget* child : findChildren<SwWidget>()) {
            child->move(newX, newY);  // Déplace les enfants relativement au parent
        }
    }

    /**
     * @brief Resizes the SwWidget and triggers the resize event.
     *
     * Updates the SwWidget's width and height, emits the `resized` signal,
     * and calls the `resizeEvent` to handle the resize logic.
     *
     * @param newWidth The new width of the SwWidget.
     * @param newHeight The new height of the SwWidget.
     */
    virtual void resize(int newWidth, int newHeight) override {
        width = newWidth;
        height = newHeight;
        ResizeEvent event(width, height);
        resizeEvent(&event);
        emit resized(width, height);
    }

    /**
     * @brief Retrieves the current rectangle representing the SwWidget's position and size.
     *
     * Computes and returns a `RECT` structure with the SwWidget's coordinates and dimensions.
     *
     * @return A `RECT` structure containing the SwWidget's position (left, top) and size (right, bottom).
     */
    RECT getRect() const override {
        RECT rectVal;
        rectVal.left = x;
        rectVal.top = y;
        rectVal.right = x + width;
        rectVal.bottom = y + height;
        return rectVal;
    }

    /**
     * @brief Retrieves the deepest child SwWidget located under the given cursor position.
     *
     * Iterates through the SwWidget's children to determine which one contains the specified coordinates.
     * If multiple nested children match, the method returns the deepest child.
     *
     * @param x The X-coordinate of the cursor.
     * @param y The Y-coordinate of the cursor.
     * @return A pointer to the deepest child SwWidget under the cursor, or nullptr if none is found.
     */
    SwWidget* getChildUnderCursor(int x, int y) {
        SwWidget* deepestWidget = nullptr;

        // Parcourir tous les enfants
        for (SwWidget* child : findChildren<SwWidget>()) {
            // Vérifier si le pointeur est à l'intérieur de l'enfant
            if (child->isPointInside(x, y)) {
                // Appeler récursivement getChildUnderCursor pour trouver l'enfant le plus profond
                SwWidget* deepChild = child->getChildUnderCursor(x, y);
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

    /**
     * @brief Retrieves the stylesheet associated with the SwWidget.
     *
     * Returns a pointer to the internal `StyleSheet` object used by the SwWidget.
     *
     * @return A pointer to the `StyleSheet` object.
     */
    StyleSheet* getToolSheet() override {
        return &m_ComplexSheet;
    }

signals:
    /**
     * @brief Signal emitted when the SwWidget is resized.
     *
     * Parameters:
     * - `int newWidth`: The new width of the SwWidget.
     * - `int newHeight`: The new height of the SwWidget.
     */
    DECLARE_SIGNAL(resized, int, int);

    /**
     * @brief Signal emitted when the SwWidget is moved.
     *
     * Parameters:
     * - `int newX`: The new X-coordinate of the SwWidget.
     * - `int newY`: The new Y-coordinate of the SwWidget.
     */
    DECLARE_SIGNAL(moved);

    /**
     * @brief Signal emitted when the visibility of the SwWidget changes.
     *
     * Parameters:
     * - `bool isVisible`: The new visibility state of the SwWidget.
     */
    DECLARE_SIGNAL(visibilityChanged);


protected:

    /**
     * @brief Handles the event when the SwWidget gets a new parent.
     *
     * Updates the SwWidget's window handle (`hwnd`) and visibility state based on the new parent.
     * Calls the base `SwObject::newParentEvent` to ensure proper propagation.
     *
     * @param parent Pointer to the new parent object.
     */
    virtual void newParentEvent(SwObject* parent) override {
        SwWidget *ui = dynamic_cast<SwWidget*>(parent);
        hwnd = ui->hwnd;
        this->setVisible(ui->getVisible());
        SwObject::newParentEvent(parent);
    }

    /**
     * @brief Handles the paint event for the SwWidget.
     *
     * Draws the SwWidget's background using a specified color and propagates the paint event
     * to visible child SwWidgets whose rectangles intersect the paint area.
     *
     * @param event Pointer to the `PaintEvent` containing the context and paint area.
     */
    virtual void paintEvent(PaintEvent* event) override {
        if (!getVisible()) {
            return;
        }

        SwPainter painter(event->context());
        RECT rect = getRect();
        COLORREF bgColor = RGB(100, 149, 237); 
        this->m_style->drawBackground(&rect, &painter, bgColor);


        RECT paintRect = event->getPaintRect();
        for (SwWidget* child : findChildren<SwWidget>()) {
            RECT childRect = child->getRect();

            // Vérifier si l'enfant est visible et si son rectangle intersecte la zone de peinture
            if (child->getVisible() && rectsIntersect(paintRect, childRect)) {
                child->paintEvent(event);
            }
        }
    }

    /**
     * @brief Utility function to check if two Windows RECT structures intersect.
     *
     * Determines whether the rectangles `r1` and `r2` overlap by evaluating their boundaries.
     *
     * @param r1 The first RECT structure.
     * @param r2 The second RECT structure.
     * @return `true` if the rectangles intersect, `false` otherwise.
     */
    bool rectsIntersect(const RECT& r1, const RECT& r2) {
        return !(r1.right < r2.left ||   // r1 est complètement à gauche de r2
            r1.left > r2.right ||   // r1 est complètement à droite de r2
            r1.bottom < r2.top ||   // r1 est complètement au-dessus de r2
            r1.top > r2.bottom);    // r1 est complètement en dessous de r2
    }

    /**
     * @brief Handles the key press event for the SwWidget.
     *
     * Processes the key press event and propagates it to the child SwWidgets.
     * If the event is marked as accepted by any child, further propagation stops.
     *
     * @param event Pointer to the `KeyEvent` containing information about the key press.
     */
    virtual void keyPressEvent(KeyEvent* event) override {

        for (SwWidget* child : findChildren<SwWidget>()) {
            if (event->isAccepted()) {
                return;
            }
            child->keyPressEvent(event);
        }
    }

    /**
     * @brief Handles the resize event for the SwWidget.
     *
     * Triggers an update to redraw the SwWidget after a resize.
     * Optionally, a mechanism could be added to notify child SwWidgets of the parent's size change.
     *
     * @param event Pointer to the `ResizeEvent` containing the new dimensions of the SwWidget.
     */
    virtual void resizeEvent(ResizeEvent* event) {
        this->update();
    }

    /**
     * @brief Handles the mouse press event for the SwWidget.
     *
     * Determines the deepest child SwWidget under the cursor and propagates the event to it.
     * If the target SwWidget has a focus policy, it is given focus, and other child SwWidgets lose focus.
     * If the event is marked as accepted by any child, further propagation stops.
     *
     * @param event Pointer to the `MouseEvent` containing the mouse press details.
     */
    virtual void mousePressEvent(MouseEvent* event) override {
        // Trouver l'enfant le plus profond sous le pointeur de la souris
        SwWidget* targetWidget = getChildUnderCursor(event->x(), event->y());
        if (targetWidget && targetWidget->getFocusPolicy() != FocusPolicyEnum::NoFocus) {
            for (SwWidget* child : findChildren<SwWidget>()) {
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
        for (SwWidget* child : findChildren<SwWidget>()) {
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

    /**
     * @brief Handles the mouse release event for the SwWidget.
     *
     * Propagates the mouse release event to all child SwWidgets.
     * If the event is marked as accepted by any child, further propagation stops.
     *
     * @param event Pointer to the `MouseEvent` containing the mouse release details.
     */
    virtual void mouseReleaseEvent(MouseEvent* event) override {
        for (SwWidget* child : findChildren<SwWidget>()) {
            if (event->isAccepted()) {
                return;
            }
            child->mouseReleaseEvent(event);
        }
    }

    /**
     * @brief Handles the mouse double-click event for the SwWidget.
     *
     * Propagates the mouse double-click event to all child SwWidgets that contain the cursor position.
     * If the event is marked as accepted by any child, further propagation stops.
     *
     * @param event Pointer to the `MouseEvent` containing the double-click details.
     */
    virtual void mouseDoubleClickEvent(MouseEvent* event) override {
        for (SwWidget* child : findChildren<SwWidget>()) {
            if (event->isAccepted()) {
                return;
            }
            if (child->isPointInside(event->x(), event->y())) {
                child->mouseDoubleClickEvent(event);
            }
        }
    }

    /**
     * @brief Handles the mouse move event for the SwWidget.
     *
     * Propagates the mouse move event to all child SwWidgets.
     * Updates the hover state of the current SwWidget based on the cursor position.
     * If the event is not accepted and the SwWidget is hovered, it sets the cursor and marks the event as accepted.
     *
     * @param event Pointer to the `MouseEvent` containing the mouse move details.
     */
    virtual void mouseMoveEvent(MouseEvent* event) override {
        if (!getVisible()) {
            return;
        }

        
        //c'est l'enfant qui decide donc c'est lui qui parlera le derneir
        for (SwWidget* child : findChildren<SwWidget>()) {
            child->mouseMoveEvent(event);
        }

        this->setHover(isPointInside(event->x(), event->y()));

        if (!event->isAccepted() && this->getHover()) {
            SetCursor(winPtrCursor);
            event->accept();
        }
    }

protected:

    /**
     * @brief Checks if a given point is inside the SwWidget's boundaries.
     *
     * Determines whether the point specified by its X and Y coordinates lies within the SwWidget's rectangle.
     *
     * @param px The X-coordinate of the point.
     * @param py The Y-coordinate of the point.
     * @return `true` if the point is inside the SwWidget, `false` otherwise.
     */
    bool isPointInside(int px, int py) const {
        return (px >= x && px <= x + width && py >= y && py <= y + height);
    }

    /**
     * @brief Invalidates the SwWidget's rectangular area, marking it for redrawing.
     *
     * Computes the SwWidget's rectangle based on its position and size, and requests a redraw
     * by calling the `InvalidateRect` function on the associated window handle (`hwnd`).
     */
    virtual void invalidateRect() {
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
