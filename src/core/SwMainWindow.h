// MainWindow.h
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

#include "SwGuiApplication.h"
#include "SwWidget.h"
#include <windows.h>
#include <iostream>
#include <chrono>

class SwMainWindow : public SwWidget {
public:
    /**
     * @brief Constructs the SwMainWindow and registers it with SwGuiApplication.
     * @param title Title of the window.
     * @param width Width of the window.
     * @param height Height of the window.
     */
    SwMainWindow(const std::wstring& title = L"Main Window", int width = 800, int height = 600)
        : SwWidget(nullptr), lastMousePosition{ 0, 0 }, lastMoveTime(std::chrono::steady_clock::now())
    {
        // Define the window class name
        const wchar_t CLASS_NAME[] = L"SwMainWindowClass";

        // Initialize WNDCLASSW structure
        WNDCLASSW wc = {};
        wc.lpfnWndProc = SwGuiApplication::WindowProc;  // Use SwGuiApplication's WindowProc
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = CLASS_NAME;
        wc.style = CS_DBLCLKS;

        // Attempt to register the window class
        if (!RegisterClassW(&wc)) {
            if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                std::wcerr << L"Failed to register window class. Error: " << GetLastError() << std::endl;
                return;
            }
        }

        // Create the window
        hwnd = CreateWindowExW(
            0,                              // Optional window styles.
            CLASS_NAME,                     // Window class
            title.c_str(),                 // Window text
            WS_OVERLAPPEDWINDOW,            // Window style

            // Size and position
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,

            nullptr,       // Parent window
            nullptr,       // Menu
            GetModuleHandle(nullptr), // Instance handle
            this           // Additional application data (pointer to SwMainWindow instance)
            );

        if (hwnd == nullptr) {
            std::wcerr << L"Failed to create window. Error: " << GetLastError() << std::endl;
            return;
        }

        // Define the callbacks for this window
        WindowCallbacks callbacks;
        callbacks.paintHandler = std::bind(&SwMainWindow::onPaint, this, std::placeholders::_1, std::placeholders::_2);
        callbacks.deleteHandler = std::bind(&SwMainWindow::onDelete, this);
        callbacks.mousePressHandler = std::bind(&SwMainWindow::onMousePress, this, std::placeholders::_1, std::placeholders::_2);
        callbacks.mouseReleaseHandler = std::bind(&SwMainWindow::onMouseRelease, this, std::placeholders::_1, std::placeholders::_2);
        callbacks.mouseDoubleClickHandler = std::bind(&SwMainWindow::onMouseDoubleClick, this, std::placeholders::_1, std::placeholders::_2);
        callbacks.mouseMoveHandler = std::bind(&SwMainWindow::onMouseMove, this, std::placeholders::_1, std::placeholders::_2);
        callbacks.keyPressHandler = std::bind(&SwMainWindow::onKeyPress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
        callbacks.resizeHandler = std::bind(&SwMainWindow::onResize, this, std::placeholders::_1, std::placeholders::_2);

        // Register this window with SwGuiApplication
        SwGuiApplication::registerWindow(hwnd, callbacks);
    }

    /**
     * @brief Destructor. Deregisters the window from SwGuiApplication.
     */
    ~SwMainWindow() override {
        if (hwnd) {
            SwGuiApplication::deregisterWindow(hwnd);
            DestroyWindow(hwnd);
            hwnd = nullptr;
        }
    }

    /**
     * @brief Shows the window.
     *
     * Displays the window.
     */
    void show() override {
        if (hwnd) {
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }
        SwWidget::show();
    }

    /**
     * @brief Hides the window.
     *
     * Hides the window from the screen.
     */
    void hide() override {
        if (hwnd) {
            ShowWindow(hwnd, SW_HIDE);
        }
        SwWidget::hide();
    }

    /**
     * @brief Minimizes the window.
     *
     * Reduces the window to the taskbar.
     */
    void showMinimized() {
        if (hwnd) {
            ShowWindow(hwnd, SW_MINIMIZE);
        }
    }

    /**
     * @brief Maximizes the window.
     *
     * Expands the window to fill the screen.
     */
    void showMaximized() {
        if (hwnd) {
            ShowWindow(hwnd, SW_MAXIMIZE);
        }
    }

    /**
     * @brief Restores the window.
     *
     * Restores the window to its original size and position.
     */
    void showNormal() {
        if (hwnd) {
            ShowWindow(hwnd, SW_RESTORE);
        }
    }

    /**
     * @brief Defines the states for the window.
     */
    enum class WindowState {
        Minimized,
        Maximized,
        Normal
    };

    /**
     * @brief Sets the window to the specified state.
     *
     * Adjusts the window's display state based on the provided value.
     *
     * @param state The desired window state.
     */
    void setWindowState(WindowState state) {
        switch (state) {
        case WindowState::Minimized:
            showMinimized();
            break;
        case WindowState::Maximized:
            showMaximized();
            break;
        case WindowState::Normal:
        default:
            showNormal();
            break;
        }
    }

    void setWindowFlags(WindowFlags flags) {
        if (!hwnd) return;

        // Récupération des styles actuels
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

        // Style de base : fenêtre classique
        style = WS_OVERLAPPEDWINDOW;

        // Gestion du style sans cadre (Frameless)
        if (flags.testFlag(WindowFlag::FramelessWindowHint)) {
            // Fenêtre sans bordure ni barre de titre
            style = WS_POPUP;
        } else {
            // Boutons de la barre de titre
            if (flags.testFlag(WindowFlag::NoMinimizeButton)) {
                style &= ~WS_MINIMIZEBOX;
            }
            if (flags.testFlag(WindowFlag::NoMaximizeButton)) {
                style &= ~WS_MAXIMIZEBOX;
            }
        }

        // Fenêtre outil (ToolWindowHint)
        if (flags.testFlag(WindowFlag::ToolWindowHint)) {
            exStyle |= WS_EX_TOOLWINDOW;
            exStyle &= ~WS_EX_APPWINDOW; // pour éviter l'affichage dans la barre des tâches
        }

        // Toujours au-dessus (StayOnTopHint)
        if (flags.testFlag(WindowFlag::StayOnTopHint)) {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        } else {
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }

        // Application du style
        SetWindowLongPtr(hwnd, GWL_STYLE, style);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

        // Gestion du bouton de fermeture (NoCloseButton)
        HMENU hMenu = GetSystemMenu(hwnd, FALSE);
        if (flags.testFlag(WindowFlag::NoCloseButton)) {
            if (hMenu) {
                RemoveMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
            }
        } else {
            // Réinsertion du bouton "Fermer" s'il avait été retiré
            if (hMenu && GetMenuState(hMenu, SC_CLOSE, MF_BYCOMMAND) == (UINT)-1) {
                AppendMenuW(hMenu, MF_STRING, SC_CLOSE, L"Close");
            }
        }

        // Forcer la prise en compte des changements
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        InvalidateRect(hwnd, nullptr, TRUE);
    }

    WindowFlags getWindowFlags() const {
        WindowFlags flags = WindowFlag::NoFlag;
        if (!hwnd) {
            return flags;
        }

        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

        // Déterminer si la fenêtre est frameless
        // On considère qu'elle est frameless si style == WS_POPUP et pas WS_OVERLAPPEDWINDOW
        // (Ajuster selon votre logique interne si nécessaire)
        if ((style & WS_POPUP) == WS_POPUP && (style & WS_OVERLAPPEDWINDOW) != WS_OVERLAPPEDWINDOW) {
            flags |= WindowFlag::FramelessWindowHint;
        } else {
            // Vérifier les boutons minimize/maximize
            if ((style & WS_MINIMIZEBOX) == 0) {
                flags |= WindowFlag::NoMinimizeButton;
            }
            if ((style & WS_MAXIMIZEBOX) == 0) {
                flags |= WindowFlag::NoMaximizeButton;
            }
        }

        // Vérifier ToolWindowHint
        // Si WS_EX_TOOLWINDOW est présent, la fenêtre est probablement un tool window
        if ((exStyle & WS_EX_TOOLWINDOW) == WS_EX_TOOLWINDOW) {
            flags |= WindowFlag::ToolWindowHint;
        }

        // Vérifier StayOnTop
        // Si WS_EX_TOPMOST est présent, alors la fenêtre est au-dessus
        if ((exStyle & WS_EX_TOPMOST) == WS_EX_TOPMOST) {
            flags |= WindowFlag::StayOnTopHint;
        }

        // Vérifier NoCloseButton
        // On regarde dans le menu système si le close est présent
        HMENU hMenu = GetSystemMenu(hwnd, FALSE);
        if (hMenu) {
            UINT state = GetMenuState(hMenu, SC_CLOSE, MF_BYCOMMAND);
            if (state == (UINT)-1) {
                // Le bouton close a été supprimé
                flags |= WindowFlag::NoCloseButton;
            }
        }

        return flags;
    }

private:
    POINT lastMousePosition; ///< Last recorded mouse position.
    std::chrono::steady_clock::time_point lastMoveTime; ///< Last recorded time of mouse movement.

    // Event Handlers

    /**
     * @brief Handles paint events.
     * @param hdc Handle to device context.
     * @param rect Rectangle to paint.
     */
    void onPaint(HDC hdc, const RECT& rect) {
        PaintEvent paintEvent(hdc, rect);
        this->paintEvent(&paintEvent);
    }

    /**
     * @brief Handles window deletion.
     */
    void onDelete() {
        std::wcout << L"MainWindow is being deleted." << std::endl;
    }

    /**
     * @brief Handles resize events.
     *
     * Adjusts the size of the widget based on the given width and height dimensions.
     *
     * @param width The new width of the widget.
     * @param height The new height of the widget.
     */
    void onResize(int width, int height) {
        SwWidget::resize(width, height);
    }


    /**
     * @brief Handles mouse press events.
     * @param x X-coordinate of the mouse.
     * @param y Y-coordinate of the mouse.
     */
    void onMousePress(int x, int y) {
        MouseEvent mouseEvent(EventType::MousePress, x, y);
        SwWidget::mousePressEvent(&mouseEvent);
    }

    /**
     * @brief Handles mouse release events.
     * @param x X-coordinate of the mouse.
     * @param y Y-coordinate of the mouse.
     */
    void onMouseRelease(int x, int y) {
        MouseEvent mouseEvent(EventType::MouseRelease, x, y);
        SwWidget::mouseReleaseEvent(&mouseEvent);
    }

    /**
     * @brief Handles mouse double-click events.
     * @param x X-coordinate of the mouse.
     * @param y Y-coordinate of the mouse.
     */
    void onMouseDoubleClick(int x, int y) {
        MouseEvent mouseEvent(EventType::MouseDoubleClick, x, y);
        SwWidget::mouseDoubleClickEvent(&mouseEvent);
    }

    /**
     * @brief Handles mouse move events.
     * @param x X-coordinate of the mouse.
     * @param y Y-coordinate of the mouse.
     */
    void onMouseMove(int x, int y) {
        // Calculate delta and speed
        MouseEvent mouseEvent(EventType::MousePress, x, y);
        if (lastMoveTime.time_since_epoch().count() != 0) { // Check if lastMoveTime is initialized
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMoveTime).count();
            int deltaX = x - lastMousePosition.x;
            int deltaY = y - lastMousePosition.y;
            double speedX = duration > 0 ? (static_cast<double>(deltaX) / duration) * 1000 : 0;
            double speedY = duration > 0 ? (static_cast<double>(deltaY) / duration) * 1000 : 0;


            mouseEvent.setDeltaX(deltaX);
            mouseEvent.setDeltaY(deltaY);
            mouseEvent.setSpeedX(speedX);
            mouseEvent.setSpeedY(speedY);

            // Update lastMoveTime
            lastMoveTime = now;
        } else {
            // Initialize lastMoveTime
            lastMoveTime = std::chrono::steady_clock::now();
        }

        // Update lastMousePosition
        lastMousePosition.x = x;
        lastMousePosition.y = y;

        // Optionally, call the base class mouseMoveEvent if necessary
        SwWidget::mouseMoveEvent(&mouseEvent);
    }

    /**
     * @brief Handles key press events.
     * @param keyCode Virtual key code.
     * @param ctrlPressed Whether Ctrl is pressed.
     * @param shiftPressed Whether Shift is pressed.
     * @param altPressed Whether Alt is pressed.
     */
    void onKeyPress(int keyCode, bool ctrlPressed, bool shiftPressed, bool altPressed) {
        KeyEvent keyEvent(keyCode, ctrlPressed, shiftPressed, altPressed);
        SwWidget::keyPressEvent(&keyEvent);
    }
};


