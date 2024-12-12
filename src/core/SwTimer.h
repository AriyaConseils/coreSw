#pragma once

#include "Object.h"
#include "SwCoreApplication.h"

/**
 * @class Timer
 * @brief Provides a timer implementation for periodic execution of tasks.
 */
class SwTimer : public Object
{

public:

    /**
     * @brief Constructs a SwTimer object.
     *
     * @param ms The interval in milliseconds for the timer (default is 1000 ms).
     * @param parent The parent object for the timer.
     */
    SwTimer(int ms = 1000, Object *parent = nullptr)
        : Object(parent), interval(ms*1000), running(false), timerId(-1) {
    }

    /**
     * @brief Destructor to clean up the SwTimer resources.
     */
    virtual ~SwTimer() {
        stop();
        if (timerId != -1) {
            SwCoreApplication::instance()->removeTimer(timerId);
        }
    }

    /**
     * @brief Sets the interval for the timer.
     *
     * @param ms The interval in milliseconds.
     */
    void setInterval(int ms) {
        if (!running) {
            interval = ms*1000;
        }
    }

    /**
     * @brief Starts the timer.
     */
    void start() {
        if (!running) {
            running = true;
            timerId = SwCoreApplication::instance()->addTimer([this]() {
                if (running) {
                    emit timeout();
                }
                }, interval);
        }
    }

    /**
     * @brief Stops the timer.
     */
    void stop() {
        if (running) {
            running = false;
            if (timerId != -1) {
                int currentTimerId = timerId;
                SwCoreApplication::instance()->postEvent([currentTimerId]() {
                    //pour pouvoir fait un stop alors que je suis deja dans un timer
                    //programmer un event
                    SwCoreApplication::instance()->removeTimer(currentTimerId);
                });
                timerId = -1;
            }
        }
    }

    /**
     * @brief Creates a single-shot timer that executes a callback after a specified delay.
     *
     * @param ms The delay in milliseconds.
     * @param callback The callback function to execute.
     */
    static void singleShot(int ms, std::function<void()> callback) {
        SwTimer* tempTimer = new SwTimer(ms);

        tempTimer->connect(tempTimer, SIGNAL(timeout), std::function<void(void)>([callback, tempTimer]() {
            callback();
            tempTimer->stop();
            tempTimer->deleteLater();
        }));
        
        tempTimer->start();
    }

signals:
    /**
     * @brief Signal emitted when the timer interval elapses.
     */
    DECLARE_SIGNAL(timeout)

private:
    int interval;    ///< The interval in microseconds for the timer.
    bool running;    ///< Indicates if the timer is currently running.
    int timerId;     ///< The unique identifier for the timer in the SwCoreApplication instance.
};
