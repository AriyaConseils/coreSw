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

#include "SwJsonValue.h"

#include <map>
#include <string>

/**
 * @class SwJsonObject
 * @brief Represents a JSON object with key-value pairs.
 */
class SwJsonObject {
public:

    /**
     * @brief Default constructor for SwJsonObject.
     */
    SwJsonObject() = default;

    /**
     * @brief Provides mutable access to the value associated with the given key.
     *
     * @param key The key to access.
     * @return A reference to the SwJsonValue associated with the key.
     */
    SwJsonValue& operator[](const std::string& key) {
        return data_[key];
    }

    /**
     * @brief Provides read-only access to the value associated with the given key.
     *
     * @param key The key to access.
     * @return A const reference to the SwJsonValue associated with the key. Returns a null value if the key does not exist.
     */
    const SwJsonValue& operator[](const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            return it->second;
        }
        static const SwJsonValue nullValue;
        return nullValue;
    }

    /**
     * @brief Checks if the object contains the specified key.
     *
     * @param key The key to check.
     * @return true if the key exists, false otherwise.
     */
    bool contains(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

    /**
     * @brief Inserts or updates a key-value pair in the JSON object.
     *
     * @param key The key to insert or update.
     * @param value The value to associate with the key.
     */
    void insert(const std::string& key, const SwJsonValue& value) {
        data_[key] = value;
    }

    /**
     * @brief Removes a key-value pair from the JSON object.
     *
     * @param key The key to remove.
     * @return true if the key was successfully removed, false otherwise.
     */
    bool remove(const std::string& key) {
        return data_.erase(key) > 0;
    }

    /**
     * @brief Retrieves the number of key-value pairs in the JSON object.
     *
     * @return The number of key-value pairs.
     */
    size_t size() const {
        return data_.size();
    }

    /**
     * @brief Checks if the JSON object is empty.
     *
     * @return true if the object is empty, false otherwise.
     */
    bool isEmpty() const {
        return data_.empty();
    }

    /**
     * @brief Retrieves a list of all keys in the JSON object.
     *
     * @return A vector containing all keys.
     */
    std::vector<std::string> keys() const {
        std::vector<std::string> keyList;
        for (const auto& pair : data_) {
            keyList.push_back(pair.first);
        }
        return keyList;
    }

    /**
     * @brief Retrieves a list of all values in the JSON object.
     *
     * @return A vector containing all values as SwJsonValue.
     */
    std::vector<SwJsonValue> values() const {
        std::vector<SwJsonValue> valueList;
        for (const auto& pair : data_) {
            valueList.push_back(pair.second);
        }
        return valueList;
    }

    /**
     * @brief Compares two JSON objects for equality.
     *
     * @param other The other JSON object to compare.
     * @return true if the two objects are equal, false otherwise.
     */
    bool operator==(const SwJsonObject& other) const {
        return data_ == other.data_;
    }

    /**
     * @brief Compares two JSON objects for inequality.
     *
     * @param other The other JSON object to compare.
     * @return true if the two objects are not equal, false otherwise.
     */
    bool operator!=(const SwJsonObject& other) const {
        return !(*this == other);
    }

    /**
     * @brief Converts the JSON object to a JSON-formatted string.
     *
     * @param compact If true, produces a compact JSON string. Otherwise, formats with indentation.
     * @param indentLevel The current level of indentation for nested objects.
     * @return A JSON-formatted string representation of the object.
     */
    std::string toJsonString(bool compact = true, int indentLevel = 0) const {
        std::ostringstream os;
        std::string indent(indentLevel * 2, ' '); // Indentation basée sur le niveau actuel
        std::string childIndent((indentLevel + 1) * 2, ' '); // Indentation pour les enfants

        os << "{";

        bool first = true;
        for (const auto& pair : data_) {
            if (!first) os << (compact ? "," : ",\n"); // Ajouter une virgule entre les éléments
            first = false;

            if (!compact) os << "\n" << childIndent; // Indenter pour chaque clé

            // Ajouter la clé
            os << "\"" << pair.first << "\": ";

            // Ajouter la valeur
            if (pair.second.isObject() && pair.second.toObject()) {
                os << pair.second.toObject()->toJsonString(compact, indentLevel + 1);
            } else if (pair.second.isArray() && pair.second.toArray()) {
                os << "[SwJsonValue(Array)]";
            } else {
                os << pair.second.toJsonString();
            }
        }

        if (!compact && !data_.empty()) os << "\n" << indent;
        os << "}";
        return os.str();
    }

    /**
     * @brief Retrieves the underlying data as a map of key-value pairs.
     *
     * @return A map of strings to SwJsonValue objects.
     */
    inline std::map<std::string, SwJsonValue> data() const
    {
        return data_;
    }

private:
    std::map<std::string, SwJsonValue> data_; ///< Stores key-value pairs in the JSON object.
};


