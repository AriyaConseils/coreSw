#pragma once

#include "SwJsonObject.h"
#include "SwJsonArray.h"
#include "SwString.h"
#include "SwList.h"
#include <sstream>

class SwJsonDocument {
public:
    enum class JsonFormat { Compact, Pretty };

    // Constructeurs
    SwJsonDocument() = default;
    SwJsonDocument(const SwJsonObject& object) : rootValue_(object) {}
    SwJsonDocument(const SwJsonArray& array) : rootValue_(array) {}

    // Définir la racine
    void setObject(const SwJsonObject& object) {
        rootValue_ = SwJsonValue(object);
    }

    void setArray(const SwJsonArray& array) {
        rootValue_ = SwJsonValue(array);
    }

    // Vérifier le type
    bool isObject() const { return rootValue_.isObject(); }
    bool isArray() const { return rootValue_.isArray(); }

    SwJsonObject object() const {
        if (!rootValue_.isObject()) {
            std::cerr << "Root is not a JSON object." << std::endl;
            return SwJsonObject(); // Retourne un objet vide si ce n'est pas un objet
        }
        return *rootValue_.toObject(); // Retourne l'objet JSON
    }

    // Retourne le document sous forme de tableau JSON (SwJsonArray)
    SwJsonArray array() const {
        if (!rootValue_.isArray()) {
            std::cerr << "Root is not a JSON array." << std::endl;
            return SwJsonArray(); // Retourne un tableau vide si ce n'est pas un tableau
        }
        return *rootValue_.toArray(); // Retourne le tableau JSON
    }

    // Retourne la racine sous forme de valeur JSON (SwJsonValue)
    SwJsonValue toJsonValue() const {
        return rootValue_; // Retourne directement la valeur racine
    }

    SwJsonValue& find(const SwString& rawPath, bool createIfNotExist = false) {
        // Remplacer les '\\' par '/'
        SwString path = rawPath;
        path.replace("\\", "/");

        SwList<SwString> tokens = path.split('/');

        SwJsonValue* current = &rootValue_;

        for (const SwString& token : tokens) {
            if (!current->isObject()) {
                if (createIfNotExist) {
                    current->setObject(std::make_shared<SwJsonObject>()); // Convertir en objet si nécessaire
                } else {
                    std::cerr << "Path not found: '" << token << "' - Current value is not an object.\n";
                    static SwJsonValue invalidValue; // Valeur par défaut pour les cas d'échec
                    return invalidValue;
                }
            }

            if (!current->toObject()->contains(token)) {
                if (createIfNotExist) {
                    (*current->toObject())[token] = SwJsonValue(std::make_shared<SwJsonObject>());
                } else {
                    std::cerr << "Path not found: '" << token << "' - Key does not exist.\n";
                    static SwJsonValue invalidValue; // Valeur par défaut pour les cas d'échec
                    return invalidValue;
                }
            }

            current = &(*current->toObject())[token];
        }

        return *current;
    }

    // Générer une chaîne JSON
    SwString toJson(JsonFormat format = JsonFormat::Compact, const SwString& encryptionKey = "") const {
        SwString result;

        generateJson(rootValue_, result, format == JsonFormat::Pretty, 0, encryptionKey);

        return result;
    }


    static SwJsonDocument fromJson(const std::string& jsonString, const SwString& decryptionKey = "") {
        SwJsonDocument doc;
        std::string errorMessage;
        doc.loadFromJson(jsonString, errorMessage, decryptionKey);
        return doc;
    }

    bool loadFromJson(const std::string& jsonString, std::string& errorMessage, const SwString& decryptionKey = "") {
        try {
            // Analyse JSON
            size_t index = 0;
            rootValue_ = parseJson(jsonString, index, decryptionKey);

            // Vérifiez qu'il ne reste pas de caractères non analysés
            while (index < jsonString.size() && std::isspace(jsonString[index])) {
                ++index;
            }
            if (index < jsonString.size()) {
                errorMessage = "Unexpected characters at the end of JSON.";
                return false;
            }
            return true;
        } catch (const std::runtime_error& e) {
            errorMessage = e.what();
            return false;
        }
    }



private:
    SwJsonValue rootValue_; // Racine du document

    void generateJson(const SwJsonValue& value, SwString& output, bool pretty, int indentLevel, const SwString& encryptionKey = "") const {
        SwString indent = pretty ? SwString(indentLevel * 2, ' ') : SwString("");
        SwString childIndent = pretty ? SwString((indentLevel + 1) * 2, ' ') : SwString("");

        auto processValue = [&](const SwString& val) -> SwString {
            return encryptionKey.isEmpty() ? val : val.encryptAES(encryptionKey);
        };

        if (value.isString()) {
            output += SwString("\"%1\"").arg(processValue(value.toString()));
        } else if (value.isBool()) {
            SwString boolStr = value.toBool() ? "true" : "false";
            output += SwString("%1").arg(processValue(boolStr));
        } else if (value.isInt()) {
            SwString intStr = SwString::number(value.toInt());
            output += SwString("%1").arg(processValue(intStr));
        } else if (value.isDouble()) {
            SwString doubleStr = SwString::number(value.toDouble());
            output += SwString("%1").arg(processValue(doubleStr));
        } else if (value.isNull()) {
            SwString nullStr = "null";
            output += SwString("\"%1\"").arg(processValue(nullStr));
        } else if (value.isObject()) {
            const auto& obj = *value.toObject();
            if(!obj.isEmpty()){
                output += pretty ? "{\n" : "{";
                bool first = true;
                for (const auto& pair : obj.data()) {
                    if (!first) output += pretty ? ",\n" : ",";
                    first = false;

                    if (pretty) output += childIndent;
                    output += "\"" + pair.first + "\": ";
                    generateJson(pair.second, output, pretty, indentLevel + 1, encryptionKey);
                }
                if (pretty && !obj.data().empty()) output += SwString("\n") + indent;
                output += "}";
            } else {
                output += "{}";
            }
        } else if (value.isArray()) {
            const auto& arr = *value.toArray();
            if(!arr.isEmpty()){
                output += pretty ? "[\n" : "[";
                for (size_t i = 0; i < arr.data().size(); ++i) {
                    if (i > 0) output += pretty ? ",\n" : ",";
                    if (pretty) output += childIndent;
                    generateJson(arr.data()[i], output, pretty, indentLevel + 1, encryptionKey);
                }
                if (pretty && !arr.data().empty()) output += SwString("\n") + indent;
                output += "]";
            } else {
                output += "[]";
            }
        }
    }



    SwJsonValue parseJson(const SwString& jsonString, size_t& index, const SwString& decryptionKey = "") const {
        auto debugContext = [&](size_t errorIndex, const SwString& message) {
            size_t start = (errorIndex > 30) ? errorIndex - 30 : 0;
            size_t end = (errorIndex + 30 < jsonString.size()) ? errorIndex + 30 : jsonString.size();
            std::cerr << message << "\nContext around error:\n"
                      << jsonString.mid(start, end - start).toStdString() << "\n"
                      << std::string(errorIndex - start, ' ') << "^\n";
        };

        // Ignorer les espaces
        while (index < jsonString.size() && std::isspace(jsonString[index])) ++index;

        if (index >= jsonString.size()) {
            debugContext(index, "Unexpected end of JSON: the parser reached the end of the input.");
            return SwJsonValue();
        }

        char c = jsonString[index];

        // Gestion des différents types de jetons JSON
        if (c == '{') {
            return parseObject(jsonString, index, decryptionKey);
        } else if (c == '[') {
            return parseArray(jsonString, index, decryptionKey);
        } else if (c == '\"') {
            SwString value = parseString(jsonString, index);

            if (!decryptionKey.isEmpty()) {
                value = value.decryptAES(decryptionKey);
            }

            if (value == "true") return SwJsonValue(true);
            if (value == "false") return SwJsonValue(false);
            if (value == "null") return SwJsonValue();
            if (value.isInt()) return SwJsonValue(value.toInt());
            if (value.isFloat()) return SwJsonValue(value.toFloat());

            return SwJsonValue(value);
        } else if (std::isdigit(c) || c == '-' || c == 't' || c == 'f' || (!decryptionKey.isEmpty())) {
            if (!decryptionKey.isEmpty()) {
                // Lecture jusqu'à la première virgule, accolade fermante ou espace
                size_t start = index;
                while (index < jsonString.size() && jsonString[index] != ',' &&
                       jsonString[index] != '}' && !std::isspace(jsonString[index])) {
                    ++index;
                }

                SwString encryptedValue = jsonString.mid(start, index - start);
                SwString decryptedValue;

                try {
                    decryptedValue = encryptedValue.decryptAES(decryptionKey);
                } catch (const std::exception& e) {
                    debugContext(start, SwString("Failed to decrypt value: ") + SwString(e.what()));
                    return SwJsonValue();
                }

                // Vérification du type après décryptage
                if (decryptedValue == "true") return SwJsonValue(true);
                if (decryptedValue == "false") return SwJsonValue(false);
                if (decryptedValue == "null") return SwJsonValue();
                if (decryptedValue.isInt()) return SwJsonValue(decryptedValue.toInt());
                if (decryptedValue.isFloat()) return SwJsonValue(decryptedValue.toFloat());

                return SwJsonValue(decryptedValue);
            }

            if (c == 't' || c == 'f') {
                SwString literal = parseLiteral(jsonString, index);
                if (literal == "true") return SwJsonValue(true);
                if (literal == "false") return SwJsonValue(false);

                debugContext(index, "Invalid literal detected. Expected 'true' or 'false'.");
                return SwJsonValue();
            } else {
                return parseNumber(jsonString, index);
            }
        }
        debugContext(index, "Invalid JSON token detected.");
        return SwJsonValue();
    }


    SwJsonValue parseObject(const SwString& jsonString, size_t& index, const SwString& decryptionKey = "") const {
        SwJsonObject object;
        ++index; // Passer '{'

        while (index < jsonString.size()) {
            // Ignorer les espaces
            while (index < jsonString.size() && std::isspace(jsonString[index])) ++index;

            if (jsonString[index] == '}') { ++index; break; } // Fin de l'objet

            if (jsonString[index] != '\"') {
                std::cerr << "Expected key at index " << index << std::endl;
                return SwJsonValue(); // Erreur
            }

            SwString key = parseString(jsonString, index); // Parse la clé

            while (index < jsonString.size() && std::isspace(jsonString[index])) ++index;
            if (jsonString[index] != ':') {
                std::cerr << "Expected ':' after key at index " << index << std::endl;
                return SwJsonValue(); // Erreur
            }

            ++index; // Passer ':'
            object[key] = parseJson(jsonString, index, decryptionKey); // Parse la valeur

            while (index < jsonString.size() && std::isspace(jsonString[index])) ++index;
            if (jsonString[index] == ',') ++index; // Passer ','
            else if (jsonString[index] == '}') { ++index; break; } // Fin de l'objet
            else {
                std::cerr << "Expected ',' or '}' at index " << index << std::endl;
                return SwJsonValue(); // Erreur
            }
        }

        return SwJsonValue(object);
    }




    SwJsonValue parseArray(const SwString& jsonString, size_t& index, const SwString& decryptionKey = "") const {
        SwJsonArray array;
        ++index; // Passer '['

        while (index < jsonString.size()) {
            // Ignorer les espaces
            while (index < jsonString.size() && std::isspace(jsonString[index])) ++index;

            if (jsonString[index] == ']') { ++index; break; } // Fin du tableau

            array.append(parseJson(jsonString, index, decryptionKey)); // Parse un élément

            // Ignorer les espaces
            while (index < jsonString.size() && std::isspace(jsonString[index])) ++index;

            if (jsonString[index] == ',') ++index;  // Passer ','
            else if (jsonString[index] == ']') { ++index; break; } // Fin du tableau
            else {
                std::cerr << "Expected ',' or ']' at index " << index << std::endl;
                return SwJsonValue(); // Retourner une valeur par défaut en cas d'erreur
            }
        }

        return SwJsonValue(array);
    }




    SwString parseString(const SwString& jsonString, size_t& index, const SwString& decryptionKey = "") const {
        ++index; // Passer le premier guillemet '"'
        SwString result;

        while (index < jsonString.size() && jsonString[index] != '\"') {
            if (jsonString[index] == '\\') {
                if (++index >= jsonString.size()) {
                    std::cerr << "Invalid escape sequence in string." << std::endl;
                    return SwString();
                }
                char escapeChar = jsonString[index];
                result += (escapeChar == '\"') ? '\"' :
                              (escapeChar == '\\') ? '\\' :
                              (escapeChar == '/') ? '/' :
                              (escapeChar == 'b') ? '\b' :
                              (escapeChar == 'f') ? '\f' :
                              (escapeChar == 'n') ? '\n' :
                              (escapeChar == 'r') ? '\r' :
                              (escapeChar == 't') ? '\t' : '\0';
                if (result.last() == '\0') {
                    std::cerr << "Invalid escape character in string." << std::endl;
                    return SwString();
                }
            } else {
                result += jsonString[index];
            }
            ++index;
        }

        if (index >= jsonString.size() || jsonString[index] != '\"') {
            std::cerr << "Unterminated string in JSON." << std::endl;
            return SwString();
        }
        ++index; // Passer le dernier guillemet '"'

        return decryptionKey.isEmpty() ? result : result.decryptAES(decryptionKey);
    }



    SwJsonValue parseNumber(const SwString& jsonString, size_t& index, const SwString& decryptionKey = "") const {
        size_t start = index;

        if (jsonString[index] == '-') ++index; // Gérer les nombres négatifs

        while (index < jsonString.size() && (std::isdigit(jsonString[index]) || jsonString[index] == '.')) ++index;

        SwString numberString = jsonString.mid(start, index - start);
        if (!decryptionKey.isEmpty()) numberString = numberString.decryptAES(decryptionKey);

        return numberString.isFloat() ? SwJsonValue(numberString.toFloat())
                                      : SwJsonValue(numberString.toInt());
    }

    SwString parseLiteral(const SwString& jsonString, size_t& index) const {
        size_t start = index;
        while (index < jsonString.size() && std::isalpha(jsonString[index])) ++index;
        return jsonString.mid(start, index - start);
    }




};
