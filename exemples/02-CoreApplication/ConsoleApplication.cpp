// ConsoleApplication.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "PushButton.h"
#include "SwCoreApplication.h"
#include "SwTimer.h"
#include "SwEventLoop.h"
#include "SwProcess.h"
#include "SwString.h"
#include "SwJsonDocument.h" // Assurez-vous que ce fichier est correctement configur�

int main() {
    // Cha�ne JSON initiale
    SwAny::registerMetaType<SwJsonDocument>();

    std::string jsonString = R"({
        "person": {
            "name": "John Doe",
            "address": {
                "city": "Toulouse",
                "postalCode": 31000
            },
            "age": 30
        }
    })";

        // Cr�er un document JSON � partir de la cha�ne
        std::string errorMessage;
        SwJsonDocument doc = SwJsonDocument::fromJson(jsonString, errorMessage);

        // Acc�der � la racine
        SwJsonValue root = doc.toJsonValue();

        // Naviguer et acc�der aux valeurs
        SwJsonValue& name = doc.find("person/name");
        SwJsonValue& city = doc.find("person/address/city");
        SwJsonValue& postalCode = doc.find("person/address/postalCode");

        // Afficher les valeurs actuelles
        std::cout << "Name: " << name.toString() << std::endl;
        std::cout << "City: " << city.toString() << std::endl;
        std::cout << "Postal Code: " << postalCode.toInt() << std::endl;

        // Modifier des valeurs existantes
        doc.find("person/address/city", true) = "Paris";

        // Ajouter une nouvelle cl� avec sa valeur
        doc.find("person/address/country", true) = "France";

        // Convertir le document JSON en une cha�ne mise � jour
        std::string updatedJson = doc.toJson(SwJsonDocument::JsonFormat::Pretty);

        SwAny docAnyPtr = SwAny::fromVoidPtr(reinterpret_cast<void *>(&doc), "class SwJsonDocument");
        std::cout << "SwAny JSON from dynamic void pointer copy:\n" << docAnyPtr.get<SwJsonDocument>().toJson(SwJsonDocument::JsonFormat::Pretty) << std::endl;

        SwAny docAny = SwAny::from(doc);
        std::cout << "SwAny JSON from regular copy:\n" << docAny.get<SwJsonDocument>().toJson(SwJsonDocument::JsonFormat::Pretty) << std::endl;

    return 0;
}
