#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <queue>
#include <chrono>
#include <functional>
#include <condition_variable>
#include <limits>   // Pour std::numeric_limits
#include <algorithm>  // Pour std::max
#include <windows.h>
#include "SwAny.h"
#include "SwMap.h"






// Classe _T pour les tâches périodiques en microsecondes
class _T {
    friend class SwCoreApplication;

public:
    // Constructeur
    _T(std::function<void()> callback, int interval)
        : callback(callback),
        interval(interval),
        lastExecutionTime(std::chrono::steady_clock::now()) {}

    // Vérifie si le timer est prêt à être exécuté
    bool isReady() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - lastExecutionTime).count() >= interval;
    }

    // Exécute le callback et réinitialise le timer
    void execute() {
        callback();
        lastExecutionTime = std::chrono::steady_clock::now();
    }

    // Calcul du temps restant avant que le timer soit prêt à être exécuté
    int timeUntilReady() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastExecutionTime).count();
        return (std::max)(0, interval - static_cast<int>(elapsed));
    }

private:
    std::function<void()> callback;
    int interval;  // Intervalle en microsecondes
    std::chrono::steady_clock::time_point lastExecutionTime;
};


// Classe CoreApplication pour gérer la boucle d'événements en microsecondes
class SwCoreApplication {
    friend class SwTimer;
    friend class SwEventLoop;
public:
    SwCoreApplication() : running(true), exitCode(0) {
        instance(false) = this;
        enableHighPrecisionTimers();
    }
    SwCoreApplication(int argc, char* argv[])
        : running(true), exitCode(0) {
        instance(false) = this;
        enableHighPrecisionTimers();

        parseArguments(argc, argv);
    }

    ~SwCoreApplication() {
        disableHighPrecisionTimers();
    }

    // Ajouter un événement à la boucle
    void postEvent(std::function<void()> event) {
        std::lock_guard<std::mutex> lock(eventQueueMutex);
        eventQueue.push(event);
        cv.notify_one();
    }

    // Ajouter un timer à la boucle d'événements et retourner un ID unique
    int addTimer(std::function<void()> callback, int interval) {
        int timerId = nextTimerId++;
        timers.emplace(timerId, new _T(callback, interval));
        return timerId;
    }


    // Supprimer un timer de la boucle d'événements
    void removeTimer(int timerId) {
        auto it = timers.find(timerId);
        if (it != timers.end()) {
            delete it->second;
            timers.erase(it);
        }
        auto itOld = timers.find(-1);
        if (itOld != timers.end()) {
            delete itOld->second;
            timers.erase(itOld);
        }
    }



    // Démarrer la boucle d'événements
    virtual int exec() {
        setHighThreadPriority();

        auto lastTime = std::chrono::steady_clock::now();
        while (running) {
            auto currentTime = std::chrono::steady_clock::now();
            int sleepDuration = processEvent();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime).count();
            int adjustedSleepDuration = (std::max)(0, sleepDuration - static_cast<int>(elapsed));
            lastTime = currentTime;
            std::this_thread::sleep_for(std::chrono::microseconds(adjustedSleepDuration / 2));
        }
        return exitCode;
    }

    // Démarrer avec une durée maximale
    virtual int exec(int maxDurationMicroseconds) {
        setHighThreadPriority();

        auto startTime = std::chrono::steady_clock::now();
        auto lastTime = startTime;

        while (running) {
            auto currentTime = std::chrono::steady_clock::now();
            int sleepDuration = processEvent();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime).count();
            int adjustedSleepDuration = (std::max)(0, sleepDuration - static_cast<int>(elapsed));
            lastTime = currentTime;
            auto totalElapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count();
            if (totalElapsed >= maxDurationMicroseconds) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(adjustedSleepDuration / 2));
        }
        return exitCode;
    }

    // Traiter un seul événement (s'il y en a) et retourner la durée de sommeil recommandée
    int processEvent(bool waitForEvent = false) {
        std::unique_lock<std::mutex> lock(eventQueueMutex);

        if (eventQueue.empty() && timers.empty() && waitForEvent) {
            cv.wait(lock);
        }

        if (!eventQueue.empty()) {
            auto event = eventQueue.front();
            eventQueue.pop();
            lock.unlock();
            event();
            return 0;
        }
        lock.unlock();

        // Traiter les timers
        auto now = std::chrono::steady_clock::now();
        int minTimeUntilNext = (std::numeric_limits<int>::max)();

        for (auto& timer : timers) {
            if (timer.second->isReady()) {
                timer.second->execute();
            }
            else {
                int timeUntilNext = timer.second->timeUntilReady();
                if (timeUntilNext < minTimeUntilNext) {
                    minTimeUntilNext = timeUntilNext;
                }
            }
        }
        if (!eventQueue.empty()) {
            return 0;
        }
        return minTimeUntilNext != (std::numeric_limits<int>::max)() ? minTimeUntilNext : 10;
    }


    bool hasPendingEvents() {
        std::lock_guard<std::mutex> lock(eventQueueMutex);
        return !eventQueue.empty() || !timers.empty();
    }

    void exit(int code = 0) {
        exitCode = code;
        quit();
    }

    void quit() {
        running = false;
        cv.notify_all();
    }


    static SwCoreApplication* &instance(bool create = true) {
        static SwCoreApplication* _mainApp = nullptr;

        if(!_mainApp && create)
            _mainApp = new SwCoreApplication();

        return _mainApp;
    }

    SwString getArgument(const SwString& key, const SwString& defaultValue = "") const {
        if (parsedArguments.contains(key)) {
            return parsedArguments[key];
        }
        return defaultValue;
    }

    bool hasArgument(const SwString& key) const {
        return parsedArguments.contains(key);
    }

    SwList<SwString> getPositionalArguments() const {
        SwList<SwString> positionalArgs;
        for (auto it = parsedArguments.begin(); it != parsedArguments.end(); ++it) {
            const SwString& key = it->first;
            if (!key.startsWith("-")) {
                positionalArgs.append(key);
            }
        }
        return positionalArgs;
    }

private:

    void enableHighPrecisionTimers() {
        HMODULE hWinMM = LoadLibrary(TEXT("winmm.dll")); // Utiliser TEXT pour compatibilité Unicode/Multi-Byte
        if (hWinMM) {
            // Obtenir l'adresse de la fonction timeBeginPeriod
            auto timeBeginPeriodFunc = (MMRESULT(WINAPI*)(UINT))GetProcAddress(hWinMM, "timeBeginPeriod");
            if (timeBeginPeriodFunc) {
                MMRESULT result = timeBeginPeriodFunc(1); // Passer la résolution souhaitée (1ms)
                if (result != TIMERR_NOERROR) {
                    std::cerr << "[ERROR] Failed to enable high precision timers. Code: " << result << std::endl;
                }
            } else {
                std::cerr << "[ERROR] Could not locate 'timeBeginPeriod' in winmm.dll. High precision mode not activated." << std::endl;
            }
            FreeLibrary(hWinMM); // Libérer la bibliothèque
        } else {
            std::cerr << "[ERROR] Failed to load winmm.dll. High precision timers are unavailable." << std::endl;
            std::cerr << "[INFO] Default timer precision will be used (typically 15ms)." << std::endl;
        }
    }

    void disableHighPrecisionTimers() {
        HMODULE hWinMM = LoadLibrary(TEXT("winmm.dll")); // Utiliser TEXT pour compatibilité Unicode/Multi-Byte
        if (hWinMM) {
            // Obtenir l'adresse de la fonction timeEndPeriod
            auto timeEndPeriodFunc = (MMRESULT(WINAPI*)(UINT))GetProcAddress(hWinMM, "timeEndPeriod");
            if (timeEndPeriodFunc) {
                MMRESULT result = timeEndPeriodFunc(1); // Restaurer la résolution par défaut
                if (result != TIMERR_NOERROR) {
                    std::cerr << "[ERROR] Failed to disable high precision timers. Code: " << result << std::endl;
                }
            } else {
                std::cerr << "[ERROR] Could not locate 'timeEndPeriod' in winmm.dll. Default resolution may not be restored." << std::endl;
            }
            FreeLibrary(hWinMM); // Libérer la bibliothèque
        } else {
            std::cerr << "[ERROR] Failed to load winmm.dll. Default resolution remains unchanged." << std::endl;
        }
    }



    void setHighThreadPriority() {
        HANDLE thread = GetCurrentThread();
        SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
    }

    void parseArguments(int argc, char* argv[]) {
        SwString lastKey = ""; // Garde en mémoire la dernière clé rencontrée

        for (int i = 1; i < argc; ++i) {
            SwString arg = argv[i];

            if (arg.startsWith("--")) { // Option longue (--option)
                auto eqPos = arg.indexOf('=');
                if (eqPos != -1) {
                    // Format --option=value
                    SwString key = arg.mid(2, eqPos - 2); // Clé après "--" jusqu'à '='
                    SwString value = arg.mid(eqPos + 1);  // Valeur après '='
                    parsedArguments[key] = value;
                    lastKey = key; // Mémorise cette clé
                } else {
                    // Format --option [value]
                    SwString key = arg.mid(2); // Clé après "--"
                    SwString value = "";
                    if (i + 1 < argc && !SwString(argv[i + 1]).startsWith("-")) {
                        value = argv[++i]; // La valeur est l'argument suivant s'il ne commence pas par '-'
                    }
                    parsedArguments[key] = value;
                    lastKey = key; // Mémorise cette clé
                }
            } else if (arg.startsWith("-")) { // Option courte (-o)
                SwString key = arg.mid(1); // Clé après "-"
                SwString value = "";
                if (i + 1 < argc && !SwString(argv[i + 1]).startsWith("-")) {
                    value = argv[++i]; // La valeur est l'argument suivant s'il ne commence pas par '-'
                }
                parsedArguments[key] = value;
                lastKey = key; // Mémorise cette clé
            } else {
                // Si l'argument ne commence pas par '-' ou '--', on l'ajoute à la valeur de la dernière clé
                if (!lastKey.isEmpty()) {
                    parsedArguments[lastKey] += SwString(" ") + arg;
                }
            }
        }
    }



private:
    bool running;
    int exitCode;
    std::queue<std::function<void()>> eventQueue;
    std::mutex eventQueueMutex;
    std::condition_variable cv;
    int nextTimerId = 0;
    std::map<int, _T*> timers;
    SwMap<SwString, SwString> parsedArguments;
};
