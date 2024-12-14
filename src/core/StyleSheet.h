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

#include <string>
#include <sstream>      // Pour std::istringstream
#include <map>          // Pour std::map
#include <vector>       // Pour std::vector
#include <algorithm>    // Pour std::find
#include <cctype>       // Pour std::isspace
#include <stdexcept>    // Pour std::invalid_argument
#include <sstream>      // Pour std::stringstream
#include <stdexcept>    // Pour std::invalid_argument
#include <windows.h>    // Pour l'utilisation de COLORREF et RGB
#include <map>          // Pour stocker les noms de couleurs



class StyleSheet {
public:
    // Un dictionnaire qui mappe des sélecteurs CSS à des styles (propriétés)
    std::map<std::string, std::map<std::string, std::string>> styles;

    StyleSheet()
    {
        colorNames = {
               {"red", RGB(255, 0, 0)},
               {"green", RGB(0, 255, 0)},
               {"blue", RGB(0, 0, 255)},
               {"yellow", RGB(255, 255, 0)},
               {"black", RGB(0, 0, 0)},
               {"white", RGB(255, 255, 255)},
               {"gray", RGB(128, 128, 128)},
               {"cyan", RGB(0, 255, 255)},
               {"magenta", RGB(255, 0, 255)},
               {"orange", RGB(255, 165, 0)},
               {"purple", RGB(128, 0, 128)},
               {"brown", RGB(165, 42, 42)},
               {"pink", RGB(255, 192, 203)},
               {"lime", RGB(0, 255, 0)},
               {"olive", RGB(128, 128, 0)},
               {"navy", RGB(0, 0, 128)},
               {"teal", RGB(0, 128, 128)},
               {"maroon", RGB(128, 0, 0)},
               {"silver", RGB(192, 192, 192)},
               {"gold", RGB(255, 215, 0)}
        };
    }
    // Méthode pour décoder une ligne CSS
    void parseStyleSheet(const std::string& css) {
        std::istringstream stream(css);
        std::string line;
        std::string currentSelector;

        // Lire ligne par ligne le contenu du style CSS
        while (std::getline(stream, line)) {
            line = trim(line);  // Supprimer les espaces
            if (line.empty()) continue;

            // Vérifier si la ligne est un sélecteur CSS (comme "Button {")
            if (line.back() == '{') {
                currentSelector = trim(line.substr(0, line.size() - 1));
            }
            else if (line == "}") {
                currentSelector.clear();
            }
            else if (!currentSelector.empty()) {
                auto parts = split(line, ':');
                if (parts.size() == 2) {
                    std::string property = trim(parts[0]);
                    std::string value = trim(parts[1]);
                    value = value.substr(0, value.size() - 1);  // Retirer le ; à la fin de la propriété
                    styles[currentSelector][property] = value;
                }
            }
        }
    }

    // Récupérer une propriété spécifique pour un sélecteur donné
    std::string getStyleProperty(const std::string& selector, const std::string& property) const {
        auto selectorIt = styles.find(selector);
        if (selectorIt != styles.end()) {
            const auto& props = selectorIt->second;
            auto propIt = props.find(property);
            if (propIt != props.end()) {
                return propIt->second;
            }
        }
        return "";
    }



    // Convertir une couleur CSS en COLORREF (hex, rgb(), nom de couleur)
    COLORREF parseColor(const std::string& color) {
        std::string trimmedColor = trim(color);

        // Vérifier si la couleur est au format hexadécimal (#RRGGBB)
        if (trimmedColor[0] == '#') {
            if (trimmedColor.length() == 7) {
                unsigned int rgb;
                std::stringstream ss;
                ss << std::hex << trimmedColor.substr(1);  // Ignorer le '#'
                ss >> rgb;

                return RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            }
            else {
                throw std::invalid_argument("Invalid hex color format. Expected #RRGGBB.");
            }
        }

        // Vérifier si la couleur est au format rgb()
        if (trimmedColor.find("rgb(") == 0 && trimmedColor.back() == ')') {
            std::string rgbValues = trimmedColor.substr(4, trimmedColor.size() - 5);  // Extraire le contenu entre rgb( et )
            std::stringstream ss(rgbValues);
            std::string item;
            int rgb[3], i = 0;

            while (std::getline(ss, item, ',') && i < 3) {
                rgb[i++] = std::stoi(trim(item));
            }

            if (i == 3) {
                return RGB(rgb[0], rgb[1], rgb[2]);
            }
            else {
                throw std::invalid_argument("Invalid rgb() format. Expected rgb(R, G, B).");
            }
        }
        auto it = colorNames.find(trimmedColor);
        if (it != colorNames.end()) {
            return it->second;
        }

        return RGB(0, 0, 0);
    }

private:
    std::string trim(const std::string& s) {
        std::string result = s;

        // Supprimer les commentaires de style /* ... */
        size_t startComment = result.find("/*");
        while (startComment != std::string::npos) {
            size_t endComment = result.find("*/", startComment + 2);
            if (endComment != std::string::npos) {
                result.erase(startComment, endComment - startComment + 2);  // Supprimer le commentaire
            }
            else {
                result.erase(startComment);  // Si le commentaire n'est pas fermé, on supprime tout à partir de /* 
            }
            startComment = result.find("/*");
        }

        // Supprimer les commentaires de style // 
        size_t commentPos = result.find("//");
        if (commentPos != std::string::npos) {
            result = result.substr(0, commentPos);  // Supprimer tout ce qui suit //
        }

        // Supprimer les espaces en début et en fin de chaîne
        size_t start = result.find_first_not_of(" \t\n\r");
        size_t end = result.find_last_not_of(" \t\n\r");

        return (start == std::string::npos) ? "" : result.substr(start, end - start + 1);
    }


    // Sépare une chaîne en utilisant un délimiteur donné
    std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    // Gérer les noms de couleurs CSS courants
     std::map<std::string, COLORREF> colorNames;
};
