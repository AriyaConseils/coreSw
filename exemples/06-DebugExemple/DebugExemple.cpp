#define _WINSOCKAPI_

#include <SwCoreApplication.h>
#include <SwDebug.h>
#include <SwTimer.h>

int main(int argc, char *argv[]) {
    // Création de l'application principale
    SwCoreApplication app(argc, argv);

    SwDebug::setAppName("MySuperApp");
    SwDebug::setVersion("1.2.3");

    // Tentative de connexion sur le host et port
    SwDebug::instance().connectToHostAndIdentify("127.0.0.1", 12345);

    swDebug() << "Ceci est un message de debug avec valeur: " << 42;
    swWarning() << "Attention, quelque chose n'est pas optimal";
    swError() << "Erreur critique: valeur invalide.";

    // Création d'un timer
    SwTimer timer;
    timer.setInterval(2000); // Intervalle de 2000 ms (2 secondes)

    // Connexion du signal timeout à une lambda pour afficher des messages de debug
    Object::connect(&timer, SIGNAL(timeout), &app, [&]() {
        static int counter = 0;
        swDebug() << "Message périodique numéro: " << ++counter;
    });

    // Démarrage du timer
    timer.start();

    return app.exec();
}
