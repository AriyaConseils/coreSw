#ifndef SWSTRING_H
#define SWSTRING_H
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

#ifdef QT_CORE_LIB
#include <QDebug>
#endif

#include <string>
#include <iostream>
#include <cstring>
#include "SwList.h"
#include "SwCrypto.h"
#include <cctype>
#include <unordered_map>
#include <stdexcept>

class SwString {

public:
    // Constructeurs
    SwString() : data_("") {} // Constructeur par d�faut
    SwString(const char* str) : data_(str) {} // Constructeur � partir de c-string
    SwString(const std::string& str) : data_(str) {} // Constructeur � partir de std::string
    SwString(const SwString& other) : data_(other.data_) {} // Constructeur par copie
    SwString(SwString&& other) noexcept : data_(std::move(other.data_)) {} // Constructeur par mouvement
    SwString(size_t count, char ch) : data_(std::string(count, ch)) {}
    SwString(char ch) : data_(1, ch) {}

    operator std::string&() {
        return data_;
    }

    operator const std::string&() const {
        return data_;
    }

    // Op�rateurs
    SwString& operator=(const SwString& other) {
        if (this != &other) {
            data_ = other.data_;
        }
        return *this;
    }

    SwString& operator=(SwString&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
        }
        return *this;
    }

    SwString& operator=(const char* str) {
        data_ = str;
        return *this;
    }

    SwString& operator=(const std::string& str) {
        data_ = str;
        return *this;
    }


    SwString& operator+=(const SwString& other) {
        data_ += other.data_;
        return *this;
    }

    // Surcharge pour SwString + const char*
    SwString operator+(const char* str) const {
        return SwString(data_ + str);
    }

    // Surcharge pour SwString + std::string
    SwString operator+(const std::string& str) const {
        return SwString(data_ + str);
    }

    // Surcharge pour SwString + SwString
    SwString operator+(const SwString& other) const {
        return SwString(data_ + other.data_);
    }


    bool operator==(const SwString& other) const {
        return data_ == other.data_;
    }

    bool operator!=(const SwString& other) const {
        return data_ != other.data_;
    }

    bool operator<(const SwString& other) const {
        return data_ < other.data_;
    }

    bool operator>(const SwString& other) const {
        return data_ > other.data_;
    }

    char operator[](size_t index) const {
        return data_[index];
    }

    char& operator[](size_t index) {
        return data_[index];
    }

    operator std::string() const {
        return data_;
    }

    // M�thodes de base
    size_t size() const {
        return data_.size();
    }

    size_t length() const {
        return data_.length();
    }

    bool isEmpty() const {
        return data_.empty();
    }

    bool isInt() const {
        if (data_.empty() || (data_[0] == '-' && data_.size() == 1)) return false;
        for (size_t i = (data_[0] == '-') ? 1 : 0; i < data_.size(); ++i)
            if (!std::isdigit(data_[i])) return false;
        return true;
    }

    bool isFloat() const {
        if (data_.empty() || (data_[0] == '-' && data_.size() == 1)) return false;
        bool hasDot = false;
        for (size_t i = (data_[0] == '-') ? 1 : 0; i < data_.size(); ++i) {
            if (data_[i] == '.') {
                if (hasDot) return false;
                hasDot = true;
            } else if (!std::isdigit(data_[i])) return false;
        }
        return hasDot;
    }

    const std::string& toStdString() const {
        return data_;
    }

    int toInt(bool* ok = nullptr) const {
        int result = 0;
        try {
            result = std::stoi(data_);
            if (ok) {
                *ok = true; // Conversion r�ussie
            }
        } catch (const std::exception&) {
            if (ok) {
                *ok = false; // Conversion �chou�e
            }
            std::cerr << "Invalid int conversion in SwString: " << data_ << std::endl;
        }
        return result;
    }


    float toFloat(bool* ok = nullptr) const {
        try {
            float result = std::stof(data_);
            if (ok) {
                *ok = true; // Conversion r�ussie
            }
            return result;
        } catch (const std::exception&) {
            if (ok) {
                *ok = false; // Conversion �chou�e
            }
            std::cerr << "Invalid float conversion in SwString: " << data_ << std::endl;
            return 0.0f; // Retourner une valeur par d�faut
        }
    }

    static SwString number(float value, int precision = -1) {
        std::ostringstream os;

        // Utiliser la notation fixe pour �viter le format scientifique
        os << std::fixed;

        // Appliquer la pr�cision si elle est sp�cifi�e
        if (precision >= 0) {
            os.precision(precision);
        }

        os << value;

        std::string result = os.str();

        // Supprimer les z�ros inutiles uniquement si une pr�cision est sp�cifi�e
        if (precision >= 0 && result.find('.') != std::string::npos) {
            result.erase(result.find_last_not_of('0') + 1);
            if (result.back() == '.') {
                result.pop_back();
            }
        }

        return SwString(result);
    }

    static SwString number(double value, int precision = -1) {
        std::ostringstream os;

        // Utiliser la notation fixe pour �viter le format scientifique
        os << std::fixed;

        // Appliquer la pr�cision si elle est sp�cifi�e
        if (precision >= 0) {
            os.precision(precision);
        }

        os << value;

        std::string result = os.str();

        // Supprimer les z�ros inutiles uniquement si une pr�cision est sp�cifi�e
        if (precision >= 0 && result.find('.') != std::string::npos) {
            result.erase(result.find_last_not_of('0') + 1);
            if (result.back() == '.') {
                result.pop_back();
            }
        }

        return SwString(result);
    }

    static SwString number(int value) {
        // Les entiers ne n�cessitent pas de format sp�cifique
        return SwString(std::to_string(value));
    }


    SwString toBase64() const {
        return SwString(SwCrypto::base64Encode(data_));
    }

    SwString deBase64() {
        std::vector<unsigned char> decoded = SwCrypto::base64Decode(data_);
        return SwString(std::string(decoded.begin(), decoded.end()));
    }

    static SwString fromBase64(const SwString& base64) {
        std::vector<unsigned char> decoded = SwCrypto::base64Decode(base64.toStdString());
        return SwString(std::string(decoded.begin(), decoded.end()));
    }

    SwString encryptAES(const SwString& key) const {
        try {
            return SwString(SwCrypto::encryptAES(data_, key.data_));
        } catch (const std::exception& e) {
            std::cerr << "Encryption error: " << e.what() << std::endl;
            return SwString("");
        }
    }

    // D�crypter avec une cl� donn�e
    SwString decryptAES(const SwString& key) {
        try {
            return SwString(SwCrypto::decryptAES(data_, key.data_));
        } catch (const std::exception& e) {
            std::cerr << "Decryption error: " << e.what() << std::endl;
            return SwString("");
        }
    }

    // D�crypter avec une cl� donn�e (statique)
    static SwString decryptAES(const SwString& encryptedBase64, const SwString& key) {
        try {
            return SwString(SwCrypto::decryptAES(encryptedBase64.data_, key.data_));
        } catch (const std::exception& e) {
            std::cerr << "Decryption error: " << e.what() << std::endl;
            return SwString("");
        }
    }


    // Amis pour les flux
    friend std::ostream& operator<<(std::ostream& os, const SwString& str) {
        os << str.data_;
        return os;
    }

    friend std::istream& operator>>(std::istream& is, SwString& str) {
        is >> str.data_;
        return is;
    }

    SwList<SwString> split(const char* delimiter) const {
        SwList<SwString> result;
        if (delimiter == nullptr || *delimiter == '\0') {
            return result; // Retourne une liste vide si le d�limiteur est invalide.
        }

        std::string strData = this->toStdString(); // Convertir SwString en std::string.
        std::string strDelimiter = delimiter;
        size_t start = 0;
        size_t end = 0;

        while ((end = strData.find(strDelimiter, start)) != std::string::npos) {
            result.append(SwString(strData.substr(start, end - start).c_str())); // Ajouter la sous-cha�ne trouv�e.
            start = end + strDelimiter.length();
        }

        // Ajouter le dernier segment, s'il y en a un.
        if (start < strData.length()) {
            result.append(SwString(strData.substr(start).c_str()));
        }

        return result;
    }


    SwList<SwString> split(char delimiter) const {
        SwList<SwString> result;

        size_t start = 0;
        size_t end = data_.find(delimiter);

        while (end != std::string::npos) {
            result.append(SwString(data_.substr(start, end - start)));
            start = end + 1;
            end = data_.find(delimiter, start);
        }

        // Ajouter le dernier segment de la cha�ne
        if (start < data_.size()) {
            result.append(SwString(data_.substr(start)));
        }

        return result;
    }

    SwList<SwString> split(const std::string& delimiter) const {
        if (delimiter.empty()) {
            throw std::invalid_argument("Delimiter cannot be empty.");
        }
        return split(SwString(delimiter));
    }

    SwList<SwString> split(const SwString& delimiter) const {
        if (delimiter.isEmpty()) {
            throw std::invalid_argument("Delimiter cannot be empty.");
        }

        SwList<SwString> result;

        size_t start = 0;
        size_t end = data_.find(delimiter.toStdString());

        while (end != std::string::npos) {
            result.append(SwString(data_.substr(start, end - start)));
            start = end + delimiter.size();
            end = data_.find(delimiter.toStdString(), start);
        }

        // Ajouter le dernier segment de la cha�ne
        if (start < data_.size()) {
            result.append(SwString(data_.substr(start)));
        }

        return result;
    }

    bool contains(const SwString& substring) const {
        return data_.find(substring.data_) != std::string::npos;
    }

    bool contains(const char* substring) const {
        return data_.find(substring) != std::string::npos;
    }

    SwString reversed() const {
        return SwString(std::string(data_.rbegin(), data_.rend()));
    }

    bool startsWith(const SwString& prefix) const {
        return data_.compare(0, prefix.size(), prefix.data_) == 0;
    }

    bool endsWith(const SwString& suffix) const {
        if (suffix.size() > data_.size()) return false;
        return data_.compare(data_.size() - suffix.size(), suffix.size(), suffix.data_) == 0;
    }

    int indexOf(const SwString& substring, size_t startIndex = 0) const {
        if (startIndex >= data_.size()) {
            return -1;
        }

        size_t pos = data_.find(substring.data_, startIndex);
        return (pos != std::string::npos) ? static_cast<int>(pos) : -1;
    }

    size_t lastIndexOf(const SwString& substring) const {
        size_t pos = data_.rfind(substring.data_);
        return (pos != std::string::npos) ? pos : -1; // Retourne -1 si non trouv�
    }

    size_t lastIndexOf(char character) const {
        size_t pos = data_.rfind(character);
        return (pos != std::string::npos) ? pos : -1; // Retourne -1 si non trouv�
    }

    size_t firstIndexOf(const SwString& substring) const {
        size_t pos = data_.find(substring.data_);
        return (pos != std::string::npos) ? pos : -1; // Retourne -1 si non trouv�
    }

    size_t firstIndexOf(char character) const {
        size_t pos = data_.find(character);
        return (pos != std::string::npos) ? pos : -1; // Retourne -1 si non trouv�
    }

    SwString trimmed() const {
        size_t start = data_.find_first_not_of(" \t\n\r");
        size_t end = data_.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) return SwString(""); // Cha�ne vide si tout est des espaces
        return SwString(data_.substr(start, end - start + 1));
    }

    SwString toUpper() const {
        std::string upper(data_);
        for (auto& c : upper) {
            c = std::toupper(c);
        }
        return SwString(upper);
    }

    SwString toLower() const {
        std::string lower(data_);
        for (auto& c : lower) {
            c = std::tolower(c);
        }
        return SwString(lower);
    }

    static SwString fromWString(const std::wstring& wideStr) {
        if (wideStr.empty()) {
            return SwString();
        }
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (bufferSize <= 0) {
            return SwString();
        }
        std::string utf8Str(bufferSize - 1, '\0'); // bufferSize - 1 pour ignorer le caractère nul final
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], bufferSize, nullptr, nullptr);
        return SwString(utf8Str);
    }

    static SwString fromWCharArray(const wchar_t* wideStr) {
        if (!wideStr) {
            return SwString();
        }
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
        if (bufferSize <= 0) {
            return SwString();
        }
        std::string utf8Str(bufferSize - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &utf8Str[0], bufferSize, nullptr, nullptr);
        return SwString(utf8Str);
    }

    SwString& replace(const SwString& oldSub, const SwString& newSub) {
        if(oldSub == "" || oldSub == "\0"){
            return *this;
        }
        size_t pos = 0;
        while ((pos = data_.find(oldSub.data_, pos)) != std::string::npos) {
            data_.replace(pos, oldSub.size(), newSub.data_);
            pos += newSub.size();
        }
        return *this;
    }

    SwString arg(const SwString& value) const {
        SwString result(*this); // Copie de la chaîne actuelle

        // Trouver le premier placeholder "%N"
        size_t start = result.data_.find('%');
        while (start != std::string::npos) {
            // Vérifier si c'est un placeholder valide ("%N" où N est un chiffre)
            if (start + 1 < result.data_.size() && std::isdigit(result.data_[start + 1])) {
                // Remplacer le placeholder par la valeur
                result.data_.replace(start, 2, value.data_);
                break; // Un seul remplacement par appel à `arg`
            }
            // Chercher le placeholder suivant
            start = result.data_.find('%', start + 1);
        }

        return result;
    }

    size_t count(const SwString& substring) const {
        size_t pos = 0, occurrences = 0;
        while ((pos = data_.find(substring.data_, pos)) != std::string::npos) {
            ++occurrences;
            pos += substring.size();
        }
        return occurrences;
    }

    SwString simplified() const {
        std::string result;
        bool inSpace = false;
        for (char c : data_) {
            if (std::isspace(c)) {
                if (!inSpace) {
                    result += ' ';
                    inSpace = true;
                }
            } else {
                result += c;
                inSpace = false;
            }
        }
        return SwString(result);
    }

    SwString mid(int pos, int len = -1) const {
        if (pos < 0 || pos >= static_cast<int>(data_.size())) {
            return SwString("");
        }

        if (len < 0 || pos + len > static_cast<int>(data_.size())) {
            len = static_cast<int>(data_.size()) - pos;
        }

        return SwString(data_.substr(pos, len));
    }

    SwString left(int n) const {
        if (n < 0) { n = 0; }
        return SwString(data_.substr(0, static_cast<size_t>(n)));
    }


    SwString right(size_t n) const {
        if (n >= data_.size()) return *this;
        return SwString(data_.substr(data_.size() - n));
    }

    SwString first() const {
        if (data_.empty()) {
            return SwString("");
        }
        return SwString(1, data_.front()); // Cr�e une SwString avec le premier caract�re
    }

    SwString last() const {
        if (data_.empty()) {
            return SwString("");
        }
        return SwString(1, data_.back()); // Cr�e une SwString avec le dernier caract�re
    }

    SwString& append(const SwString& other) {
        data_ += other.data_; // Ajouter `other.data_` à la fin de `data_`
        return *this;
    }

    SwString& append(const std::string& str) {
        data_ += str; // Ajouter `str` à la fin de `data_`
        return *this;
    }

    SwString& append(const char* cstr) {
        data_ += std::string(cstr); // Ajouter la chaîne C à la fin de `data_`
        return *this;
    }

    SwString& append(char ch) {
        data_ += ch; // Ajouter le caractère à la fin de `data_`
        return *this;
    }

    SwString& prepend(const SwString& other) {
        data_ = other.data_ + data_; // Préfixer `data_` avec `other.data_`
        return *this;                // Retourner l'objet courant pour permettre le chaînage
    }

    SwString& prepend(const std::string& str) {
        data_ = str + data_; // Préfixer `data_` avec `str`
        return *this;
    }

    SwString& prepend(const char* cstr) {
        data_ = std::string(cstr) + data_; // Préfixer `data_` avec la chaîne C
        return *this;
    }

    SwString& prepend(char ch) {
        data_.insert(data_.begin(), ch); // Insérer le caractère au début
        return *this;
    }


    const char* toUtf8() const {
        return data_.c_str();
    }

    const wchar_t* toWChar() const {
        std::wstring wideString(data_.begin(), data_.end());
        return wideString.c_str();
    }

    std::wstring toStdWString() const {
        if (data_.empty()) {
            return std::wstring();
        }

        int size_needed = MultiByteToWideChar(CP_UTF8, 0, data_.c_str(), -1, nullptr, 0);
        if (size_needed <= 0) {
            throw std::runtime_error("Failed to convert string to wstring");
        }

        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, data_.c_str(), -1, &wstr[0], size_needed);

        // Remove the extra null character added by MultiByteToWideChar
        wstr.pop_back();

        return wstr;
    }

    const char* toLatin1() const {
        static std::string latin1String;
        latin1String.clear();

        for (char32_t c : data_) {
            // Vérification si c'est un caractère standard Latin1
            if (c <= 0xFF) {
                latin1String += static_cast<char>(c); // Ajout direct pour les caractères standards
                continue;
            }

            // Vérification si c'est un Unicode valide (0x0000 à 0x10FFFF)
            if (c > 0x00FF && c <= 0x10FFFF) {
                latin1String += unicodeToLatin1(c);
            } else {
                latin1String += '?';
            }
        }

        return latin1String.c_str();
    }

    static SwString fromLatin1(const char* str, size_t length) {
        return SwString(std::string(str, length));
    }

    void resize(int newSize) {
        data_.resize(static_cast<size_t>(newSize));
    }

    const char* data() const {
        return data_.data();
    }

    char* data() {
        return &data_[0]; // Attention : modifie directement la donnée
    }

    char* begin() {
        return &data_[0];
    }

    char* end() {
        return &data_[0] + data_.size();
    }

    const char* begin() const {
        return data_.data();
    }

    const char* end() const {
        return data_.data() + data_.size();
    }

    size_t utf16Size() const {
        return std::wstring(data_.begin(), data_.end()).size();
    }

    size_t utf32Size() const {
        return std::u32string(data_.begin(), data_.end()).size();
    }

    SwString& chop(int n) {
        if (n <= 0) {
            return *this;
        }

        if (static_cast<size_t>(n) >= data_.size()) {
            data_.clear();
        } else {
            data_.erase(data_.size() - static_cast<size_t>(n));
        }

        return *this;
    }


    friend SwString operator+(const char* lhs, const SwString& rhs);

#ifdef QT_CORE_LIB
    friend QDebug operator<<(QDebug debug, const SwString& str) {
        debug.nospace() << str.toStdString().c_str();
        return debug.space();
    }
#endif

private:
    std::string data_;
    char unicodeToLatin1(char32_t unicode) const {
        static const std::unordered_map<char32_t, char> unicodeToLatin1Table = {
            {0x0100, 'A'}, {0x0101, 'a'}, {0x0102, 'A'}, {0x0103, 'a'}, {0x0104, 'A'}, {0x0105, 'a'}, // Ā, ā, Ă, ă, Ą, ą
            {0x0106, 'C'}, {0x0107, 'c'}, {0x0108, 'C'}, {0x0109, 'c'}, {0x010A, 'C'}, {0x010B, 'c'}, // Ć, ć, Ĉ, ĉ, Ċ, ċ
            {0x010C, 'C'}, {0x010D, 'c'}, {0x010E, 'D'}, {0x010F, 'd'}, {0x0110, 'D'}, {0x0111, 'd'}, // Č, č, Ď, ď, Đ, đ
            {0x0112, 'E'}, {0x0113, 'e'}, {0x0114, 'E'}, {0x0115, 'e'}, {0x0116, 'E'}, {0x0117, 'e'}, // Ē, ē, Ĕ, ĕ, Ė, ė
            {0x0118, 'E'}, {0x0119, 'e'}, {0x011A, 'E'}, {0x011B, 'e'}, {0x011C, 'G'}, {0x011D, 'g'}, // Ę, ę, Ě, ě, Ĝ, ĝ
            {0x011E, 'G'}, {0x011F, 'g'}, {0x0120, 'G'}, {0x0121, 'g'}, {0x0122, 'G'}, {0x0123, 'g'}, // Ğ, ğ, Ġ, ġ, Ģ, ģ
            {0x0124, 'H'}, {0x0125, 'h'}, {0x0126, 'H'}, {0x0127, 'h'}, {0x0128, 'I'}, {0x0129, 'i'}, // Ĥ, ĥ, Ħ, ħ, Ĩ, ĩ
            {0x012A, 'I'}, {0x012B, 'i'}, {0x012C, 'I'}, {0x012D, 'i'}, {0x012E, 'I'}, {0x012F, 'i'}, // Ī, ī, Ĭ, ĭ, Į, į
            {0x0130, 'I'}, {0x0131, 'i'}, {0x0132, 'I'}, {0x0133, 'i'}, {0x0134, 'J'}, {0x0135, 'j'}, // İ, ı, Ĳ, ĳ, Ĵ, ĵ
            {0x0136, 'K'}, {0x0137, 'k'}, {0x0138, 'k'}, {0x0139, 'L'}, {0x013A, 'l'}, {0x013B, 'L'}, // Ķ, ķ, ĸ, Ĺ, ĺ, Ļ
            {0x013C, 'l'}, {0x013D, 'L'}, {0x013E, 'l'}, {0x013F, 'L'}, {0x0140, 'l'}, {0x0141, 'L'}, // ļ, Ľ, ľ, Ŀ, ŀ, Ł
            {0x0142, 'l'}, {0x0143, 'N'}, {0x0144, 'n'}, {0x0145, 'N'}, {0x0146, 'n'}, {0x0147, 'N'}, // ł, Ń, ń, Ņ, ņ, Ň
            {0x0148, 'n'}, {0x0149, 'n'}, {0x014A, 'N'}, {0x014B, 'n'}, {0x014C, 'O'}, {0x014D, 'o'}, // ň, ŉ, Ŋ, ŋ, Ō, ō
            {0x014E, 'O'}, {0x014F, 'o'}, {0x0150, 'O'}, {0x0151, 'o'}, {0x0152, 'O'}, {0x0153, 'o'}, // Ŏ, ŏ, Ő, ő, Œ, œ
            {0x0154, 'R'}, {0x0155, 'r'}, {0x0156, 'R'}, {0x0157, 'r'}, {0x0158, 'R'}, {0x0159, 'r'}, // Ŕ, ŕ, Ŗ, ŗ, Ř, ř
            {0x015A, 'S'}, {0x015B, 's'}, {0x015C, 'S'}, {0x015D, 's'}, {0x015E, 'S'}, {0x015F, 's'}, // Ś, ś, Ŝ, ŝ, Ş, ş
            {0x0160, 'S'}, {0x0161, 's'}, {0x0162, 'T'}, {0x0163, 't'}, {0x0164, 'T'}, {0x0165, 't'}, // Š, š, Ţ, ţ, Ť, ť
            {0x0166, 'T'}, {0x0167, 't'}, {0x0168, 'U'}, {0x0169, 'u'}, {0x016A, 'U'}, {0x016B, 'u'}, // Ŧ, ŧ, Ũ, ũ, Ū, ū
            {0x016C, 'U'}, {0x016D, 'u'}, {0x016E, 'U'}, {0x016F, 'u'}, {0x0170, 'U'}, {0x0171, 'u'}, // Ŭ, ŭ, Ů, ů, Ű, ű
            {0x0172, 'U'}, {0x0173, 'u'}, {0x0174, 'W'}, {0x0175, 'w'}, {0x0176, 'Y'}, {0x0177, 'y'}, // Ų, ų, Ŵ, ŵ, Ŷ, ŷ
            {0x0178, 'Y'}, {0x0179, 'Z'}, {0x017A, 'z'}, {0x017B, 'Z'}, {0x017C, 'z'}, {0x017D, 'Z'}, // Ÿ, Ź, ź, Ż, ż, Ž
            {0x017E, 'z'}, {0x017F, 's'}, {0x0180, 'b'}, {0x0181, 'B'}, {0x0182, 'B'}, {0x0183, 'b'}, // ž, ſ, ƀ, Ɓ, Ƃ, ƃ
            {0x0186, 'C'}, {0x0187, 'C'}, {0x0188, 'c'}, {0x0189, 'D'}, {0x018A, 'D'}, {0x018B, 'D'}, // Ɔ, Ƈ, ƈ, Ɖ, Ɗ, Ƌ
            {0x018C, 'd'}, {0x0192, 'f'}, {0x0193, 'G'}, {0x0194, 'G'}, {0x0195, 'h'}, {0x0197, 'I'}, // ƌ, ƒ, Ɠ, Ɣ, ƕ, Ɨ
            {0x0198, 'K'}, {0x0199, 'k'}, {0x019A, 'l'}, {0x019B, 'l'}, {0x019C, 'M'}, {0x019D, 'N'}, // Ƙ, ƙ, ƚ, ƛ, Ɯ, Ɲ
            {0x019E, 'n'}, {0x019F, 'O'}, {0x01A0, 'O'}, {0x01A1, 'o'}, {0x01A2, 'Q'}, {0x01A3, 'q'}, // ƞ, Ɵ, Ơ, ơ, Ƣ, ƣ
            {0x01A4, 'P'}, {0x01A5, 'p'}, {0x01A6, 'R'}, {0x01A7, 'S'}, {0x01A8, 's'}, {0x01A9, 'T'}, // Ƥ, ƥ, Ʀ, Ƨ, ƨ, Ʃ
            {0x01AA, 't'}, {0x01AB, 't'}, {0x01AC, 'T'}, {0x01AD, 't'}, {0x01AE, 'T'}, {0x01AF, 'U'}, // ƪ, ƫ, Ƭ, ƭ, Ʈ, Ư
            {0x01B0, 'u'}, {0x01B1, 'V'}, {0x01B2, 'Y'}, {0x01B3, 'Y'}, {0x01B4, 'y'}, {0x01B5, 'Z'}, // ư, Ʋ, Ƴ, ƴ, Ƶ, ƶ
            // Ajoutez d'autres caractères Unicode si nécessaire
        };

        auto it = unicodeToLatin1Table.find(unicode);
        if (it != unicodeToLatin1Table.end()) {
            return it->second; // Retourner le caractère converti
        }

        // Retourner '?' si le caractère n'est pas trouvé
        return '?';
    }
};


inline SwString operator+(const char* lhs, const SwString& rhs) {
    // Construit un SwString à partir de lhs puis utilise l'opérateur existant
    return SwString(lhs) + rhs;
}


#include <functional>

namespace std {
template <>
struct hash<SwString> {
    size_t operator()(const SwString& s) const noexcept {
        return std::hash<std::string>{}(s.toStdString());
    }
};
}

using SwStringList = SwList<SwString>;
#endif // SWSTRING_H
