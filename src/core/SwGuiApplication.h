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

#include "SwCoreApplication.h"
#include <windows.h>
#include <gdiplus.h>
#include <functional>

class SwVirtualGraphicsEngine {
public:
    virtual ~SwVirtualGraphicsEngine() = default;

    // Initialisation spécifique à l'implémentation
    virtual void initialize() = 0;

    // Nettoyage spécifique à l'implémentation
    virtual void shutdown() = 0;

    // Exemple de méthode de rendu générique
    virtual void render() = 0;
};


class SwGdiPlusEngine : public SwVirtualGraphicsEngine {
public:
    // Singleton pour garantir une seule instance
    static SwGdiPlusEngine& instance() {
        static SwGdiPlusEngine instance;
        return instance;
    }

    // Initialisation
    void initialize() override {
        if (initialized) {
            return; // Déjà initialisé
        }

        // Charger dynamiquement la bibliothèque gdiplus.dll
        gdiplusLib = LoadLibrary(TEXT("gdiplus.dll"));
        if (!gdiplusLib) {
            throw std::runtime_error("Failed to load gdiplus.dll");
        }

        // Charger les fonctions nécessaires
        auto startupProc = reinterpret_cast<decltype(&Gdiplus::GdiplusStartup)>(
            GetProcAddress(gdiplusLib, "GdiplusStartup"));
        auto shutdownProc = reinterpret_cast<decltype(&Gdiplus::GdiplusShutdown)>(
            GetProcAddress(gdiplusLib, "GdiplusShutdown"));

        if (!startupProc || !shutdownProc) {
            FreeLibrary(gdiplusLib);
            gdiplusLib = nullptr;
            throw std::runtime_error("Failed to retrieve GDI+ functions from gdiplus.dll");
        }

        gdiplusStartup = startupProc;
        gdiplusShutdown = shutdownProc;

        // Initialiser GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput = {};
        gdiplusStartupInput.GdiplusVersion = 1; // Spécifier la version de GDI+

        Gdiplus::Status status = gdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
        if (status != Gdiplus::Ok) {
            FreeLibrary(gdiplusLib);
            gdiplusLib = nullptr;
            throw std::runtime_error("Failed to initialize GDI+");
        }

        initialized = true;
    }

    // Arrêt
    void shutdown() override {
        if (!initialized) {
            return; // Rien à faire si non initialisé
        }

        try {
            if (gdiplusShutdown) {
                gdiplusShutdown(gdiplusToken); // Fermer GDI+
            }
        } catch (...) {
            // Gestion des exceptions dans le shutdown pour éviter des crashs
        }

        if (gdiplusLib) {
            FreeLibrary(gdiplusLib); // Libérer la bibliothèque
            gdiplusLib = nullptr;
        }

        initialized = false;
    }

    // Exemple de rendu (vide pour l'instant)
    void render() override {
        // Rendu spécifique à GDI+ (ajouter le code ici si nécessaire)
    }

private:
    // Constructeur privé pour le singleton
    SwGdiPlusEngine()
        : initialized(false), gdiplusLib(nullptr), gdiplusToken(0),
        gdiplusStartup(nullptr), gdiplusShutdown(nullptr) {}

    // Destructeur pour libérer les ressources
    ~SwGdiPlusEngine() {
        if (initialized) {
            shutdown();
        }
    }

    // Interdire la copie et l'affectation
    SwGdiPlusEngine(const SwGdiPlusEngine&) = delete;
    SwGdiPlusEngine& operator=(const SwGdiPlusEngine&) = delete;

    // Attributs pour la liaison dynamique
    bool initialized;
    HMODULE gdiplusLib;
    ULONG_PTR gdiplusToken;

    // Pointeurs de fonction GDI+
    decltype(&Gdiplus::GdiplusStartup) gdiplusStartup;
    decltype(&Gdiplus::GdiplusShutdown) gdiplusShutdown;
};



/**
 * @brief Structure encapsulating all event callbacks for a window.
 */
struct WindowCallbacks {
    // Paint event: Provides HDC and RECT for painting
    std::function<void(HDC, const RECT&)> paintHandler;

    // Delete event: Called when the window is being destroyed
    std::function<void()> deleteHandler;

    // Mouse press event: Provides x and y coordinates
    std::function<void(int, int)> mousePressHandler;

    // Mouse release event: Provides x and y coordinates
    std::function<void(int, int)> mouseReleaseHandler;

    // Mouse double-click event: Provides x and y coordinates
    std::function<void(int, int)> mouseDoubleClickHandler;

    // Mouse move event: Provides x and y coordinates
    std::function<void(int, int)> mouseMoveHandler;

    // Key press event: Provides virtual key code and modifier states
    std::function<void(int, bool, bool, bool)> keyPressHandler;

    std::function<void(int width, int height)> resizeHandler;
};

/**
 * @class SwGuiApplication
 * @brief A GUI event-driven application class that integrates Windows message processing into the SwCoreApplication fiber system.
 *
 * This class extends SwCoreApplication to support traditional Windows GUI events, enabling a hybrid approach:
 * - Standard Win32 message loop for GUI events (WM_xxx messages).
 * - Fiber-based event handling (from SwCoreApplication) for asynchronous callbacks, timers, and posted events.
 *
 * When `exec()` is called, the application enters a loop that:
 * 1. Fetches and processes all pending Windows messages.
 * 2. Dispatches them through TranslateMessage and DispatchMessage, each within a separate fiber for isolation and control.
 * 3. Processes application-level events and timers via SwCoreApplication's fiber system (`processEvent()`).
 *
 * GDI+ initialization and shutdown are handled automatically upon construction and destruction, ensuring the
 * GUI subsystem is properly configured throughout the application's lifetime.
 *
 * ### Example Usage
 *
 * ```cpp
 * int main(int argc, char *argv[]) {
 *     SwGuiApplication app(argc, argv);
 *     // Set up windows, widgets, etc.
 *
 *     return app.exec();
 * }
 * ```
 *
 * ### Key Behavior
 * - `exec()` runs the main event loop:
 *   - Retrieves Windows messages using `PeekMessage`.
 *   - For each message, a fiber is created to run `TranslateMessage()` and `DispatchMessage()` safely.
 *   - Calls `processEvent()` to handle custom events and timers posted to the application.
 * - The loop continues until `WM_QUIT` is received or `SwCoreApplication::quit()`/`exit()` is called, at which point `exec()` returns.
 *
 * ### GDI+ Integration
 * - GDI+ is initialized in the constructor and shut down in the destructor.
 * - Ensures a consistent graphical environment for drawing operations.
 *
 * @note Ensure that the thread is converted to a fiber and that the SwCoreApplication is properly initialized
 * before entering `exec()`.
 *
 * @see SwCoreApplication for details on event processing and fibers.
 * @see postEvent()
 *
 * @author
 * Eymeric O'Neill
 * Contact: eymeric.oneill@gmail.com, +33 6 52 83 83 31
 *
 * @since 1.0
 */
/**
 * @class SwGuiApplication
 * @brief GUI application class that manages window registrations and event dispatching.
 *
 * This class extends SwCoreApplication to handle Windows GUI events by maintaining a registry
 * of window handles and their corresponding event callbacks.
 */
#include <windows.h>
#include <functional>
#include <map>
#include <mutex>

/**
 * @class SwGuiApplication
 * @brief GUI application class that manages window registrations and event dispatching.
 *
 * This class extends SwCoreApplication to handle Windows GUI events by maintaining a registry
 * of window handles and their corresponding event callbacks.
 *
 * It ensures that all event dispatching is done through fibers, maintaining a decoupled and
 * thread-safe architecture.
 */
class SwGuiApplication : public SwCoreApplication {
    friend class SwMainWindow;
public:

    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    SwGuiApplication() : SwCoreApplication() {
        instance(false) = this;
    }

    /**
     * @brief Destructor.
     */
    ~SwGuiApplication() override {
        // Properly shutdown resources if necessary
    }


    /**
     * @brief Retrieves the singleton instance of SwGuiApplication.
     * @return Reference to the singleton instance.
     */
    static SwGuiApplication*& instance(bool create = true) {
        static SwGuiApplication* _mainApp = nullptr;
        if(!_mainApp && create)
            _mainApp = new SwGuiApplication();
        return _mainApp;
    }

    /**
     * @brief Registers a window with the application.
     *
     * @param hwnd Handle to the window.
     * @param callbacks Struct containing event handlers for the window.
     */
    static void registerWindow(HWND hwnd, const WindowCallbacks& callbacks) {
        std::lock_guard<std::mutex> lock(s_windowMutex);
        s_windows[hwnd] = callbacks;
    }

    /**
     * @brief Deregisters a window from the application.
     *
     * @param hwnd Handle to the window.
     */
    static void deregisterWindow(HWND hwnd) {
        std::lock_guard<std::mutex> lock(s_windowMutex);
        auto it = s_windows.find(hwnd);
        if (it != s_windows.end()) {
            // Optionally invoke the delete handler before removal
            if (it->second.deleteHandler) {
                it->second.deleteHandler();
            }
            s_windows.erase(it);
        }
    }

    /**
     * @brief Runs the main GUI event loop.
     *
     * This loop:
     * - Retrieves and processes Windows messages from the queue.
     * - For each message, creates a fiber to safely call TranslateMessage and DispatchMessage.
     * - Calls processEvent from SwCoreApplication to handle application-level events and timers.
     * - Sleeps briefly to reduce CPU usage between iterations.
     *
     * The loop continues until a WM_QUIT message is received or quit()/exit() is called, at which point it returns.
     *
     * @return The exit code set by exit() or 0 if quit() was called.
     */
    int exec(int maxDurationMicroseconds = 0) override {
        setHighThreadPriority();
        MSG msg = {};

        auto guiTask = [&](){
            // Process all available Windows messages
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    SwCoreApplication::exit(static_cast<int>(msg.wParam));
                    return exitCode;
                }

                // // Dispatch the message to the window's WindowProc
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        };
        SwCoreApplication::instance()->addTimer(guiTask, 10, false);
        SwCoreApplication::exec(maxDurationMicroseconds);

        // Cleanup all registered windows
        unregisterAllWindows();

        return exitCode;
    }

private:
    /**
     * @brief Static WindowProc to handle Windows messages.
     *
     * Dispatches messages to the appropriate window callbacks based on HWND.
     *
     * @param hwnd Handle to the window.
     * @param uMsg Message identifier.
     * @param wParam Additional message information.
     * @param lParam Additional message information.
     * @return Result of message processing.
     */
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        WindowCallbacks callbacks;

        {
            std::lock_guard<std::mutex> lock(s_windowMutex);
            auto it = s_windows.find(hwnd);
            if (it != s_windows.end()) {
                callbacks = it->second;
            } else {
                // No callbacks registered for this window; use default processing
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
        }

        switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Dispatch the paint event via fiber
            if (callbacks.paintHandler) {
                runPaintHandler(callbacks.paintHandler, hdc, ps.rcPaint);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            if (callbacks.resizeHandler) {
                runResizeHandler(callbacks.resizeHandler, width, height);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if (callbacks.mousePressHandler) {
                runMousePressHandler(callbacks.mousePressHandler, x, y);
            }
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if (callbacks.mouseDoubleClickHandler) {
                runMouseDoubleClickHandler(callbacks.mouseDoubleClickHandler, x, y);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if (callbacks.mouseReleaseHandler) {
                runMouseReleaseHandler(callbacks.mouseReleaseHandler, x, y);
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if (callbacks.mouseMoveHandler) {
                runMouseMoveHandler(callbacks.mouseMoveHandler, x, y);
            }
            return 0;
        }

        case WM_KEYDOWN: {
            int keyCode = static_cast<int>(wParam);
            bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            bool altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
            if (callbacks.keyPressHandler) {
                runKeyPressHandler(callbacks.keyPressHandler, keyCode, ctrlPressed, shiftPressed, altPressed);
            }
            return 0;
        }

        case WM_DESTROY: {
            if (callbacks.deleteHandler) {
                runDeleteHandler(callbacks.deleteHandler);
            }
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    /**
    /**
     * @brief Runs a paint handler within the current execution context.
     *
     * This function invokes the specified paint handler, passing in the device context handle and the rectangle to paint.
     *
     * @param handler Callback function to handle painting.
     * @param hdc Handle to the device context (HDC).
     * @param rect The rectangle area to be painted.
     */
    static void runPaintHandler(const std::function<void(HDC, const RECT&)>& handler, HDC hdc, const RECT& rect) {
        handler(hdc, rect);
    }

    /**
     * @brief Runs a resize handler within the current execution context.
     *
     * This function invokes the specified resize handler, passing in the width and height dimensions.
     *
     * @param handler Callback function to handle resizing.
     * @param width New width of the window or component.
     * @param height New height of the window or component.
     */
    static void runResizeHandler(const std::function<void(int, int)>& handler, int width, int height) {
        handler(width, height);
    }

    /**
     * @brief Runs a mouse press handler asynchronously.
     *
     * Posts an event to run the specified mouse press handler with the given coordinates.
     *
     * @param handler Callback function to handle mouse press events.
     * @param x X-coordinate of the mouse position.
     * @param y Y-coordinate of the mouse position.
     */
    static void runMousePressHandler(const std::function<void(int, int)>& handler, int x, int y) {
        SwGuiApplication::instance()->postEvent([handler, x, y]() {
            handler(x, y);
        });
    }

    /**
     * @brief Runs a mouse double-click handler asynchronously.
     *
     * Posts an event to run the specified mouse double-click handler with the given coordinates.
     *
     * @param handler Callback function to handle mouse double-click events.
     * @param x X-coordinate of the mouse position.
     * @param y Y-coordinate of the mouse position.
     */
    static void runMouseDoubleClickHandler(const std::function<void(int, int)>& handler, int x, int y) {
        SwGuiApplication::instance()->postEvent([handler, x, y]() {
            handler(x, y);
        });
    }

    /**
     * @brief Runs a mouse release handler asynchronously.
     *
     * Posts an event to run the specified mouse release handler with the given coordinates.
     *
     * @param handler Callback function to handle mouse release events.
     * @param x X-coordinate of the mouse position.
     * @param y Y-coordinate of the mouse position.
     */
    static void runMouseReleaseHandler(const std::function<void(int, int)>& handler, int x, int y) {
        SwGuiApplication::instance()->postEvent([handler, x, y]() {
            handler(x, y);
        });
    }

    /**
     * @brief Runs a mouse move handler asynchronously.
     *
     * Posts an event to run the specified mouse move handler with the given coordinates.
     *
     * @param handler Callback function to handle mouse move events.
     * @param x X-coordinate of the mouse position.
     * @param y Y-coordinate of the mouse position.
     */
    static void runMouseMoveHandler(const std::function<void(int, int)>& handler, int x, int y) {
        SwGuiApplication::instance()->postEvent([handler, x, y]() {
            handler(x, y);
        });
    }

    /**
     * @brief Runs a key press handler asynchronously.
     *
     * Posts an event to run the specified key press handler with the provided key code and modifier states.
     *
     * @param handler Callback function to handle key press events.
     * @param keyCode Virtual key code of the pressed key.
     * @param ctrlPressed Indicates whether the Ctrl key is pressed.
     * @param shiftPressed Indicates whether the Shift key is pressed.
     * @param altPressed Indicates whether the Alt key is pressed.
     */
    static void runKeyPressHandler(const std::function<void(int, bool, bool, bool)>& handler,
                                   int keyCode, bool ctrlPressed, bool shiftPressed, bool altPressed) {
        SwGuiApplication::instance()->postEvent([handler, keyCode, ctrlPressed, shiftPressed, altPressed]() {
            handler(keyCode, ctrlPressed, shiftPressed, altPressed);
        });
    }

    /**
     * @brief Runs a delete handler asynchronously.
     *
     * Posts an event to run the specified delete handler.
     *
     * @param handler Callback function to handle delete operations.
     */
    static void runDeleteHandler(const std::function<void()>& handler) {
        SwGuiApplication::instance()->postEvent([handler]() {
            handler();
        });
    }

    /**
     * @brief Unregisters all registered windows.
     *
     * This function is called during application shutdown to deregister and clean up all managed windows.
     */
    void unregisterAllWindows() {
        std::lock_guard<std::mutex> lock(s_windowMutex);
        for (auto& pair : s_windows) {
            if (pair.second.deleteHandler) {
                pair.second.deleteHandler();
            }
        }
        s_windows.clear();
    }

    /**
     * @brief Sets the current thread priority to high.
     *
     * Adjusts the priority level of the current thread to ensure it runs with higher priority.
     */
    void setThreadPriorityHigh() {
        HANDLE thread = GetCurrentThread();
        SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
    }

    // Static members
    static std::map<HWND, WindowCallbacks> s_windows; ///< Map associating HWND with WindowCallbacks
    static std::mutex s_windowMutex; ///< Mutex protecting access to s_windows
};

// Initialize static members
std::map<HWND, WindowCallbacks> SwGuiApplication::s_windows;
std::mutex SwGuiApplication::s_windowMutex;
