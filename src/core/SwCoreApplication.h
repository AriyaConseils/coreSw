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
 * @brief Internal class representing a high-precision timer that executes a callback at specified intervals.
 *
 * The `_T` class is used internally by `SwCoreApplication` to manage timers. It allows scheduling
 * and execution of a callback function either as a recurring task or as a single-shot event.
 * The timer operates with microsecond precision, making it suitable for applications requiring
 * accurate timing.
 *
 * ### Key Features:
 * - **Callback Execution**:
 *   - Executes the provided callback function when the timer is ready.
 * - **Precision Timing**:
 *   - Tracks elapsed time with microsecond-level precision using `std::chrono::steady_clock`.
 * - **Timer Modes**:
 *   - Supports both recurring timers and single-shot timers.
 * - **Utility Functions**:
 *   - Determines readiness of the timer with `isReady`.
 *   - Calculates the time remaining until the timer is ready with `timeUntilReady`.
 *
 * ### Usage:
 * 1. Create an instance of `_T` with a callback function, interval (in microseconds), and
 *    an optional single-shot mode.
 * 2. Use `isReady` to check if the timer is ready for execution.
 * 3. Call `execute` to run the callback and reset the timer (for recurring timers).
 * 4. Use `timeUntilReady` to query the time remaining until the timer is ready.
 *
 * ### Example:
 * ```cpp
 * _T timer([]() { std::cout << "Timer fired!" << std::endl; }, 1000000, false); // 1-second recurring timer
 * if (timer.isReady()) {
 *     timer.execute();
 * }
 * ```
 *
 * @note This class is designed to be lightweight and efficient for use in high-frequency event loops.
 *
 * @warning Instances of `_T` are not thread-safe. Ensure that the timer is managed within a
 *          thread-safe context (e.g., using mutexes) if accessed concurrently.
 */
class _T {
    friend class SwCoreApplication;

public:
    /**
     * @brief Constructs a timer instance.
     * @param callback The function to execute when the timer fires.
     * @param interval The interval in microseconds between executions.
     * @param singleShot If `true`, the timer fires only once and must be manually removed after execution.
     */
    _T(std::function<void()> callback, int interval, bool singleShot = false)
        : callback(callback),
        interval(interval),
        singleShot(singleShot),
        lastExecutionTime(std::chrono::steady_clock::now())
    {}

    /**
     * @brief Checks if the timer is ready to fire.
     * @return `true` if the timer is ready, otherwise `false`.
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
     * @brief Calculates the time remaining until the timer is ready.
     * @return The time in microseconds until the timer is ready, or `0` if the timer is already ready.
     */
    int timeUntilReady() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastExecutionTime).count();
        return (std::max)(0, interval - static_cast<int>(elapsed));
    }

private:
    std::function<void()> callback; ///< The function to execute when the timer fires.
    int interval; ///< Interval in microseconds between timer executions.
    bool singleShot; ///< Indicates if the timer is single-shot (`true`) or recurring (`false`).
    std::chrono::steady_clock::time_point lastExecutionTime; ///< The last time the timer was executed.
};



// Déclaration du gestionnaire
static BOOL WINAPI ConsoleHandler(DWORD ctrlType);


/**
 * @brief Main application class for managing an event-driven system with fiber-based multitasking.
 *
 * The `SwCoreApplication` class serves as the core of the application, providing an event loop,
 * timer management, and fiber-based cooperative multitasking. It allows for precise control over
 * the execution flow of tasks through mechanisms like `yield`, `unYield`, and fiber switching.
 * Additionally, it utilizes high-precision timers on Windows to ensure accurate timing for events
 * and tasks.
 *
 * ### Key Features:
 * - **Event Loop**:
 *   - Handles an event queue, processing functions and callbacks asynchronously.
 *   - Supports efficient task scheduling and execution.
 * - **Timer Management**:
 *   - Allows adding, removing, and managing timers with microsecond-level precision.
 *   - Supports single-shot and recurring timers.
 * - **Fiber-Based Multitasking**:
 *   - Implements cooperative multitasking using Windows fibers.
 *   - Provides `yieldFiber` and `unYieldFiber` for pausing and resuming task execution.
 *   - Ensures efficient resource management for fibers through mechanisms like `release` and `deleteFiberIfNeeded`.
 * - **High-Precision Timing**:
 *   - Uses multimedia timers on Windows for enhanced precision in timer scheduling.
 *   - Ensures consistent timing behavior across tasks and events.
 *
 * ### Workflow:
 * 1. The main event loop (`exec`) continuously processes events and manages timers.
 * 2. Tasks and timers are executed within fibers, allowing for non-blocking execution.
 * 3. Fibers can yield control back to the main loop and resume later as needed.
 * 4. Events and timers are handled with thread-safe mechanisms to ensure consistency in
 *    multi-threaded environments.
 *
 * ### Typical Usage:
 * - Instantiate the `SwCoreApplication` class.
 * - Use `postEvent` to enqueue tasks or events.
 * - Add timers with `addTimer` for time-based task scheduling.
 * - Start the main event loop with `exec` to process tasks and timers.
 *
 * @note This class is designed for applications requiring precise control over asynchronous tasks,
 *       such as real-time systems or applications with complex scheduling requirements.
 *
 * @warning This class heavily relies on Windows-specific APIs (fibers and multimedia timers).
 *          It is not portable to non-Windows platforms.
 */
class SwCoreApplication {

    friend class SwTimer;
    friend class SwEventLoop;

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
        SetConsoleCtrlHandler(ConsoleHandler, TRUE);
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
        SetConsoleCtrlHandler(ConsoleHandler, TRUE);
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
    int addTimer(std::function<void()> callback, int interval, bool singleShot = false) {
        int timerId = nextTimerId++;
        timers.emplace(timerId, new _T(callback, interval, singleShot));
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
     * @brief Executes the main event loop for a specified duration.
     *
     * This method runs the application's main event loop for a maximum duration specified in microseconds.
     * It processes events, manages timers, and resumes fibers while ensuring precise time control
     * for the duration of the loop.
     *
     * ### Workflow:
     * 1. Sets the thread priority to high using `setHighThreadPriority`.
     * 2. Records the start time of the loop for duration tracking.
     * 3. Enters the main loop:
     *    - Processes events using `processEvent`, which also handles timers and fibers.
     *    - Calculates the elapsed time since the last iteration and adjusts the sleep duration
     *      to maintain timing precision.
     *    - Checks the total elapsed time and exits the loop if it exceeds the maximum duration.
     *    - Introduces a short sleep to prevent CPU overuse and allow for other tasks to execute.
     * 4. Exits the loop and returns the application's exit code.
     *
     * @param maxDurationMicroseconds Maximum time the loop should run, in microseconds.
     * @return The application's exit code, typically set using `exit()` or `quit()`.
     *
     * @note The loop ensures the system remains responsive by balancing event processing
     *       and controlled delays.
     *
     * @warning Ensure that the `running` flag is managed correctly to avoid infinite loops.
     *          If the application is terminated before the duration expires, the loop will exit early.
     *
     * @remarks This method is useful for applications that require running the event loop
     *          for a fixed duration, such as for testing or temporary tasks.
     */
    virtual int exec(int maxDurationMicroseconds = 0) {
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
            if (maxDurationMicroseconds != 0 && totalElapsed >= maxDurationMicroseconds) {
                break;
            }

            if(adjustedSleepDuration > 1000){
                std::this_thread::sleep_for(std::chrono::microseconds(adjustedSleepDuration / 2));
            }
        }
        return exitCode;
    }

    /**
     * @brief Processes a single event or manages timers, handling fibers as needed.
     *
     * This function processes events from the event queue, executes timer callbacks, and manages
     * the resumption of fibers that are ready to run. It is the core function for handling
     * event-driven tasks and timer-based scheduling within the application.
     *
     * ### Workflow:
     * 1. **Event Handling**:
     *    - If the event queue is empty and `waitForEvent` is `true`, the function waits for a new
     *      event or timer expiration using a condition variable.
     *    - If an event is available, it is extracted from the queue and executed within a fiber
     *      using `runEventInFiber`. The function returns `0` after processing the event.
     * 2. **Timer Management**:
     *    - Iterates through the list of active timers and checks if they are ready to execute.
     *    - Executes callbacks for ready timers within fibers:
     *        - If the timer is single-shot, it is removed and deleted after execution.
     *        - If the timer is recurring, it remains in the timer list.
     *    - Updates the minimum time until the next timer is ready.
     * 3. **Fiber Management**:
     *    - Calls `resumeReadyFibers` to handle fibers that were previously paused and are now
     *      ready to run.
     * 4. **Return Value**:
     *    - If an event or a timer is processed, the function returns `0`.
     *    - Otherwise, it returns the time in microseconds until the next timer is ready, or a
     *      small default value if no timers are active.
     *
     * @param waitForEvent If `true`, blocks and waits for an event to arrive if the event queue and
     *                     timer list are empty.
     * @return The time in microseconds until the next timer expires, or `0` if an event is imminent.
     *
     * @note This function ensures thread-safe access to the event queue and timer list using
     *       `std::mutex` locks.
     *
     * @warning Timer callbacks and events are executed within fibers. Care must be taken to ensure
     *          that these callbacks do not block or cause deadlocks.
     *
     * @remarks This function is designed to be called repeatedly within the main event loop of the
     *          application to drive the event and timer system.
     */
    int processEvent(bool waitForEvent = false) {
        std::unique_lock<std::mutex> lock(eventQueueMutex);

        // Wait for an event if the queue is empty and waiting is allowed
        if (eventQueue.empty() && timers.empty() && waitForEvent) {
            cv.wait(lock);
        }

        // Process the next event if available
        if (!eventQueue.empty()) {
            auto event = eventQueue.front();
            eventQueue.pop();
            lock.unlock(); // Unlock before running the event in a fiber
            runEventInFiber(event);
            return 0; // An event was processed, so no delay is required
        }
        lock.unlock();

        // Process timer and get ne next Rendez-vous
        int minTimeUntilNext = processTimers();

        // Resume fibers that are ready to run
        resumeReadyFibers();

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
     * @brief Releases the current fiber and queues it for re-execution in the event loop.
     *
     * This function is used to pause the execution of the current fiber and move it to the
     * `s_readyFibers` queue, enabling it to be resumed during the next iteration of the event loop.
     * Once the fiber is released, execution switches back to the main fiber.
     *
     * ### Workflow:
     * 1. Retrieve the current fiber using `GetCurrentFiber`.
     * 2. Check if the current fiber is the main fiber:
     *    - If true, the function ignores the release operation and returns immediately, as the main
     *      fiber cannot be paused or queued.
     * 3. Lock the `s_readyMutex` to ensure thread-safe access to the `s_readyFibers` queue.
     * 4. Add the current fiber to the `s_readyFibers` queue.
     * 5. Switch execution back to the main fiber using `SwitchToFiber`.
     *
     * @note This function differs from `yieldFiber` in that the fiber is immediately queued for
     *       re-execution without requiring an explicit call to resume it (e.g., via `unYieldFiber`).
     *
     * @remarks This method is particularly useful when a fiber has completed some work and needs
     *          to temporarily pause to allow other fibers or tasks to execute before resuming.
     *
     * @warning If called from the main fiber, the function exits silently, as the main fiber cannot
     *          participate in cooperative multitasking.
     */
    static void release() {
        SwCoreApplication* app = instance(false);
        LPVOID current = GetCurrentFiber();
        if (current == app->mainFiber) {
            // Not in the event loop; ignore yielding
            return;
        }
        {
            std::lock_guard<std::mutex> lock(s_readyMutex);
            s_readyFibers.push(current);
        }

        // Retour à la fibre principale
        SwitchToFiber(app->mainFiber);
    }

    /**
     * @brief Pauses the execution of the current fiber and associates it with the given ID.
     *
     * This function is used to yield (pause) the execution of the current fiber by storing it in
     * the `s_yieldedFibers` map, using the provided `id` as the key. Once yielded, the function
     * switches execution back to the main fiber, allowing other fibers or tasks to run.
     *
     * ### Workflow:
     * 1. Retrieve the current fiber using `GetCurrentFiber`.
     * 2. If the current fiber is the main fiber, yielding is ignored since it is not part of the
     *    cooperative multitasking system.
     * 3. If the current fiber is not the main fiber:
     *    - Lock the `s_yieldMutex` to ensure thread-safe access to the `s_yieldedFibers` map.
     *    - Store the current fiber in the map with the specified `id` as the key.
     * 4. Switch execution back to the main fiber using `SwitchToFiber`.
     *
     * @param id The unique identifier to associate with the yielded fiber.
     *
     * @note Yielding allows a fiber to pause its execution until explicitly resumed by calling
     *       `unYieldFiber` with the same `id`.
     *
     * @warning If the main fiber attempts to yield itself, the function will exit without performing
     *          any operation, as yielding is only valid for non-main fibers.
     *
     * @remarks This function is critical for implementing cooperative multitasking, where fibers
     *          voluntarily yield control to enable other fibers or tasks to execute.
     */
    static void yieldFiber(int id) {
        SwCoreApplication* app = instance(false);
        LPVOID current = GetCurrentFiber();
        if (current == app->mainFiber) {
            // Not in the event loop; ignore yielding
            return;
        }
        // Store the current fiber in the yielded fibers map
        {
            std::lock_guard<std::mutex> lock(s_yieldMutex);
            s_yieldedFibers[id] = current;
        }

        // Switch execution back to the main fiber
        SwitchToFiber(app->mainFiber);
    }

    /**
     * @brief Restores a previously yielded fiber to the ready queue for execution.
     *
     * This function handles the transition of a fiber from the "yielded" state back to the "ready"
     * state. It locates the fiber associated with the specified identifier (`id`) in the
     * `s_yieldedFibers` map. If the fiber is found, it is removed from the yielded fibers map and
     * added to the `s_readyFibers` queue. Fibers in the ready queue will be resumed by the main
     * fiber during the event loop execution.
     *
     * ### Workflow:
     * 1. Lock the `s_yieldMutex` to safely access the `s_yieldedFibers` map.
     * 2. Search for the fiber using the provided `id`.
     * 3. If the fiber is found:
     *    - Extract and remove it from the `s_yieldedFibers` map.
     *    - Store the fiber pointer in a local variable.
     * 4. Lock the `s_readyMutex` to safely push the fiber into the `s_readyFibers` queue.
     * 5. If the fiber is not found in `s_yieldedFibers`, no operation is performed.
     *
     * @param id The unique identifier of the fiber to un-yield.
     *
     * @note This function ensures thread safety when modifying shared data structures by using
     *       `std::lock_guard` for both `s_yieldMutex` and `s_readyMutex`.
     *
     * @warning If the `id` provided does not correspond to any fiber in `s_yieldedFibers`, the
     *          function will exit silently without performing any operation.
     *
     * @remarks This function is designed to facilitate cooperative multitasking by enabling
     *          previously paused fibers to rejoin the execution flow. It is typically called
     *          when an external event signals that the fiber should continue its execution.
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
     * @brief Entry point for a newly created fiber.
     *
     * Executes the function passed as a parameter to the fiber and
     * switches back to the main fiber upon completion.
     *
     * @param lpParameter Pointer to the function to execute within the fiber.
     */
    static VOID WINAPI FiberProc(LPVOID lpParameter) {
        std::function<void()>* callback = reinterpret_cast<std::function<void()>*>(lpParameter);

        // Execute the callback function. Once we return from this function call,
        // we know that the callback has fully completed its execution, including
        // any yields that might have occurred during its lifetime.
        (*callback)();

        // At this point, the callback is guaranteed to have finished, so we can
        // safely delete the allocated std::function. By managing the lifetime
        // here, we ensure that the callback's memory is only freed after it
        // truly completes, preventing the risk of double deletes or accessing
        // deallocated memory if it had simply yielded instead of fully returning.
        delete callback;

        // Finally, return control to the main fiber.
        SwitchToFiber(instance(false)->mainFiber);
    }


    /**
     * @brief Executes a given event (function) within a fiber and manages its lifecycle.
     *
     * This function creates a new fiber to execute the provided event (function), switches
     * to the fiber to run it, and ensures proper cleanup of the fiber after execution.
     *
     * Steps:
     * 1. Allocates a new fiber with `CreateFiber` and passes the event function as a parameter.
     * 2. Switches to the newly created fiber using `SwitchToFiber` to execute the event.
     * 3. After execution or yielding, the function ensures the fiber is properly deleted
     *    using `deleteFiberIfNeeded`.
     *
     * @param event The function to execute within the fiber.
     *
     * @note If fiber creation fails, the event is executed synchronously in the current thread,
     *       and the function logs an error message.
     *
     * @warning The caller must ensure that the event does not hold any references to objects
     *          that may become invalid during fiber execution.
     *
     * @exception None. Errors during fiber creation are logged but do not throw exceptions.
     *
     * @remarks Fibers are a cooperative multitasking construct, so the event is expected to
     *          yield or complete its execution without blocking other operations indefinitely.
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
     * @brief Resumes fibers that are ready to run.
     *
     * This function processes the queue of ready fibers (`s_readyFibers`) and resumes their execution
     * one by one using `SwitchToFiber`. Each fiber is executed until it either completes or yields
     * again. The function ensures that no fiber is resumed more than once during the same cycle.
     *
     * The function follows these steps:
     * 1. Retrieves a fiber from the `s_readyFibers` queue.
     * 2. Checks if the fiber has already been resumed during the current cycle using `resumedThisCycle`.
     *    - If it has, the fiber is requeued, and the function exits to avoid infinite loops.
     * 3. If the fiber has not been resumed, it is marked as resumed and executed using `SwitchToFiber`.
     * 4. After execution, checks if the fiber needs to be deleted using `deleteFiberIfNeeded`.
     *
     * @note This function uses thread safety mechanisms (mutex locks) to ensure consistent access
     *       to the `s_readyFibers` queue.
     *
     * @note A fiber that has already been resumed in the current cycle is requeued for execution
     *       in subsequent cycles of the event loop.
     */
    void resumeReadyFibers() {
        std::unordered_set<LPVOID> resumedThisCycle;

        while (true) {
            LPVOID fiber = nullptr;
            {
                std::lock_guard<std::mutex> lock(s_readyMutex);
                if (!s_readyFibers.empty()) {
                    fiber = s_readyFibers.front();
                    s_readyFibers.pop();
                } else {
                    break; // No more fibers to resume
                }
            }

            // Check if the fiber has already been resumed during this cycle
            if (resumedThisCycle.find(fiber) != resumedThisCycle.end()) {
                // Fiber has already been executed this cycle, requeue it for later
                {
                    std::lock_guard<std::mutex> lock(s_readyMutex);
                    s_readyFibers.push(fiber);
                }
                // Exit this cycle; the fiber will be retried in the next processEvent() call.
                break;
            }

            resumedThisCycle.insert(fiber);

            if (fiber) {
                // Resume the fiber
                SwitchToFiber(fiber);
                // Back here after the fiber finishes or yields again
                // Check if the fiber needs to be deleted
                deleteFiberIfNeeded(fiber);
            }
        }
    }


    /**
     * @brief Deletes a fiber if it is no longer in use.
     *
     * This function checks whether a given fiber is still being used, either as a yielded fiber
     * (waiting to be resumed) or as a ready fiber (waiting to be executed). If the fiber is neither
     * yielded nor ready, it is safely deleted.
     *
     * The function performs the following steps:
     * 1. Checks if the fiber is present in the `s_yieldedFibers` map (yielded fibers).
     * 2. If not yielded, checks if the fiber is present in the `s_readyFibers` queue (ready fibers).
     * 3. If the fiber is neither yielded nor ready, calls `DeleteFiber` to release its resources.
     *
     * @param fiber Pointer to the fiber to check and potentially delete.
     *
     * @note The function ensures thread safety by using mutex locks when accessing the shared
     *       `s_yieldedFibers` and `s_readyFibers` data structures.
     * @note If the fiber is still in use, it is not deleted.
     */
    void deleteFiberIfNeeded(void* fiber) {
        bool isYielded = false;
        bool isReady = false;

        // Check if the fiber is in the yielded fibers map
        {
            std::lock_guard<std::mutex> lock(s_yieldMutex);
            for (auto &kv : s_yieldedFibers) {
                if (kv.second == fiber) {
                    isYielded = true;
                    break;
                }
            }
        }

        // Check if the fiber is in the ready fibers queue
        if (!isYielded) {
            std::lock_guard<std::mutex> lock(s_readyMutex);

            // Iterate through the ready fibers queue without modifying it
            std::queue<LPVOID> temp = s_readyFibers;
            while (!temp.empty()) {
                LPVOID f = temp.front();
                temp.pop();
                if (f == fiber) {
                    isReady = true;
                    break;
                }
            }
        }

        if (!isYielded && !isReady) {
            // If the fiber is neither yielded nor ready, delete it
            DeleteFiber(fiber);
        }
    }

    /**
     * @brief Processes timers, executing callbacks for ready timers, and calculates the time
     *        until the next timer is ready.
     *
     * This function iterates through a list of timers, executes the callbacks for those that are ready,
     * and determines the minimum time remaining until the next timer becomes ready.
     *
     * @param timers Map of timers where the key is the timer ID and the value is a pointer to the timer object.
     * @return The time in microseconds until the next timer is ready, or the maximum possible integer
     *         if no timers are active.
     */
    int processTimers() {
        int minTimeUntilNext = (std::numeric_limits<int>::max)();

        for (auto it = timers.begin(); it != timers.end(); ) {
            _T* currentTimer = it->second;
            bool isSingleShot = currentTimer->singleShot;

            if (currentTimer->isReady()) {
                // Timer is ready to execute
                if (isSingleShot) {
                    // Remove and delete single-shot timer after execution
                    _T* toDelete = currentTimer;
                    it = timers.erase(it);  // Advance the iterator after removal

                    // Execute timer callback in a fiber
                    std::function<void()> timerEvent = [toDelete]() {
                        toDelete->execute();
                        delete toDelete; // Delete the timer after execution
                    };
                    runEventInFiber(timerEvent);
                } else {
                    // Execute recurring timer without removing it
                    std::function<void()> timerEvent = [currentTimer]() {
                        currentTimer->execute();
                    };
                    runEventInFiber(timerEvent);

                    ++it; // Advance the iterator
                }
            } else {
                // Calculate time until the next timer is ready
                int timeUntilNext = currentTimer->timeUntilReady();
                if (timeUntilNext < minTimeUntilNext) {
                    minTimeUntilNext = timeUntilNext;
                }
                ++it;
            }
        }

        return minTimeUntilNext;
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

/**
 * @brief Handles console control events.
 *
 * This function is called when a control signal is sent to the console.
 * It gracefully handles various events such as closure, shutdown,
 * or an interruption (Ctrl+C).
 *
 * @param ctrlType The type of control event sent to the console. Possible values are:
 *                 - `CTRL_CLOSE_EVENT`: Console close event.
 *                 - `CTRL_C_EVENT`: Ctrl+C signal.
 *                 - `CTRL_BREAK_EVENT`: Ctrl+Break signal.
 *                 - `CTRL_LOGOFF_EVENT`: User logoff.
 *                 - `CTRL_SHUTDOWN_EVENT`: System shutdown.
 * @return BOOL Returns `TRUE` if the event was successfully handled, otherwise `FALSE`.
 */
static BOOL WINAPI ConsoleHandler(DWORD ctrlType) {
    switch (ctrlType) {
    case CTRL_CLOSE_EVENT:
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    {
        // Arrêt propre de l'application
        SwCoreApplication* app = SwCoreApplication::instance(false);
        if(app) {
            // Appeler quit pour stopper la boucle d'événements proprement
            app->quit();
        }
        // On retourne TRUE pour indiquer qu'on a géré l'événement
        return TRUE;
    }
    default:
        return FALSE;
    }
}

// Declaration of static members
std::mutex SwCoreApplication::s_yieldMutex;
std::map<int, LPVOID> SwCoreApplication::s_yieldedFibers;
std::mutex SwCoreApplication::s_readyMutex;
std::queue<LPVOID> SwCoreApplication::s_readyFibers;
