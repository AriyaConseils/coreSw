#include <iostream>
#include <SwCoreApplication.h>
#include <SwProcess.h>
#include <SwString.h>

class MyProcessHandler : public SwObject {
public:
    // void onReadyReadStandardOutput(SwProcess* process) {
    //     SwString output = process->readAllStandardOutput();
    //     std::cout << "Sortie du processus : " << output.toStdString() << std::endl;
    // }

    // void onReadyReadStandardError(SwProcess* process) {
    //     SwString errorOutput = process->readAllStandardError();
    //     std::cerr << "Erreur du processus : " << errorOutput.toStdString() << std::endl;
    // }

    // void onProcessFinished(int exitCode, SwProcess::ExitStatus exitStatus) {
    //     std::cout << "Le processus s'est terminé avec le code : " << exitCode
    //               << " et le statut : " << (exitStatus == SwProcess::NormalExit ? "Normal" : "Crash") << std::endl;
    //     // Quitter l'application après la fin du processus
    //     SwCoreApplication::instance()->quit();
    // }
};

int main(int argc, char** argv) {
    // Instancier l'application
    SwCoreApplication app(argc, argv);

    // Instancier le gestionnaire et le processus
    MyProcessHandler handler;
    SwProcess process;

    SwObject::connect(&process, SIGNAL(readyReadStdOut), std::function<void()>([&]() {
        SwString output = process.read();
        std::cout << "OUTPUT: " << output <<  std::endl;
    }));

    // Connecte les signaux pour stderr
    SwObject::connect(&process, SIGNAL(readyReadStdErr), std::function<void()>([&]() {
        SwString error = process.readStdErr();
        std::cerr << "ERROR: " << error <<  std::endl;

    }));

    // Connecte la fin du processus
    SwObject::connect(&process, SIGNAL(processFinished), std::function<void()>([&]() {
        std::cout << "********FINISH*******" << std::endl;
    }));

    // Connecte la terminaison du processus
    SwObject::connect(&process, SIGNAL(processTerminated), std::function<void(int)>([&](int exitCode) {
        std::cout << "********TERMINATE*******" << std::endl;
    }));


    // Définir le programme et ses arguments
    SwString program = "tar"; // Sur Windows, utilisez "dir" ou un programme équivalent
    SwStringList arguments;
    arguments.append("-h");


    // Démarrer le processus
    if (!process.start(program, arguments)) {
        std::cerr << "Impossible de démarrer le processus." << std::endl;
        return 1;
    }

    // Exécuter la boucle d'événements
    return app.exec();
}
