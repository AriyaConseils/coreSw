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

#include "SwCommandLineOption.h"
#include "SwCoreApplication.h"
#include "SwString.h"
#include "SwList.h"
#include "SwMap.h"

class SwCommandLineParser {
public:
    SwCommandLineParser()
        : helpOptionAdded(false), appDescription("") {}

    // Définir la description de l'application
    void setApplicationDescription(const SwString& description) {
        appDescription = description;
    }

    // Ajouter une option au parser
    void addOption(const SwCommandLineOption& option) {
        options.append(option);
    }

    // Ajouter l'option d'aide (comme --help ou -h)
    void addHelpOption() {
        if (!helpOptionAdded) {
            SwCommandLineOption help({"h", "help"}, "Displays this help message.");
            addOption(help);
            helpOptionAdded = true;
        }
    }

    // Traiter les arguments à partir de SwCoreApplication
    bool process(const SwCoreApplication& app) {
        parsedOptions.clear();
        positionalArguments.clear();

        for (const auto& option : options) {
            // Traiter les options longues
            for (const auto& name : option.getNames()) {
                if (app.hasArgument(name)) {
                    SwString value = app.getArgument(name);
                    if (value.isEmpty() && option.isValueRequired()) {
                        errorMessage = SwString("Option '--") + name + SwString("' requires a value.");
                        return false;
                    }
                    parsedOptions[name] = value;
                }
            }
        }

        // Récupérer les arguments positionnels
        SwList<SwString> allArguments = app.getPositionalArguments();
        for (const auto& arg : allArguments) {
            if (!arg.startsWith("-")) {
                positionalArguments.append(arg);
            }
        }

        // Vérifier si l'option d'aide est activée
        if (isSet("help")) {
            std::cout << generateHelpText().toStdString() << "\n";
            std::exit(0);
        }

        return true;
    }

    // Vérifie si une option a été spécifiée
    bool isSet(const SwString& key) const {
        for (const auto& option : options) {
            if (option.getNames().contains(key)) {
                for (const auto& name : option.getNames()) {
                    if (parsedOptions.contains(name)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // Récupère la valeur d'une option
    SwString value(const SwString& key, const SwString& defaultValue = "") const {
        for (const auto& option : options) {
            if (option.getNames().contains(key)) {
                for (const auto& name : option.getNames()) {
                    if (parsedOptions.contains(name)) {
                        return parsedOptions[name]; // Retourne la première valeur trouvée
                    }
                }
            }
        }
        return defaultValue; // Retourne la valeur par défaut si aucun nom ne correspond
    }


    // Récupère les arguments positionnels
    SwList<SwString> positionalArgumentsList() const {
        return positionalArguments;
    }

    // Génère le texte d'aide
    SwString generateHelpText() const {
        SwString result;

        if (!appDescription.isEmpty()) {
            result += appDescription + "\n\n";
        }

        result += "Options:\n";

        for (const auto& option : options) {
            SwString names = SwString("--") + option.getNames().first();
            if (option.getNames().size() > 1) {
                names += SwString(", -") + option.getNames().last();
            }
            if (!option.getValueName().isEmpty()) {
                names += SwString(" <") + option.getValueName() + ">";
            }

            result += SwString("  ") + names + SwString("\n    ") + option.getDescription() + SwString("\n");
        }

        return result;
    }

    // Récupérer le message d'erreur (en cas d'échec du parsing)
    SwString error() const {
        return errorMessage;
    }

private:
    SwList<SwCommandLineOption> options;     // Liste des options possibles
    SwMap<SwString, SwString> parsedOptions; // Options parsées
    SwList<SwString> positionalArguments;   // Arguments positionnels
    SwString appDescription;                // Description de l'application
    SwString errorMessage;                  // Message d'erreur en cas de problème
    bool helpOptionAdded;                   // Indique si l'option d'aide a été ajoutée
};
