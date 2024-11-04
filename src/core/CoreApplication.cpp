// ExampleClass.cpp
#include "CoreApplication.h"


// Définition de la variable statique
CoreApplication* CoreApplication::m_mainApp = nullptr;

// Constructeur de la classe _T
_T::_T(std::function<void()> callback, int interval)
    : callback(callback), interval(interval), lastExecutionTime(std::chrono::steady_clock::now()) {}

// Vérifie si le timer est prêt à être exécuté
bool _T::isReady() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - lastExecutionTime).count() >= interval;
}

// Exécute le callback et réinitialise le timer
void _T::execute() {
    callback();
    lastExecutionTime = std::chrono::steady_clock::now();
}

// Calcul du temps restant avant que le timer soit prêt à être exécuté
int _T::timeUntilReady() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastExecutionTime).count();
    return (std::max)(0, interval - static_cast<int>(elapsed));
}

// Constructeur de CoreApplication
CoreApplication::CoreApplication() : running(true), exitCode(0) {
    CoreApplication::setMainApp(this);
    enableHighPrecisionTimers();
    SwAny::registerAllType();
}

// Destructeur de CoreApplication
CoreApplication::~CoreApplication() {
    disableHighPrecisionTimers();
}

// Ajouter un événement à la boucle
void CoreApplication::postEvent(std::function<void()> event) {
    std::lock_guard<std::mutex> lock(eventQueueMutex);
    eventQueue.push(event);
    cv.notify_one();
}

// Ajouter un timer à la boucle d'événements et retourner un ID unique
int CoreApplication::addTimer(std::function<void()> callback, int interval) {
    int timerId = nextTimerId++;
    timers.emplace(timerId, new _T(callback, interval));
    return timerId;
}

// Supprimer un timer de la boucle d'événements
void CoreApplication::removeTimer(int timerId) {
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
int CoreApplication::exec() {
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
int CoreApplication::exec(int maxDurationMicroseconds) {
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
int CoreApplication::processEvent(bool waitForEvent) {
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

bool CoreApplication::hasPendingEvents() {
    std::lock_guard<std::mutex> lock(eventQueueMutex);
    return !eventQueue.empty() || !timers.empty();
}

void CoreApplication::exit(int code) {
    exitCode = code;
    quit();
}

void CoreApplication::quit() {
    running = false;
    cv.notify_all();
}

CoreApplication* CoreApplication::instance() {
    return m_mainApp;
}

void CoreApplication::setMainApp(CoreApplication* app) {
    m_mainApp = app;
}

void CoreApplication::enableHighPrecisionTimers() {
    timeBeginPeriod(1);
}

void CoreApplication::disableHighPrecisionTimers() {
    timeEndPeriod(1);
}

void CoreApplication::setHighThreadPriority() {
    HANDLE thread = GetCurrentThread();
    SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
}
