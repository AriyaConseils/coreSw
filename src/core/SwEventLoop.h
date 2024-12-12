#pragma once

#include "Object.h"
#include "SwCoreApplication.h"
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>

class SwEventLoop : public Object {
public:
    SwEventLoop() : running(false) {}

    ~SwEventLoop() {
        quit();
    }

    int exec() {
        running = true;

        while (running) {
            int interval = SwCoreApplication::instance()->exec();
            //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return exitCode;
    }

    void quit() {
        running = false;
    }

    void exit(int code = 0) {
        exitCode = code;
        quit();  // Quitter la boucle
    }

    static void wait(int timeout) {
        auto startTime = std::chrono::steady_clock::now(); 
         SwCoreApplication::instance()->exec(timeout*1000);

         auto endTime = std::chrono::steady_clock::now();
         auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

         std::cout << "Temps écoulé : " << elapsedTime << " millisecondes" << std::endl;
    }

private:
    bool running;
    int exitCode = 0;
};
