#pragma once

#include "SwJsonValue.h"
#include "SwJsonArray.h"

#include <map>
#include <string>
#include <iostream>

class SwJsonObject {
public:
    // Constructeurs
    SwJsonObject() = default;

    // Opérateurs d'accès
    SwJsonValue& operator[](const std::string& key) {
        return data_[key];
    }

    const SwJsonValue& operator[](const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            return it->second;
        }
        static const SwJsonValue nullValue;
        return nullValue;
    }

    // Vérifier si une clé existe
    bool contains(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

    // Ajouter ou mettre à jour une clé
    void insert(const std::string& key, const SwJsonValue& value) {
        data_[key] = value;
    }

    // Supprimer une clé
    bool remove(const std::string& key) {
        return data_.erase(key) > 0;
    }

    // Nombre d'éléments
    size_t size() const {
        return data_.size();
    }

    // Vérifier si l'objet est vide
    bool isEmpty() const {
        return data_.empty();
    }

    // Obtenir toutes les clés
    std::vector<std::string> keys() const {
        std::vector<std::string> keyList;
        for (const auto& pair : data_) {
            keyList.push_back(pair.first);
        }
        return keyList;
    }

    // Obtenir toutes les valeurs
    std::vector<SwJsonValue> values() const {
        std::vector<SwJsonValue> valueList;
        for (const auto& pair : data_) {
            valueList.push_back(pair.second);
        }
        return valueList;
    }

    // Comparaison d'égalité
    bool operator==(const SwJsonObject& other) const {
        return data_ == other.data_;
    }

    bool operator!=(const SwJsonObject& other) const {
        return !(*this == other);
    }

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


    inline std::map<std::string, SwJsonValue> data() const
    {
        return data_;
    }

private:
    std::map<std::string, SwJsonValue> data_; // Stocke les paires clé-valeur
};


