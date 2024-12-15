// ConsoleApplication.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "PushButton.h"
#include "SwGuiApplication.h"
#include "SwTimer.h"
#include "SwEventLoop.h"
#include "SwProcess.h"
#include "SwMainWindow.h"
#include "LineEdit.h"
#include "Label.h"
#include "SwString.h"

int main() {	
    SwGuiApplication app;
    SwMainWindow mainWindow;
    mainWindow.show();

    SwObject::connect(&mainWindow, SIGNAL(resized), [&](int width, int height) {
        std::cout << "Fenêtre -------> redimensionnée: " << width << "x" << height << std::endl;
    });

    // Taille de la grille et position de départ
    int startX = 50;
    int startY = 100;
    int spacingX = 350;  // Augmenter l'espace entre les colonnes
    int spacingY = 150;  // Espace entre les lignes
    int labelWidth = 100;
    int labelHeight = 30;
    int buttonWidth = 100;
    int buttonHeight = 30;
    int lineEditWidth = 180;  // Réduire la largeur des LineEdit
    int lineEditHeight = 30;

    // Boucle pour créer les widgets (2x2 grille)
    for (int i = 0; i < 4; ++i) {
        // Calcul de la position en grille
        int row = i / 2;  // 0 ou 1 pour la ligne
        int col = i % 2;  // 0 ou 1 pour la colonne

        int xPos = startX + col * spacingX;
        int yPos = startY + row * spacingY;

        // Création des labels
        Label* label = new Label(&mainWindow);
        label->setText(SwString("Label %1:").arg(SwString::number(i)));
        label->move(xPos, yPos);
        label->resize(labelWidth, labelHeight);

        // Création des LineEdit
        SwLineEdit* lineEdit = new SwLineEdit("Entrez votre message ici...", &mainWindow);
        lineEdit->move(xPos, yPos + labelHeight + 10);  // Positionner sous le label
        lineEdit->resize(lineEditWidth, lineEditHeight);
        lineEdit->setEchoMode(EchoModeEnum::NormalEcho);

         // Création des boutons
         PushButton* button = new PushButton(SwString("Button %1").arg(SwString::number(i)), &mainWindow);
         button->setCursor(CursorType::Hand);
         button->move(xPos + lineEditWidth + 20, yPos + labelHeight + 10);
         button->resize(buttonWidth, buttonHeight);

         SwObject::connect(button, SIGNAL(clicked), std::function<void()>([&]() {
             std::cout << "*********Button Clicked**********" << std::endl;

        }));
    }

    return app.exec();
}
