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

#include "SwJsonObject.h"
#include "SwJsonArray.h"
#include "SwString.h"
#include "SwList.h"
#include <sstream>

class SwJsonDocument {
public:
    enum class JsonFormat { Compact, Pretty };

    /**
     * @brief Default constructor for the SwJsonDocument class.
     *
     * Initializes an empty JSON document with no root value.
     */
    SwJsonDocument() = default;

    /**
     * @brief Constructor to initialize a SwJsonDocument with a JSON object.
     *
     * Sets the root of the document to the specified JSON object.
     *
     * @param object A SwJsonObject to set as the root of the document.
     */
    SwJsonDocument(const SwJsonObject& object) : rootValue_(object) {}

    /**
     * @brief Constructor to initialize a SwJsonDocument with a JSON array.
     *
     * Sets the root of the document to the specified JSON array.
     *
     * @param array A SwJsonArray to set as the root of the document.
     */
    SwJsonDocument(const SwJsonArray& array) : rootValue_(array) {}

    /**
     * @brief Sets the root of the JSON document to a JSON object.
     *
     * Replaces the current root value with the specified JSON object.
     *
     * @param object The SwJsonObject to set as the root of the document.
     */
    void setObject(const SwJsonObject& object) {
        rootValue_ = SwJsonValue(object);
    }

    /**
     * @brief Sets the root of the JSON document to a JSON array.
     *
     * Replaces the current root value with the specified JSON array.
     *
     * @param array The SwJsonArray to set as the root of the document.
     */
    void setArray(const SwJsonArray& array) {
        rootValue_ = SwJsonValue(array);
    }

    /**
     * @brief Checks if the root of the JSON document is a JSON object.
     *
     * Determines whether the root value of the document is of type SwJsonObject.
     *
     * @return `true` if the root is a JSON object, `false` otherwise.
     */
    bool isObject() const { return rootValue_.isObject(); }

    /**
     * @brief Checks if the root of the JSON document is a JSON array.
     *
     * Determines whether the root value of the document is of type SwJsonArray.
     *
     * @return `true` if the root is a JSON array, `false` otherwise.
     */
    bool isArray() const { return rootValue_.isArray(); }

    SwJsonObject object() const {
        if (!rootValue_.isObject()) {
            std::cerr << "Root is not a JSON object." << std::endl;
            return SwJsonObject(); // Retourne un objet vide si ce n'est pas un objet
        }
        return *rootValue_.toObject(); // Retourne l'objet JSON
    }

    /**
     * @brief Retrieves the root of the JSON document as a JSON object.
     *
     * Returns the root value of the document as a SwJsonObject.
     * If the root is not a JSON object, an empty SwJsonObject is returned,
     * and an error message is logged.
     *
     * @return The root value as a SwJsonObject. Returns an empty object if the root is not of type SwJsonObject.
     */
    SwJsonArray array() const {
        if (!rootValue_.isArray()) {
            std::cerr << "Root is not a JSON array." << std::endl;
            return SwJsonArray(); // Retourne un tableau vide si ce n'est pas un tableau
        }
        return *rootValue_.toArray(); // Retourne le tableau JSON
    }

    /**
     * @brief Retrieves the root value of the JSON document as a SwJsonValue.
     *
     * Returns the root value of the document, regardless of its type (object, array, string, etc.).
     *
     * @return The root value of the document as a SwJsonValue.
     */
    SwJsonValue toJsonValue() const {
        return rootValue_; // Retourne directement la valeur racine
    }

    /**
     * @brief Finds or creates a JSON value at the specified path within the document.
     *
     * Traverses the JSON document using a path to locate a specific value.
     * - The path can include nested keys separated by '/'.
     * - If the path is not found, an error is logged, and a default invalid value is returned unless `createIfNotExist` is true.
     * - If `createIfNotExist` is true, missing keys along the path are created as JSON objects.
     *
     * @param rawPath The path to the JSON value, with nested keys separated by '/'.
     * @param createIfNotExist A flag indicating whether to create missing keys as JSON objects (default: false).
     *
     * @return A reference to the located SwJsonValue. If the path is not found and creation is not allowed, returns a default invalid value.
     *
     * @note The path supports replacing '\\' with '/' to accommodate different path formats.
     */
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

    /**
     * @brief Serializes the JSON document to a string.
     *
     * Converts the JSON document into a string representation in either compact or pretty format.
     * - Optionally applies encryption to the serialized string using the provided encryption key.
     *
     * @param format The desired JSON format, either `JsonFormat::Compact` (default) or `JsonFormat::Pretty`.
     * @param encryptionKey An optional key for encrypting the JSON string (default: empty string for no encryption).
     *
     * @return A SwString containing the serialized JSON document.
     */
    SwString toJson(JsonFormat format = JsonFormat::Compact, const SwString& encryptionKey = "") const {
        SwString result;

        generateJson(rootValue_, result, format == JsonFormat::Pretty, 0, encryptionKey);

        return result;
    }

    /**
     * @brief Creates a SwJsonDocument from a JSON string.
     *
     * Parses a JSON string and constructs a SwJsonDocument object.
     * - Optionally decrypts the JSON string using the provided decryption key before parsing.
     *
     * @param jsonString The JSON string to parse.
     * @param decryptionKey An optional key for decrypting the JSON string (default: empty string for no decryption).
     *
     * @return A SwJsonDocument constructed from the parsed JSON string. If parsing fails, an empty document is returned.
     */
    static SwJsonDocument fromJson(const std::string& jsonString, const SwString& decryptionKey = "") {
        SwJsonDocument doc;
        std::string errorMessage;
        doc.loadFromJson(jsonString, errorMessage, decryptionKey);
        return doc;
    }

    /**
     * @brief Loads a JSON document from a JSON string.
     *
     * Parses a JSON string and populates the current SwJsonDocument object.
     * - Optionally decrypts the JSON string using the provided decryption key before parsing.
     * - Validates that no unexpected characters remain after parsing.
     *
     * @param jsonString The JSON string to parse.
     * @param errorMessage A reference to a string that will contain an error message if parsing fails.
     * @param decryptionKey An optional key for decrypting the JSON string (default: empty string for no decryption).
     *
     * @return `true` if the JSON string was successfully parsed and loaded, `false` otherwise.
     *
     * @note On failure, the `errorMessage` parameter provides details about the error.
     */
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

    /**
     * @brief Recursively generates a JSON string from a SwJsonValue.
     *
     * @param value The JSON value to serialize.
     * @param output The resulting JSON string.
     * @param pretty Whether to format the JSON string with indentation.
     * @param indentLevel The current level of indentation for pretty formatting.
     * @param encryptionKey Optional key to encrypt string values.
     */
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

    /**
     * @brief Parses a JSON string into a SwJsonValue.
     *
     * @param jsonString The JSON string to parse.
     * @param index A reference to the current position in the string during parsing.
     * @param decryptionKey Optional key to decrypt string values before parsing.
     *
     * @return The parsed SwJsonValue. Returns an invalid value on error.
     */
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

    /**
     * @brief Parses a JSON object from a JSON string.
     *
     * @param jsonString The JSON string to parse.
     * @param index A reference to the current position in the string during parsing.
     * @param decryptionKey Optional key to decrypt string values before parsing.
     *
     * @return The parsed SwJsonObject wrapped in a SwJsonValue. Returns an invalid value on error.
     */
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

    /**
     * @brief Parses a JSON array from a JSON string.
     *
     * @param jsonString The JSON string to parse.
     * @param index A reference to the current position in the string during parsing.
     * @param decryptionKey Optional key to decrypt string values before parsing.
     *
     * @return The parsed SwJsonArray wrapped in a SwJsonValue. Returns an invalid value on error.
     */
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

    /**
     * @brief Parses a JSON string from a JSON input.
     *
     * @param jsonString The JSON string to parse.
     * @param index A reference to the current position in the string during parsing.
     * @param decryptionKey Optional key to decrypt the parsed string value.
     *
     * @return The parsed SwString. Returns an empty string on error.
     */
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

    /**
     * @brief Parses a JSON number from a JSON string.
     *
     * @param jsonString The JSON string to parse.
     * @param index A reference to the current position in the string during parsing.
     * @param decryptionKey Optional key to decrypt the parsed numeric value.
     *
     * @return The parsed SwJsonValue containing a number. Returns an invalid value on error.
     */
    SwJsonValue parseNumber(const SwString& jsonString, size_t& index, const SwString& decryptionKey = "") const {
        size_t start = index;

        if (jsonString[index] == '-') ++index; // Gérer les nombres négatifs

        while (index < jsonString.size() && (std::isdigit(jsonString[index]) || jsonString[index] == '.')) ++index;

        SwString numberString = jsonString.mid(start, index - start);
        if (!decryptionKey.isEmpty()) numberString = numberString.decryptAES(decryptionKey);

        return numberString.isFloat() ? SwJsonValue(numberString.toFloat())
                                      : SwJsonValue(numberString.toInt());
    }

    /**
     * @brief Parses a JSON literal (e.g., "true", "false", "null") from a JSON string.
     *
     * @param jsonString The JSON string to parse.
     * @param index A reference to the current position in the string during parsing.
     *
     * @return The parsed SwString containing the literal value.
     */
    SwString parseLiteral(const SwString& jsonString, size_t& index) const {
        size_t start = index;
        while (index < jsonString.size() && std::isalpha(static_cast<int>(jsonString[index]))) ++index;
        return jsonString.mid(start, index - start);
    }
};
