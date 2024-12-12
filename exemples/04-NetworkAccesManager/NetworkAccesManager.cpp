#define _WINSOCKAPI_

#include "SwCoreApplication.h"
#include "SwNetworkAccessManager.h"

#include <iostream>


int main(int argc, char* argv[]) {
    SwCoreApplication app(argc, argv);

    SwNetworkAccessManager nam;
    nam.setRawHeader("Authorization", "Bearer my_token");
    nam.setRawHeader("Conefzetent-Type", "application/json");


    Object::connect(&nam, SIGNAL(finished), [](const SwString& result) {
        std::cout << "R�ponse re�ue:\n" << result.toStdString() << std::endl;
    });

    Object::connect(&nam, SIGNAL(errorOccurred), [](int err) {
        std::cerr << "Erreur r�seau: " << err << std::endl;
    });

    nam.get("http://app.swiiz.io/home/login/"); // lance une requ�te GET sur example.com

    return app.exec();
}

