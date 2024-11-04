#pragma once

#include "CoreApplication.h"
#include <windows.h>
#include <gdiplus.h>


using namespace Gdiplus;
class GuiApplication : public CoreApplication {
public:
    GuiApplication() : CoreApplication() {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
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

            // Ajouter une petite pause pour éviter d'utiliser 100% du CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        GdiplusShutdown(gdiplusToken);
        return 0;
    }
private:
    ULONG_PTR gdiplusToken;
};
