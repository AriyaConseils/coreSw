#pragma once

#include "SwJsonValue.h"
#include "SwJsonObject.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>

class SwJsonArray {
public:
    // Constructeurs
    SwJsonArray() = default;

    // Accéder à un élément par index
    SwJsonValue& operator[](size_t index) {
        if (index >= data_.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        return data_[index];
    }

    const SwJsonValue& operator[](size_t index) const {
        if (index >= data_.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        return data_[index];
    }

    // Ajouter une valeur
    void append(const SwJsonValue& value) {
        data_.push_back(value);
    }

    // Insérer une valeur à une position spécifique
    void insert(size_t index, const SwJsonValue& value) {
        if (index > data_.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        data_.insert(data_.begin() + index, value);
    }

    // Supprimer une valeur par index
    void remove(size_t index) {
        if (index >= data_.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        data_.erase(data_.begin() + index);
    }

    // Obtenir la taille du tableau
    size_t size() const {
        return data_.size();
    }

    // Vérifier si le tableau est vide
    bool isEmpty() const {
        return data_.empty();
    }

    // Comparaison d'égalité
    bool operator==(const SwJsonArray& other) const {
        return data_ == other.data_;
    }

    bool operator!=(const SwJsonArray& other) const {
        return !(*this == other);
    }

    std::string toJsonString(bool compact = true, int indentLevel = 0) const {
        std::ostringstream os;
        std::string indent(indentLevel * 2, ' '); // Indentation pour le niveau actuel
        std::string childIndent((indentLevel + 1) * 2, ' '); // Indentation pour les enfants

        os << (compact ? "[" : "[\n");
        for (size_t i = 0; i < data_.size(); ++i) {
            if (i > 0) os << (compact ? "," : ",\n"); // Pas de saut de ligne pour le premier élément

            if (!compact) os << childIndent; // Indenter chaque élément sauf le dernier

            const auto& value = data_[i];
            if (value.isObject() && value.toObject()) {
                os << indent << "{SwJsonValue(Object)}";
            } else if (value.isArray() && value.toArray()) {
                os << value.toArray()->toJsonString(compact, indentLevel + 1);
            } else {
                os << value.toJsonString();
            }
        }

        if (!compact && !data_.empty()) os << "\n" << indent; // Fermer avec indentation uniquement si le tableau n'est pas vide
        os << "]";
        return os.str();
    }

    std::vector<SwJsonValue> data() const
    {
        return data_;
    }

private:
    std::vector<SwJsonValue> data_; // Stocke les valeurs du tableau
};


