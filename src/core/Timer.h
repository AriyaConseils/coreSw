#pragma once

#include "Object.h"
#include "CoreApplication.h"

class Timer : public Object 
{

public:
    Timer(int ms = 1000, Object *parent = nullptr)
        : interval(ms*1000), Object(parent), running(false), timerId(-1) {
    }

    // Destructeur pour nettoyer correctement
    virtual ~Timer() {
        stop();
        if (timerId != -1) {
            CoreApplication::instance()->removeTimer(timerId);
        }
        std::cout << "Timer détruit et nettoyé." << std::endl;
    }

    // Setter pour l'intervalle
    void setInterval(int ms) {
        if (!running) {
            interval = ms*1000;
        }
    }

    // Méthode start pour démarrer le timer
    void start() {
        if (!running) {
            running = true;
            timerId = CoreApplication::instance()->addTimer([this]() {
                if (running) {
                    emit timeout();
                }
                }, interval);
            std::cout << "Timer démarré avec un intervalle de " << interval/1000 << " ms." << std::endl;
        }
    }

    // Méthode stop pour arrêter le timer
    void stop() {
        if (running) {
            running = false;
            if (timerId != -1) {
                int currentTimerId = timerId;
                CoreApplication::instance()->postEvent([currentTimerId]() {
                    //pour pouvoir fait un stop alors que je suis deja dans un timer
                    //programmer un event
                    CoreApplication::instance()->removeTimer(currentTimerId);
                });
                timerId = -1;
            }
            std::cout << "Timer arrêté." << std::endl;
        }
    }

    static void singleShot(int ms, std::function<void()> callback) {
        Timer* tempTimer = new Timer(ms);

        tempTimer->connect(tempTimer, "timeout", std::function<void(void)>([callback, tempTimer]() {
            callback();
            tempTimer->stop();
            tempTimer->deleteLater();
        }));
        
        tempTimer->start();
    }

signals:
    DECLARE_SIGNAL(timeout)

private:
    int interval;
    bool running;
    int timerId;
};
