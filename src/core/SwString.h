#ifndef SWSTRING_H
#define SWSTRING_H

#ifdef QT_CORE_LIB
#include <QDebug>
#endif

#include <string>
#include <iostream>
#include <cstring>
#include "SwList.h"
#include "SwCrypto.h"
#include <cctype>


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
        try {
            int result = std::stoi(data_);
            if (ok) {
                *ok = true; // Conversion r�ussie
            }
            return result;
        } catch (const std::exception&) {
            if (ok) {
                *ok = false; // Conversion �chou�e
            }
            std::cerr << "Invalid int conversion in SwString: " << data_ << std::endl;
        }
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

    size_t indexOf(const SwString& substring) const {
        size_t pos = data_.find(substring.data_);
        return (pos != std::string::npos) ? pos : -1; // Retourne -1 si non trouv�
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


    SwString& replace(const SwString& oldSub, const SwString& newSub) {
        size_t pos = 0;
        while ((pos = data_.find(oldSub.data_, pos)) != std::string::npos) {
            data_.replace(pos, oldSub.size(), newSub.data_);
            pos += newSub.size();
        }
        return *this;
    }

    SwString arg(const SwString& value) const {
        SwString result(*this);
        size_t pos = result.data_.find("%1");
        if (pos != std::string::npos) {
            result.data_.replace(pos, 2, value.data_);
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

    SwString mid(size_t pos, size_t len = std::string::npos) const {
        if (pos >= data_.size()) return SwString("");
        return SwString(data_.substr(pos, len));
    }

    SwString left(size_t n) const {
        return SwString(data_.substr(0, n));
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

#ifdef QT_CORE_LIB
    friend QDebug operator<<(QDebug debug, const SwString& str) {
        debug.nospace() << str.toStdString().c_str();
        return debug.space();
    }
#endif

private:
    std::string data_;
};


#include <functional>

namespace std {
template <>
struct hash<SwString> {
    size_t operator()(const SwString& s) const noexcept {
        return std::hash<std::string>{}(s.toStdString());
    }
};
}
#endif // SWSTRING_H
