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

/***************************************************************************************************
 * Acknowledgements:                                                                                *
 *                                                                                                  *
 * A heartfelt thank you to nullprogram.com, and especially to Chris Wellons, for the exceptional   *
 * quality of the technical information and insights shared on his blog:                            *
 * https://nullprogram.com/blog/2019/03/28/.                                                        *
 *                                                                                                  *
 * His guidance on Windows fibers and related concepts provided an invaluable foundation for        *
 * implementing this architecture. The clarity, detail, and thoroughness of his posts made          *
 * building a flexible, fiber-based event handling and async/await system more approachable.        *
 *                                                                                                  *
 * Contact for the blog's author:                                                                   *
 *  - Chris Wellons                                                                                 *
 *  - wellons@nullprogram.com (PGP)                                                                 *
 *  - ~skeeto/public-inbox@lists.sr.ht (mailing list)                                               *
 *                                                                                                  *
 ***************************************************************************************************/

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <queue>
#include <chrono>
#include <functional>
#include <condition_variable>
#include <limits>
#include <algorithm>
#include <windows.h>
#include "SwMap.h"
#include "SwString.h"

/**
 * @brief Internal class representing a timer that executes a callback at regular intervals.
 */
class _T {
    friend class SwCoreApplication;
public:

    /**
     * @brief Constructs a timer.
     * @param callback The function to call when the timer expires.
     * @param interval The interval in microseconds between two executions of the callback.
     */
    _T(std::function<void()> callback, int interval)
        : callback(callback),
        interval(interval),
        lastExecutionTime(std::chrono::steady_clock::now()) {}

    /**
     * @brief Checks if the timer is ready to execute its callback.
     * @return `true` if the interval has elapsed since the last execution, `false` otherwise.
     */
    bool isReady() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - lastExecutionTime).count() >= interval;
    }

    /**
     * @brief Executes the timer's callback and updates the last execution time.
     */
    void execute() {
        callback();
        lastExecutionTime = std::chrono::steady_clock::now();
    }

    /**
     * @brief Calculates the remaining time until the timer is ready.
     * @return The time in microseconds before the timer is ready. Returns 0 if already ready.
     */
    int timeUntilReady() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastExecutionTime).count();
        return (std::max)(0, interval - static_cast<int>(elapsed));
    }

private:
    std::function<void()> callback; ///< Callback to execute when the timer expires.
    int interval; ///< Interval in microseconds between callback executions.
    std::chrono::steady_clock::time_point lastExecutionTime; ///< Timestamp of the last callback execution.
};

/**
 * @brief Main application class utilizing an event system and fibers.
 *
 * This class manages an event loop, timers, and a yield/unYield mechanism to pause and resume
 * the execution of certain fibers. It also employs high-precision timers on Windows.
 */
class SwCoreApplication {
public:
    /**
     * @brief Default constructor of the application.
     *
     * Initializes high-precision timers, converts the main thread to a fiber, and sets up necessary structures.
     */
    SwCoreApplication()
        : running(true), exitCode(0) {
        instance(false) = this;
        enableHighPrecisionTimers();
        initFibers();
    }

    /**
     * @brief Constructor with command-line arguments.
     * @param argc Number of arguments.
     * @param argv Array of argument strings.
     *
     * Initializes high-precision timers, parses command-line arguments, converts the main thread to a fiber,
     * and sets up necessary structures.
     */
    SwCoreApplication(int argc, char* argv[])
        : running(true), exitCode(0) {
        instance(false) = this;
        enableHighPrecisionTimers();
        parseArguments(argc, argv);
        initFibers();
    }

    /**
     * @brief Destructor.
     *
     * Disables high-precision timers and performs necessary cleanup.
     */
    virtual ~SwCoreApplication() {
        disableHighPrecisionTimers();
    }

    /**
     * @brief Accesses the static instance of the application.
     * @param create If `true` and the instance does not exist, creates one.
     * @return Reference to the static application instance pointer.
     */
    static SwCoreApplication*& instance(bool create = true) {
        static SwCoreApplication* _mainApp = nullptr;
        if(!_mainApp && create)
            _mainApp = new SwCoreApplication();
        return _mainApp;
    }

    /**
     * @brief Posts an event (a function) to the event queue.
     * @param event Function to execute during event processing.
     */
    void postEvent(std::function<void()> event) {
        {
            std::lock_guard<std::mutex> lock(eventQueueMutex);
            eventQueue.push(event);
        }
        cv.notify_one();
    }

    /**
     * @brief Adds a timer.
     * @param callback Function to call when the timer expires.
     * @param interval Interval in microseconds between two executions of the callback.
     * @return The identifier of the created timer.
     */
    int addTimer(std::function<void()> callback, int interval) {
        int timerId = nextTimerId++;
        timers.emplace(timerId, new _T(callback, interval));
        return timerId;
    }

    /**
     * @brief Removes a timer.
     * @param timerId Identifier of the timer to remove.
     */
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

    /**
     * @brief Starts the main event loop.
     * @return Exit code of the application.
     */
    virtual int exec() {
        setHighThreadPriority();
        auto lastTime = std::chrono::steady_clock::now();

        while (running) {
            auto currentTime = std::chrono::steady_clock::now();
            int sleepDuration = processEvent();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime).count();
            int adjustedSleepDuration = (std::max)(0, sleepDuration - static_cast<int>(elapsed));
            lastTime = currentTime;

            // À la fin de chaque cycle, on vérifie s'il y a des fibers prêtes à être reprises.
            resumeReadyFibers();

            std::this_thread::sleep_for(std::chrono::microseconds(adjustedSleepDuration / 2));
        }
        return exitCode;
    }

    /**
     * @brief Starts the main event loop for a specified duration.
     * @param maxDurationMicroseconds Maximum duration in microseconds.
     * @return Exit code of the application.
     */
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

    /**
     * @brief Processes an event and manages timers.
     * @param waitForEvent If `true`, waits for an event to arrive.
     * @return The time in microseconds until the next timer expires or 0 if an event is imminent.
     */
    int processEvent(bool waitForEvent = false) {
        std::unique_lock<std::mutex> lock(eventQueueMutex);

        if (eventQueue.empty() && timers.empty() && waitForEvent) {
            cv.wait(lock);
        }

        if (!eventQueue.empty()) {
            auto event = eventQueue.front();
            eventQueue.pop();
            lock.unlock();
            runEventInFiber(event);
            return 0;
        }
        lock.unlock();

        // Timer Managment
        int minTimeUntilNext = (std::numeric_limits<int>::max)();
        for (auto& timer : timers) {
            if (timer.second->isReady()) {
                int currentTimerId = timer.first;
                _T* currentTimer = timer.second;

                //Call execute() insted of callback() to reload the next rendez-vous
                std::function<void()> timerEvent = [currentTimer]() {
                    currentTimer->execute();
                };

                runEventInFiber(timerEvent);
            } else {
                int timeUntilNext = timer.second->timeUntilReady();
                if (timeUntilNext < minTimeUntilNext) {
                    minTimeUntilNext = timeUntilNext;
                }
            }
        }

        //last check is case of yield waking up
        resumeReadyFibers();

        // If eventQueue is not empty we have to start very quickly, so return 0ms
        if (!eventQueue.empty()) {
            return 0;
        }
        return minTimeUntilNext != (std::numeric_limits<int>::max)() ? minTimeUntilNext : 10;
    }

    /**
     * @brief Checks if there are pending events or timers.
     * @return `true` if there are pending events or timers, `false` otherwise.
     */
    bool hasPendingEvents() {
        std::lock_guard<std::mutex> lock(eventQueueMutex);
        return !eventQueue.empty() || !timers.empty();
    }

    /**
     * @brief Exits the application with a specified exit code.
     * @param code Exit code.
     */
    void exit(int code = 0) {
        exitCode = code;
        quit();
    }

    /**
     * @brief Exits the application.
     */
    void quit() {
        running = false;
        cv.notify_all();
    }

    /**
     * @brief Yields the current fiber, placing it into the yielded fibers map with the given ID.
     *        Returns control to the main fiber.
     * @param id Identifier associated with the yielded fiber.
     */
    static void yieldFiber(int id) {
        SwCoreApplication* app = instance(false);
        LPVOID current = GetCurrentFiber();

        // On stocke la fiber dans s_yieldedFibers
        {
            std::lock_guard<std::mutex> lock(s_yieldMutex);
            s_yieldedFibers[id] = current;
        }

        // Retour à la fiber principale
        SwitchToFiber(app->mainFiber);
    }

    /**
     * @brief Un-yields a previously yielded fiber by its ID, moving it to the ready fibers queue.
     *        It will be resumed by the main fiber later.
     * @param id Identifier of the fiber to un-yield.
     */
    static void unYieldFiber(int id) {
        LPVOID fiber = nullptr;
        {
            std::lock_guard<std::mutex> lock(s_yieldMutex);
            auto it = s_yieldedFibers.find(id);
            if (it != s_yieldedFibers.end()) {
                fiber = it->second;
                s_yieldedFibers.erase(it);
            }
        }

        if (fiber) {
            std::lock_guard<std::mutex> lock(s_readyMutex);
            s_readyFibers.push(fiber);
        }
    }

    /**
     * @brief Retrieves the value of a command-line argument.
     * @param key The key (name) of the argument.
     * @param defaultValue The default value if the argument is not found.
     * @return The value of the argument or `defaultValue` if not defined.
     */
    SwString getArgument(const SwString& key, const SwString& defaultValue = "") const {
        if (parsedArguments.contains(key)) {
            return parsedArguments[key];
        }
        return defaultValue;
    }

    /**
     * @brief Checks for the presence of a command-line argument.
     * @param key The key (name) of the argument.
     * @return `true` if the argument exists, `false` otherwise.
     */
    bool hasArgument(const SwString& key) const {
        return parsedArguments.contains(key);
    }

    /**
     * @brief Retrieves positional command-line arguments (those without a hyphen).
     * @return A list of strings containing the positional arguments.
     */
    SwList<SwString> getPositionalArguments() const {
        SwList<SwString> positionalArgs;
        for (auto it = parsedArguments.begin(); it != parsedArguments.end(); ++it) {
            const SwString& k = it->first;
            if (!k.startsWith("-")) {
                positionalArgs.append(k);
            }
        }
        return positionalArgs;
    }

protected:

    /**
     * @brief Enables high-precision timers using the Windows multimedia timer.
     */
    void enableHighPrecisionTimers() {
        HMODULE hWinMM = LoadLibrary(TEXT("winmm.dll"));
        if (hWinMM) {
            auto timeBeginPeriodFunc = (MMRESULT(WINAPI*)(UINT))GetProcAddress(hWinMM, "timeBeginPeriod");
            if (timeBeginPeriodFunc) {
                MMRESULT result = timeBeginPeriodFunc(1);
                if (result != TIMERR_NOERROR) {
                    std::cerr << "[ERROR] Failed to enable high precision timers. Code: " << result << std::endl;
                }
            }
            FreeLibrary(hWinMM);
        }
    }

    /**
     * @brief Disables high-precision timers using the Windows multimedia timer.
     */
    void disableHighPrecisionTimers() {
        HMODULE hWinMM = LoadLibrary(TEXT("winmm.dll"));
        if (hWinMM) {
            auto timeEndPeriodFunc = (MMRESULT(WINAPI*)(UINT))GetProcAddress(hWinMM, "timeEndPeriod");
            if (timeEndPeriodFunc) {
                timeEndPeriodFunc(1);
            }
            FreeLibrary(hWinMM);
        }
    }

    /**
     * @brief Sets a high thread priority for the current thread.
     */
    void setHighThreadPriority() {
        HANDLE thread = GetCurrentThread();
        SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
    }

    /**
     * @brief Parses command-line arguments and stores them.
     * @param argc Number of arguments.
     * @param argv Array of argument strings.
     */
    void parseArguments(int argc, char* argv[]) {
        SwString lastKey = "";
        for (int i = 1; i < argc; ++i) {
            SwString arg = argv[i];
            if (arg.startsWith("--")) {
                auto eqPos = arg.indexOf('=');
                if (eqPos != -1) {
                    SwString key = arg.mid(2, eqPos - 2);
                    SwString value = arg.mid(eqPos + 1);
                    parsedArguments[key] = value;
                    lastKey = key;
                } else {
                    SwString key = arg.mid(2);
                    SwString value = "";
                    if (i + 1 < argc && !SwString(argv[i + 1]).startsWith("-")) {
                        value = argv[++i];
                    }
                    parsedArguments[key] = value;
                    lastKey = key;
                }
            } else if (arg.startsWith("-")) {
                SwString key = arg.mid(1);
                SwString value = "";
                if (i + 1 < argc && !SwString(argv[i + 1]).startsWith("-")) {
                    value = argv[++i];
                }
                parsedArguments[key] = value;
                lastKey = key;
            } else {
                if (!lastKey.isEmpty()) {
                    parsedArguments[lastKey] += SwString(" ") + arg;
                }
            }
        }
    }

    /**
     * @brief Converts the main thread to a fiber.
     *
     * If the conversion fails, an error message is printed.
     */
    void initFibers() {
        mainFiber = ConvertThreadToFiber(nullptr);
        if (!mainFiber) {
            std::cerr << "Failed to convert main thread to fiber. Error: " << GetLastError() << std::endl;
        }
    }

    /**
     * @brief Entry function for a newly created fiber.
     * @param lpParameter Pointer to the function to execute within the fiber.
     */
    static VOID WINAPI FiberProc(LPVOID lpParameter) {
        std::function<void()>* callback = reinterpret_cast<std::function<void()>*>(lpParameter);
        (*callback)();
        // Une fois le callback terminé, revenir à la fiber principale
        SwitchToFiber(instance(false)->mainFiber);
    }

    /**
     * @brief Executes an event (function) within a fiber and manages its lifecycle.
     * @param event Function to execute.
     */
    void runEventInFiber(const std::function<void()>& event) {
        std::function<void()>* cbPtr = new std::function<void()>(event);
        LPVOID newFiber = CreateFiber(0, FiberProc, cbPtr);
        if (!newFiber) {
            std::cerr << "Failed to create fiber. Error: " << GetLastError() << "\n";
            (*cbPtr)();
            delete cbPtr;
            return;
        }

        m_runningFiber = newFiber;
        SwitchToFiber(newFiber);
        // Returned here after the fiber has finished or yielded again.
        // Check if it has been yielded
        deleteFiberIfNeeded(newFiber);
    }

    /**
     * @brief Resumes fibers that have been un-yielded.
     *
     * Multiple fibers can be resumed in succession.
     */
    void resumeReadyFibers() {
        // Resume fibers that have been "unYielded"
        while (true) {
            LPVOID fiber = nullptr;
            {
                std::lock_guard<std::mutex> lock(s_readyMutex);
                if (!s_readyFibers.empty()) {
                    fiber = s_readyFibers.front();
                    s_readyFibers.pop();
                } else {
                    break; // Pas de fiber à reprendre
                }
            }

            if (fiber) {
                // Reprendre la fiber
                SwitchToFiber(fiber);
                // De retour ici après que la fiber ait terminé ou yield à nouveau.
                // Vérifier si elle est yieldée
                deleteFiberIfNeeded(fiber);
            }
        }
    }

    /**
     * @brief Deletes a fiber if it is no longer yielded.
     * @param fiber Pointer to the fiber to delete.
     */
    void deleteFiberIfNeeded(void* fiber) {
        bool isYielded = false;

        {
            std::lock_guard<std::mutex> lock(s_yieldMutex);
            for (auto &kv : s_yieldedFibers) {
                if (kv.second == fiber) {
                    // The fiber is in the yielded fibers map.
                    isYielded = true;
                    break;
                }
            }
        }

        if (!isYielded) {
            // The fiber is not yielded, so it has finished.
            DeleteFiber(fiber);
        } else {
            // The fiber is yielded, so do not delete it.
            // It will be managed by the unYield mechanism later.
            // NOTE: It might be necessary to keep the associated cbPtr in another structure
            // for reuse or deletion later.
            // For now, we assume that the yielded fiber manages its own data.
        }
    }

    /**
     * @brief Retrieves the currently running fiber.
     * @return Pointer to the currently running fiber.
     */
    LPVOID getRunningFiber() {
        return m_runningFiber;
    }

protected:
    bool running; ///< Indicates if the event loop is running.
    int exitCode; ///< Exit code of the application.

    std::queue<std::function<void()>> eventQueue; ///< Queue of events to process.
    std::mutex eventQueueMutex; ///< Mutex protecting access to the event queue.
    std::condition_variable cv; ///< Condition variable for event waiting.

    int nextTimerId = 0; ///< Identifier for the next timer to be created.
    std::map<int, _T*> timers; ///< Map associating timer IDs with their respective _T objects.
    SwMap<SwString, SwString> parsedArguments; ///< Parsed command-line arguments.

    LPVOID m_runningFiber = nullptr; ///< Pointer to the currently running fiber.
    LPVOID mainFiber = nullptr; ///< Pointer to the main fiber.

    // Static members for managing yield/unYield
    static std::mutex s_yieldMutex; ///< Mutex protecting the s_yieldedFibers map.
    static std::map<int, LPVOID> s_yieldedFibers; ///< Map of yielded fibers indexed by ID.
    static std::mutex s_readyMutex; ///< Mutex protecting the s_readyFibers queue.
    static std::queue<LPVOID> s_readyFibers; ///< Queue of fibers ready to be resumed.
};

// Declaration of static members
std::mutex SwCoreApplication::s_yieldMutex;
std::map<int, LPVOID> SwCoreApplication::s_yieldedFibers;
std::mutex SwCoreApplication::s_readyMutex;
std::queue<LPVOID> SwCoreApplication::s_readyFibers;
