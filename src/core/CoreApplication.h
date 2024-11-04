#pragma once
#pragma comment(lib, "winmm.lib")

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

// Classe _T pour les tâches périodiques en microsecondes
class _T {
    friend class CoreApplication;
public:
    _T(std::function<void()> callback, int interval);

    // Vérifie si le timer est prêt à être exécuté
    bool isReady() const;

    // Exécute le callback et réinitialise le timer
    void execute();

    // Calcul du temps restant avant que le timer soit prêt à être exécuté
    int timeUntilReady() const;

private:
    std::function<void()> callback;
    int interval;  // Intervalle en microsecondes
    std::chrono::steady_clock::time_point lastExecutionTime;
};

// Classe CoreApplication pour gérer la boucle d'événements en microsecondes
class CoreApplication {
public:
    CoreApplication();
    ~CoreApplication();

    // Ajouter un événement à la boucle
    void postEvent(std::function<void()> event);

    // Ajouter un timer à la boucle d'événements et retourner un ID unique
    int addTimer(std::function<void()> callback, int interval);

    // Supprimer un timer de la boucle d'événements
    void removeTimer(int timerId);

    // Démarrer la boucle d'événements
    virtual int exec();
    virtual int exec(int maxDurationMicroseconds);

    // Traiter un seul événement (s'il y en a) et retourner la durée de sommeil recommandée
    int processEvent(bool waitForEvent = false);

    bool hasPendingEvents();

    void exit(int code = 0);

    void quit();

    static CoreApplication* instance();

private:
    static void setMainApp(CoreApplication* app);

    void enableHighPrecisionTimers();
    void disableHighPrecisionTimers();
    void setHighThreadPriority();

private:
    static CoreApplication* m_mainApp;
    bool running;
    int exitCode;
    std::queue<std::function<void()>> eventQueue;
    std::mutex eventQueueMutex;
    std::condition_variable cv;
    int nextTimerId = 0;
    std::map<int, _T*> timers;
};
