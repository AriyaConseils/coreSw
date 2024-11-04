// ConsoleApplication.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "PushButton.h"
#include "GuiApplication.h"
#include "Timer.h"
#include "EventLoop.h"
#include "Process.h"
#include "MainWindow.h"
#include "LineEdit.h"
#include "Label.h"

int main() {
	bool wait = true;
	
	//while(wait);
	
    GuiApplication app;
    MainWindow mainWindow;
    mainWindow.show();




    Object::connect(&mainWindow, SIGNAL(resized), std::function<void(int, int)>([&](int width, int height) {
        std::cout << "Fen�tre -------> redimensionn�e: " << width << "x" << height << std::endl;
    }));



    // Taille de la grille et position de d�part
    int startX = 50;
    int startY = 100;
    int spacingX = 350;  // Augmenter l'espace entre les colonnes
    int spacingY = 150;  // Espace entre les lignes
    int labelWidth = 100;
    int labelHeight = 30;
    int buttonWidth = 100;
    int buttonHeight = 30;
    int lineEditWidth = 180;  // R�duire la largeur des LineEdit
    int lineEditHeight = 30;

    // Boucle pour cr�er les widgets (2x2 grille)
    for (int i = 0; i < 4; ++i) {
        // Calcul de la position en grille
        int row = i / 2;  // 0 ou 1 pour la ligne
        int col = i % 2;  // 0 ou 1 pour la colonne

        int xPos = startX + col * spacingX;
        int yPos = startY + row * spacingY;

        // Cr�ation des labels
        Label* label = new Label(&mainWindow);
        label->setText(L"Label " + std::to_wstring(i) + L":");
        label->move(xPos, yPos);
        label->resize(labelWidth, labelHeight);

        // Cr�ation des LineEdit
        LineEdit* lineEdit = new LineEdit(L"Entrez votre message ici...", &mainWindow);
        lineEdit->move(xPos, yPos + labelHeight + 10);  // Positionner sous le label
        lineEdit->resize(lineEditWidth, lineEditHeight);
        lineEdit->setEchoMode(EchoModeEnum::NormalEcho);

        // Cr�ation des boutons
        PushButton* button = new PushButton(L"Button " + std::to_wstring(i), &mainWindow);
        button->setCursor(CursorType::Hand);
        button->move(xPos + lineEditWidth + 20, yPos + labelHeight + 10);  // � droite du LineEdit
        button->resize(buttonWidth, buttonHeight);


        EventLoop myloop;
        //Object::connect(lineEdit, SIGNAL(Text), &myloop, &EventLoop::quit);
        Object::connect(lineEdit, SIGNAL(Text), std::function<void(std::wstring)>([&](std::wstring texte) {
            myloop.quit();
            }));


        Object::connect(button, SIGNAL(clicked), std::function<void()>([&]() {
            std::cout << "*********Loop Enter**********" << std::endl;
            myloop.exec();
            std::cout << "*********Loop Quit**********" << std::endl;
        }));
    }

    return app.exec();
}
