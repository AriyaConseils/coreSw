#ifndef SWREGULAREXPRESSION_H
#define SWREGULAREXPRESSION_H
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

#include <regex>
#include "SwString.h"
#include "SwList.h"

class SwRegularExpressionMatch {
public:
    // Constructeur par défaut
    SwRegularExpressionMatch()
        : _match() {}

    // Constructeur avec std::smatch
    explicit SwRegularExpressionMatch(const std::smatch& match)
        : _match(match) {}

    // Constructeur de copie
    SwRegularExpressionMatch(const SwRegularExpressionMatch& other)
        : _match(other._match) {}

    // Opérateur d'affectation
    SwRegularExpressionMatch& operator=(const SwRegularExpressionMatch& other) {
        if (this != &other) {
            _match = other._match;
        }
        return *this;
    }

    // Vérifier si une correspondance existe
    bool hasMatch() const { return !_match.empty(); }

    // Obtenir une correspondance capturée par index
    SwString captured(int index = 0) const {
        if (index >= 0 && index < static_cast<int>(_match.size())) {
            return SwString(_match[index].str().c_str());
        }
        return SwString("");
    }

    // Obtenir la position de début de la correspondance capturée
    int capturedStart(int index = 0) const {
        if (index >= 0 && index < static_cast<int>(_match.size())) {
            return static_cast<int>(_match.position(index));
        }
        return -1; // Retourne -1 si l'index est invalide
    }

    // Obtenir la position de fin de la correspondance capturée
    int capturedEnd(int index = 0) const {
        if (index >= 0 && index < static_cast<int>(_match.size())) {
            return static_cast<int>(_match.position(index) + _match.length(index));
        }
        return -1; // Retourne -1 si l'index est invalide
    }
private:
    std::smatch _match;
};



class SwRegularExpression {
public:
    explicit SwRegularExpression(const SwString& pattern = "")
        : _pattern(pattern), _isValid(true) {
        try {
            _regex = std::regex(pattern.toStdString());
        } catch (const std::regex_error&) {
            _isValid = false;
        }
    }

    // Copy constructor
    SwRegularExpression(const SwRegularExpression& other)
        : _pattern(other._pattern), _regex(other._regex), _isValid(other._isValid) {}

    // Copy assignment operator
    SwRegularExpression& operator=(const SwRegularExpression& other) {
        if (this != &other) {
            _pattern = other._pattern;
            _regex = other._regex;
            _isValid = other._isValid;
        }
        return *this;
    }

    bool operator==(const SwRegularExpression& other) const {
        return _pattern == other._pattern && _isValid == other._isValid;
    }

    bool operator!=(const SwRegularExpression& other) const {
        return !(*this == other);
    }

    bool isValid() const { return _isValid; }
    SwString pattern() const { return _pattern; }

    const std::regex& getStdRegex() const { return _regex; }

    SwRegularExpressionMatch match(const SwString& text) const {
        std::smatch match;
        if (std::regex_search(text.toStdString(), match, _regex)) {
            return SwRegularExpressionMatch(match);
        }
        return SwRegularExpressionMatch(std::smatch());
    }

    SwList<SwString> globalMatch(const SwString& text) const {
        SwList<SwString> matches;
        try {
            auto begin = std::sregex_iterator(text.toStdString().begin(), text.toStdString().end(), _regex);
            auto end = std::sregex_iterator();
            for (auto it = begin; it != end; ++it) {
                matches.append(SwString(it->str().c_str()));
            }
        } catch (const std::regex_error&) {
            // Handle invalid regex usage
        }
        return matches;
    }

private:
    SwString _pattern;
    std::regex _regex;
    bool _isValid;
};


#endif // SWREGULAREXPRESSION_H
