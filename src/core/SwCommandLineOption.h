#pragma once

#include "SwString.h"
#include "SwList.h"

// Classe SwCommandLineOption pour définir des options de ligne de commande
class SwCommandLineOption {
public:
    // Constructeurs
    SwCommandLineOption(const SwString& name, const SwString& description,
                        const SwString& valueName = SwString(), const SwString& defaultValue = SwString())
        : names(SwList<SwString>() << name),
          description(description),
          valueName(valueName),
          defaultValues(defaultValue.isEmpty() ? SwList<SwString>() : SwList<SwString>() << defaultValue) {}

    SwCommandLineOption(const SwList<SwString>& names, const SwString& description,
                        const SwString& valueName = SwString(), const SwList<SwString>& defaultValues = SwList<SwString>())
        : names(names),
          description(description),
          valueName(valueName),
          defaultValues(defaultValues) {}

    // Ajouter un nom court ou long
    void addName(const SwString& name) {
        if (!names.contains(name)) {
            names.append(name);
        }
    }

    // Obtenir les noms (long et court)
    SwList<SwString> getNames() const {
        return names;
    }

    // Définir une valeur par défaut
    void setDefaultValue(const SwString& value) {
        defaultValues = SwList<SwString>() << value;
    }

    // Définir plusieurs valeurs par défaut (pour les options multiples)
    void setDefaultValues(const SwList<SwString>& values) {
        defaultValues = values;
    }

    // Obtenir la valeur par défaut
    SwList<SwString> getDefaultValues() const {
        return defaultValues;
    }

    // Vérifier si l'option nécessite une valeur
    bool isValueRequired() const {
        return !valueName.isEmpty();
    }

    // Obtenir la description
    SwString getDescription() const {
        return description;
    }

    // Obtenir le nom de la valeur (pour usage dans les messages d'aide)
    SwString getValueName() const {
        return valueName;
    }

private:
    SwList<SwString> names;            // Liste des noms (longs et courts)
    SwString description;              // Description pour le texte d'aide
    SwString valueName;                // Nom de la valeur attendue (si applicable)
    SwList<SwString> defaultValues;    // Valeurs par défaut (peut être vide)
};

