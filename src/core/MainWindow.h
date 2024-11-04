#pragma once

#include "Widget.h"
#include <windows.h>
#include <iostream>
#include <chrono>



class MainWindow : public Widget {
public:
    MainWindow(Widget* parent = nullptr) : Widget(parent) {
        // Enregistrer la classe de fenêtre
        const wchar_t CLASS_NAME[] = L"MainWindowClass";
		WNDCLASSW wc = {}; // Utiliser WNDCLASSW pour Unicode
		wc.lpfnWndProc = MainWindow::WindowProc;  // Fonction de gestion des événements de fenêtre
		wc.hInstance = GetModuleHandle(nullptr);
		wc.lpszClassName = CLASS_NAME;
		wc.style = CS_DBLCLKS;

		RegisterClassW(&wc); // Utiliser RegisterClassW pour Unicode

		// Créer la fenêtre
		hwnd = CreateWindowExW(
			0, CLASS_NAME, L"Main Window", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
			nullptr, nullptr, GetModuleHandle(nullptr), this);

		if (hwnd == nullptr) {
			std::cerr << "Erreur lors de la création de la fenêtre." << std::endl;
		}
    }

    // Afficher la fenêtre
    virtual void show() override{
        ShowWindow(hwnd, SW_SHOW);
        Widget::show();
    }

    // Cacher la fenêtre
    virtual void hide() override {
        ShowWindow(hwnd, SW_HIDE);  // Utiliser la fonction Windows pour cacher la fenêtre
        Widget::hide();  // Appeler la méthode hide du widget parent
    }


protected:
    // Gestionnaire d'événements pour redessiner la fenêtre (hérite de Widget::paintEvent)
    virtual void paintEvent(PaintEvent* event) override {
        

        // Appeler la logique de base du widget (pour redessiner les enfants, par exemple)
        Widget::paintEvent(event);
    }


    // Gestionnaire d'événements pour les clics de souris (hérite de Widget::mousePressEvent)
    virtual void mousePressEvent(MouseEvent* event) override {
        Widget::mousePressEvent(event);  // Appeler la logique de base du widget
    }

    // Gestionnaire d'événements pour le relâchement du clic de souris
    virtual void mouseReleaseEvent(MouseEvent* event) {
        Widget::mouseReleaseEvent(event);  // Propager l'événement aux enfants
    }

    // Gestionnaire d'événements pour les double-clics de souris
    virtual void mouseDoubleClickEvent(MouseEvent* event) {
        // Appeler la logique de base du widget si nécessaire
        Widget::mouseDoubleClickEvent(event);
    }

    // Redéfinir mouseMoveEvent pour gérer le déplacement de la souris
    virtual void mouseMoveEvent(MouseEvent* event) override {
        Widget::mouseMoveEvent(event);  // Propager aux enfants
    }



    // Gestionnaire d'événements pour les appuis de touches
    virtual void keyPressEvent(KeyEvent* event) override {
        // Appeler la méthode de la classe de base pour traiter l'événement
        Widget::keyPressEvent(event);
    }

private:
    POINT lastMousePosition; 
    std::chrono::steady_clock::time_point lastMoveTime;

    // Fonction statique de gestion des messages Windows
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        MainWindow* window = nullptr;

        if (uMsg == WM_NCCREATE) {
            window = reinterpret_cast<MainWindow*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        }
        else {
            window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (window) {
            switch (uMsg) {
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                // Activer le mode anti-aliasing
                Gdiplus::Graphics graphics(hdc);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

                PaintEvent paintEvent(hdc, ps.rcPaint);
                window->paintEvent(&paintEvent);  // Appeler l'événement paintEvent du widget

                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_LBUTTONDOWN: {
                MouseEvent mouseEvent(EventType::MousePress, LOWORD(lParam), HIWORD(lParam));
                window->mousePressEvent(&mouseEvent);  // Appeler l'événement mousePressEvent du widget
                return 0;
            }

            case WM_LBUTTONDBLCLK: {
                MouseEvent mouseEvent(EventType::MouseDoubleClick, LOWORD(lParam), HIWORD(lParam));
                window->mouseDoubleClickEvent(&mouseEvent);  // Appel de la nouvelle méthode mouseDoubleClickEvent
                return 0;
            }

            case WM_LBUTTONUP: {
                MouseEvent mouseEvent(EventType::MouseRelease, LOWORD(lParam), HIWORD(lParam));
                window->mouseReleaseEvent(&mouseEvent);  // Appeler l'événement mouseReleaseEvent du widget
                return 0;
            }

            case WM_MOUSEMOVE: {
                // Obtenir la position actuelle de la souris
                POINT currentMousePos;
                currentMousePos.x = LOWORD(lParam);
                currentMousePos.y = HIWORD(lParam);

                // Vérifier si la souris a bougé
                if (currentMousePos.x != window->lastMousePosition.x || currentMousePos.y != window->lastMousePosition.y) {
                    // Calculer le delta
                    int deltaX = currentMousePos.x - window->lastMousePosition.x;
                    int deltaY = currentMousePos.y - window->lastMousePosition.y;

                    // Calculer la vitesse
                    auto now = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - window->lastMoveTime).count();
                    double speedX = duration > 0 ? (static_cast<double>(deltaX) / duration) * 1000 : 0;
                    double speedY = duration > 0 ? (static_cast<double>(deltaY) / duration) * 1000 : 0;

                    // Mettre à jour la dernière position et le temps de déplacement
                    window->lastMousePosition = currentMousePos;
                    window->lastMoveTime = now;

                    // Créer l'événement de déplacement de la souris
                    MouseEvent mouseEvent(EventType::MousePress, currentMousePos.x, currentMousePos.y);
                    mouseEvent.setDeltaX(deltaX);
                    mouseEvent.setDeltaY(deltaY);
                    mouseEvent.setSpeedX(speedX);
                    mouseEvent.setSpeedY(speedY);

                    window->mouseMoveEvent(&mouseEvent);  // Propager l'événement
                }
                return 0;
            }

            case WM_SIZE: {
                // WM_SIZE est appelé quand la fenêtre est redimensionnée
                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                int width = clientRect.right - clientRect.left;
                int height = clientRect.bottom - clientRect.top;

                // Redimensionner le widget à la nouvelle taille de la fenêtre
                window->resize(width, height);

                return 0;
            }


            case WM_KEYDOWN: {
                bool ctrlPressed = GetKeyState(VK_CONTROL) & 0x8000;
                bool shiftPressed = GetKeyState(VK_SHIFT) & 0x8000;
                bool altPressed = GetKeyState(VK_MENU) & 0x8000;

                KeyEvent keyEvent(static_cast<int>(wParam), ctrlPressed, shiftPressed, altPressed);
                window->keyPressEvent(&keyEvent);
                break;
            }


            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            }
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};
