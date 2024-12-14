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

#include "SwTimer.h"
#include "SwCoreApplication.h"
#include <windows.h>
/**
 * @class SwEventLoop
 * @brief A local event loop mechanism built on top of the SwCoreApplication fiber system.
 *
 * This class provides a local event loop similar to QEventLoop. It allows you to start a nested
 * event loop and block execution until `quit()` or `exit()` is called, at which point it returns
 * control back to the caller of `exec()`.
 *
 * @details
 * The SwEventLoop relies on the underlying fiber-based architecture of SwCoreApplication.
 * When `exec()` is called, it suspends the current fiber using `yieldFiber()` and waits until
 * `unYieldFiber()` is called elsewhere in the code (typically via `quit()` or `exit()`), causing
 * the event loop to resume and exit. This makes it possible to implement nested event loops,
 * modal dialogs, or short-lived waiting loops without blocking the entire application.
 *
 * ### Example Usage
 *
 * ```cpp
 * SwEventLoop loop;
 *
 * // Some code that posts an event to quit the loop later...
 * // For instance:
 * // SwCoreApplication::instance()->postEvent([&]() {
 * //     loop.quit();
 * // });
 *
 * // This call will block here until quit() or exit() is called
 * int returnCode = loop.exec();
 *
 * // At this point, the loop has stopped running and we have `returnCode`.
 * ```
 *
 * ### Key Methods
 * - `exec()`: Starts the event loop. Execution will be suspended until `quit()` or `exit()` is called.
 * - `quit()`: Stops the event loop and returns control to the caller of `exec()`.
 * - `exit(int code)`: Like `quit()`, but also sets an exit code that can be retrieved from `exec()`.
 * - `isRuning()`: Checks if the event loop is currently running.
 *
 * ### Internal Details
 * - `exec()` obtains a unique identifier for the event loop and yields the current fiber, effectively
 *   pausing execution. The fiber will only resume once `unYieldFiber()` is called with the corresponding
 *   identifier, which happens in `quit()` or `exit()`.
 * - When `quit()` is invoked, the event loop sets `running_` to `false` and triggers `unYieldFiber(id_)`,
 *   causing `exec()` to return.
 * - `exit(int code)` is similar to `quit()` but also sets an exit code, which `exec()` returns.
 *
 * @note Ensure that SwCoreApplication and its fiber system are properly initialized before using SwEventLoop.
 *
 * @author
 * Eymeric O'Neill
 * Contact: eymeric.oneill@gmail.com, +33 6 52 83 83 31
 *
 * @since 1.0
 */
class SwEventLoop : public Object {
public:
    /**
     * @brief Constructs a new SwEventLoop.
     *
     * The event loop is not running initially. You can start it by calling `exec()`.
     */
    SwEventLoop()
        : running_(false),
        exitCode(0)
    {
        // Assumes that mainFiber is accessible and initialized in SwCoreApplication.
    }

    /**
     * @brief Destroys the SwEventLoop.
     *
     * If the event loop is running, it will be stopped by calling `quit()` before destruction.
     */
    ~SwEventLoop() {
        quit();
    }

    /**
     * @brief Starts the event loop.
     *
     * This method suspends the current fiber and waits until `quit()` or `exit()` is called.
     * Once resumed, `exec()` returns the exit code set by `exit()` or 0 if `quit()` was used.
     *
     * @return The exit code set by `exit()` or 0 if `quit()` was called.
     */
    int exec(int delay = 0) {
        if(running_) return -1; // return if already runing
        if(delay) SwTimer::singleShot(delay, this, &SwEventLoop::quit); // auto wake up if delay
        exitCode = 0;
        running_ = true;
        static int _id = 0;
        id_ = _id++;
        SwCoreApplication::instance()->yieldFiber(id_);
        // Control will return here after `unYieldFiber()` is called for `id_`.
        return exitCode;
    }

    /**
     * @brief Stops the event loop without an exit code.
     *
     * Calling `quit()` will cause `exec()` to return 0. If the event loop is not currently running,
     * this function has no effect.
     */
    void quit() {
        if (running_) {
            running_ = false;
            SwCoreApplication::instance()->unYieldFiber(id_);
        }
    }

    /**
     * @brief Stops the event loop and sets an exit code.
     *
     * Similar to `quit()` but allows setting a custom exit code. `exec()` will return this code
     * when the event loop resumes.
     *
     * @param code The exit code to return from `exec()`.
     */
    void exit(int code = 0) {
        exitCode = code;
        quit();
    }

    /**
     * @brief Checks if the event loop is currently running.
     *
     * @return `true` if running, `false` otherwise.
     */
    bool isRuning(){
        return running_;
    }

private:
    bool running_;  ///< Indicates whether the event loop is currently running.
    int id_;        ///< Unique identifier for this event loop instance.
    int exitCode;   ///< Exit code returned by `exec()`.
};
