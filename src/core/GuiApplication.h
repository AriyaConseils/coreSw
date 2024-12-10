#pragma once

#include "CoreApplication.h"
#include <windows.h>
#include <gdiplus.h>


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

class GuiApplication : public CoreApplication {
public:
    GuiApplication() : CoreApplication() {
        // Initialiser GDI+ via SwGdiPlusEngine
        SwGdiPlusEngine::instance().initialize();
    }

    ~GuiApplication()  {
        // Arrêter proprement GDI+ dans le destructeur
        SwGdiPlusEngine::instance().shutdown();
    }

    int exec() override {
        MSG msg = {};
        while (true) {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    CoreApplication::exit(0);
                    return 0;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }


            processEvent();

            // Pause pour réduire l'utilisation du CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

