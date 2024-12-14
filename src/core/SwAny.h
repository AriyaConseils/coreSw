#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <vector>
#include <cstring>
#include "SwFont.h"
#include "SwString.h"
#include "SwJsonValue.h"
#include "SwJsonObject.h"
#include "SwJsonArray.h"
#include "SwJsonDocument.h"
#include "Sw.h"
#include <map>
#include <functional>



class SwAny {

protected:
    static bool registerAllTypeOnce() {
        static bool oneCheck = registerAllType();
        return oneCheck;
    }

    static bool registerAllType() {
        std::cerr << "********************CALL ONCE NOT TWICE********************" << std::endl;
        registerMetaType<SwFont>();
        registerMetaType<SwString>();
        registerMetaType<SwJsonValue>();
        registerMetaType<SwJsonObject>();
        registerMetaType<SwJsonArray>();
        registerMetaType<SwJsonDocument>();
        registerMetaType<DrawTextFormats>();
        registerMetaType<EchoModeEnum>();

        SwAny::registerConversion<const char*, SwString>([](const char* cstr) {
            return SwString(cstr);
        });

        // std::string -> SwString
        SwAny::registerConversion<std::string, SwString>([](const std::string& s) {
            return SwString(s);
        });

        // Conversions depuis SwString vers std::string
        SwAny::registerConversion<SwString, std::string>([](const SwString& s) {
            return s.toStdString();
        });

        // Conversion depuis SwString vers const char*
        // On utilise un buffer thread_local pour assurer la validité du pointeur c_str.
        SwAny::registerConversion<SwString, const char*>([](const SwString& s) {
            thread_local static std::string buffer;
            buffer = s.toStdString();
            return buffer.c_str();
        });

        // Conversion depuis SwString vers std::vector<uint8_t> (byte array)
        SwAny::registerConversion<SwString, std::vector<uint8_t>>([](const SwString& s) {
            const std::string& strVal = s.toStdString();
            return std::vector<uint8_t>(strVal.begin(), strVal.end());
        });

        // Conversion depuis SwString vers int
        // Nécessite que le contenu du SwString soit convertible (par ex. "123")
        SwAny::registerConversion<SwString, int>([](const SwString& s) {
            return s.toInt();
        });

        // Conversion depuis SwString vers float
        SwAny::registerConversion<SwString, float>([](const SwString& s) {
            return s.toFloat();
        });

        // Conversion depuis SwString vers double
        SwAny::registerConversion<SwString, double>([](const SwString& s) {
            return static_cast<double>(s.toFloat());
        });
        return true;
    }
    bool registeredType = registerAllTypeOnce();
private:
    // Union pour stocker plusieurs types
    union Storage {
        void* dynamic;
        int i;
        float f;
        double d;
        std::string str;
        std::vector<uint8_t> byteArray;

        // Constructeur et destructeur de l'union
        Storage() : dynamic(nullptr) {}
        ~Storage() {}
    } storage;

public:
    // Constructeurs pour les types de base et complexes
    SwAny(int value) { store(value); }
    SwAny(float value) { store(value); }
    SwAny(double value) { store(value); }
    SwAny(const std::string& value) { store(value); }
    SwAny(const char* value) { store(std::string(value)); }
    SwAny(const std::vector<uint8_t>& value) { store(value); }
    SwAny(const SwString& value) { store(value); }

    SwAny(const SwAny& other) {
        copyFrom(other);
    }

    SwAny() : typeNameStr("") {

    }

    void setTypeName(const std::string& typeName){
        typeNameStr = typeName;
    }

    // Opérateur d'assignation pour copier les valeurs
    SwAny& operator=(const SwAny& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    // Destructeur
    ~SwAny() { clear(); }


    // Méthode statique pour enregistrer une conversion possible entre deux types
    // On demande un lambda ou une fonction qui explique comment convertir From -> To.
    template<typename From, typename To>
    static void registerConversion(std::function<To(const From&)> converterFunc) {
        auto fromName = std::string(typeid(From).name());
        auto toName = std::string(typeid(To).name());

        // Enregistrer dans la map qu'une conversion de fromName vers toName est possible
        getConversionRules()[fromName].push_back(toName);

        // Enregistrer la fonction de conversion dans une autre map
        // Ici on encapsule converterFunc dans un lambda générique prenant un SwAny et retournant un SwAny
        getConverters()[std::make_pair(fromName, toName)] = [converterFunc](const SwAny& any) -> SwAny {
            // On sait que any stocke un From
            From val = any.get<From>();
            To convertedVal = converterFunc(val);
            return SwAny::from(convertedVal);
        };
    }

    // Version avec std::string
    bool canConvert(const std::string& targetName) const {
        // Vérification du type exact
        if (typeNameStr == targetName) {
            return true;
        }
        // Vérification des règles de conversion
        auto& rules = getConversionRules();
        auto it = rules.find(typeNameStr);
        if (it != rules.end()) {
            const auto& targets = it->second;
            for (auto& possibleTarget : targets) {
                if (possibleTarget == targetName) {
                    return true;
                }
            }
        }

        return false;
    }

    // Version template qui appelle la version string
    template<typename T>
    bool canConvert() const {
        auto targetName = std::string(typeid(T).name());
        return canConvert(targetName);
    }


    // Version avec std::string pour la conversion
    SwAny convert(const std::string& targetName) const {
        // Si le type actuel est déjà le bon
        if (typeNameStr == targetName) {
            return *this; // Pas besoin de convertir
        }

        // Vérifions si une règle de conversion existe
        auto& converters = getConverters();
        auto key = std::make_pair(typeNameStr, targetName);
        auto it = converters.find(key);
        if (it != converters.end()) {
            // Appeler la fonction de conversion
            return it->second(*this);
        } else {
            std::cerr << "No conversion rule registered from " << typeNameStr << " to " << targetName << std::endl;
            return SwAny(); // Retourne un SwAny vide si impossible
        }
    }

    // Version template qui appelle la version string
    template<typename T>
    SwAny convert() const {
        auto targetName = std::string(typeid(T).name());
        return convert(targetName);
    }


    // Récupération du type sous forme de string
    std::string typeName() const { return typeNameStr; }

    template <typename T>
    static void registerMetaType() {
        auto typeName = typeid(T).name();

        // Déplacement dynamique
        getDynamicMoveFromMap()[typeName] = [](SwAny& self, SwAny&& other) {
            if (self.storage.dynamic) {
                delete static_cast<T*>(self.storage.dynamic); // Nettoyer si nécessaire
            }
            self.storage.dynamic = other.storage.dynamic; // Déplacer les données
            other.storage.dynamic = nullptr;             // Vider l'ancien stockage
            self.typeNameStr = std::move(other.typeNameStr);
        };

        // Clear dynamique
        getDynamicClearMap()[typeName] = [](SwAny& self) {
            delete static_cast<T*>(self.storage.dynamic);
            self.storage.dynamic = nullptr;
        };

        // CopyFrom dynamique
        getDynamicCopyFromMap()[typeName] = [](SwAny& self, const SwAny& other) {
            self.storage.dynamic = new T(*static_cast<const T*>(other.storage.dynamic));
            self.typeNameStr = other.typeNameStr;
        };

        // Data dynamique
        getDynamicDataMap()[typeName] = [](const SwAny& self) -> void* {
            return const_cast<void*>(reinterpret_cast<const void*>(self.storage.dynamic));
        };

        // FromVoidPtr dynamique
        getDynamicFromVoidPtrMap()[typeName] = [typeName](void* ptr) -> SwAny {
            SwAny any;
            any.setTypeName(typeName);
            T *temp = static_cast<T*>(ptr);
            any.store(*temp);
            return any;
        };
    }


    // Créer une instance depuis un type
    template <typename T>
    static SwAny from(const T& value) {
        SwAny any;
        any.store(value);
        return any;
    }

    static SwAny fromVoidPtr(void* ptr, const std::string& typeNameStr) {
        if (ptr == nullptr || typeNameStr.empty()) {
            std::cerr << "Error: Null pointer or empty type name provided to fromVoidPtr." << std::endl;
            return SwAny(); // Retourner une instance vide si le pointeur ou le type est invalide
        }
        // Gestion des types natifs et standard
        if (typeNameStr == typeid(int).name()) {
            return SwAny(*static_cast<int*>(ptr));
        } else if (typeNameStr == typeid(float).name()) {
            return SwAny(*static_cast<float*>(ptr));
        } else if (typeNameStr == typeid(double).name()) {
            return SwAny(*static_cast<double*>(ptr));
        } else if (typeNameStr == typeid(std::string).name()) {
            return SwAny(*static_cast<std::string*>(ptr));
        } else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            return SwAny(*static_cast<std::vector<uint8_t>*>(ptr));
        }

        // Gestion des types dynamiques
        auto& dynamicMap = getDynamicFromVoidPtrMap();
        auto it = dynamicMap.find(typeNameStr);
        if (it != dynamicMap.end()) {
            return it->second(ptr); // Appel de la fonction dynamique
        }

        // Si le type n'est pas trouvé, afficher un message clair
        std::cerr << "Error: Type '" << typeNameStr << "' not found in dynamic type map.\n"
                  << "Available types in the map:" << std::endl;

        for (const auto& entry : dynamicMap) {
            std::cerr << "  - " << entry.first << std::endl; // Afficher tous les types enregistrés
        }

        // Aucun type trouvé, retourner une instance vide
        return SwAny();
    }



    // Récupérer la valeur avec cast explicite
    template <typename T>
    T get() const {
        if (typeNameStr != typeid(T).name()) {
            std::cerr << "Type mismatch: cannot cast " << typeNameStr
                      << " to " << typeid(T).name() << "." << std::endl;
            return T{}; // Retourne une valeur par défaut du type T
        }
        return *reinterpret_cast<T*>(data());
    }


    // Méthode pour obtenir un pointeur générique vers les données
    void* data() const {
        // Gestion des types natifs
        if (typeNameStr == typeid(int).name()) {
            return const_cast<void*>(static_cast<const void*>(&storage.i));
        } else if (typeNameStr == typeid(float).name()) {
            return const_cast<void*>(static_cast<const void*>(&storage.f));
        } else if (typeNameStr == typeid(double).name()) {
            return const_cast<void*>(static_cast<const void*>(&storage.d));
        } else if (typeNameStr == typeid(std::string).name()) {
            return const_cast<void*>(static_cast<const void*>(&storage.str));
        } else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            return const_cast<void*>(static_cast<const void*>(&storage.byteArray));
        }

        // Gestion des types dynamiques
        auto& dynamicDataMap = getDynamicDataMap();
        auto it = dynamicDataMap.find(typeNameStr);
        if (it != dynamicDataMap.end()) {
            return it->second(*this); // Appel de la fonction pour récupérer les données dynamiques
        }

        // Aucun type trouvé, retourne nullptr
        return nullptr;
    }




    std::string typeNameStr;  // Sauvegarde du nom du type

    void copyFrom(const SwAny& other) {
        // Copier le type du nom
        typeNameStr = other.typeNameStr;

        if (typeNameStr.empty()) {
            return; // Aucun type à copier
        }

        // Gestion des types natifs
        if (typeNameStr == typeid(int).name()) {
            store(other.storage.i);
        } else if (typeNameStr == typeid(float).name()) {
            store(other.storage.f);
        } else if (typeNameStr == typeid(double).name()) {
            store(other.storage.d);
        } else if (typeNameStr == typeid(std::string).name()) {
            store(other.storage.str);
        } else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            store(other.storage.byteArray);
        }
        // Gestion des types dynamiques
        else {
            auto& dynamicCopyMap = getDynamicCopyFromMap();
            auto it = dynamicCopyMap.find(typeNameStr);
            if (it != dynamicCopyMap.end()) {
                it->second(*this, other); // Appel de la fonction dynamique pour copier
            }
            // Copie générique des données dynamiques si aucune fonction n'est définie
            else if (other.storage.dynamic) {
                storage.dynamic = other.storage.dynamic; // Copie directe
            }
        }
    }


    // Méthode pour déplacer les données (move)
    void moveFrom(SwAny&& other) {
        // Déplacer le type du nom
        typeNameStr = std::move(other.typeNameStr);

        if (typeNameStr.empty()) {
            other.clear();
            return;
        }

        // Cas spécifiques pour les types natifs ou gérés explicitement
        if (typeNameStr == typeid(int).name()) {
            storage.i = other.storage.i;
        } else if (typeNameStr == typeid(float).name()) {
            storage.f = other.storage.f;
        } else if (typeNameStr == typeid(double).name()) {
            storage.d = other.storage.d;
        } else if (typeNameStr == typeid(std::string).name()) {
            new (&storage.str) std::string(std::move(other.storage.str));
        } else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            new (&storage.byteArray) std::vector<uint8_t>(std::move(other.storage.byteArray));
        }
        // Gestion des types dynamiques enregistrés
        else {
            auto& dynamicMoveMap = getDynamicMoveFromMap();
            auto it = dynamicMoveMap.find(typeNameStr);
            if (it != dynamicMoveMap.end()) {
                it->second(*this, std::move(other));
            }
            // Déplacement brut pour les types dynamiques non enregistrés
            else if (other.storage.dynamic) {
                storage.dynamic = other.storage.dynamic;
                other.storage.dynamic = nullptr;
            }
        }

        // Réinitialisation de l'objet source
        other.clear();
    }





    // Méthodes pour stocker les données
    template <typename T>
    void store(const T& value) {
        clear();
        storage.dynamic = new T(value);
        typeNameStr = typeid(T).name();
    }
    void store(void* ptr) { clear(); storage.dynamic = ptr; }
    void store(int val) { clear(); storage.i = val; typeNameStr = typeid(int).name(); }
    void store(float val) { clear(); storage.f = val; typeNameStr = typeid(float).name(); }
    void store(double val) { clear(); storage.d = val; typeNameStr = typeid(double).name(); }
    void store(const std::string& val) { clear(); new (&storage.str) std::string(val); typeNameStr = typeid(std::string).name(); }
    void store(const std::vector<uint8_t>& val) { clear(); new (&storage.byteArray) std::vector<uint8_t>(val); typeNameStr = typeid(std::vector<uint8_t>).name(); }

    // Libérer les ressources allouées
    void clear() {
        if (!typeNameStr.empty()) {
            // Utilisation de _dynamicClear si une fonction correspondante existe pour ce type
            auto& dynamicClearMap = getDynamicClearMap(); // Accès à la map _dynamicClear
            auto it = dynamicClearMap.find(typeNameStr);
            if (it != dynamicClearMap.end()) {
                it->second(*this); // Appel de la fonction de destruction dynamique
            } else if (typeNameStr == typeid(std::string).name()) {
                storage.str.~basic_string();
            } else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
                storage.byteArray.~vector();
            }
        }

        // Réinitialisation des données
        storage.dynamic = nullptr;
        typeNameStr.clear();
    }





    static std::map<std::string, std::function<void(SwAny&)>>& getDynamicClearMap() {
        static std::map<std::string, std::function<void(SwAny&)>> _dynamicClear;
        return _dynamicClear;
    }

    // Méthode pour accéder à _dynamicCopyFrom
    static std::map<std::string, std::function<void(SwAny&, const SwAny&)>>& getDynamicCopyFromMap() {
        static std::map<std::string, std::function<void(SwAny&, const SwAny&)>> _dynamicCopyFrom;
        return _dynamicCopyFrom;
    }

    // Méthode pour accéder à _dynamicData
    static std::map<std::string, std::function<void*(const SwAny&)>>& getDynamicDataMap() {
        static std::map<std::string, std::function<void*(const SwAny&)>> _dynamicData;
        return _dynamicData;
    }

    // Méthode pour accéder à _dynamicFromVoidPtr
    static std::map<std::string, std::function<SwAny(void*)>>& getDynamicFromVoidPtrMap() {
        static std::map<std::string, std::function<SwAny(void*)>> _dynamicFromVoidPtr;
        return _dynamicFromVoidPtr;
    }

    // Méthode pour accéder à _dynamicMoveFrom
    static std::map<std::string, std::function<void(SwAny&, SwAny&&)>>& getDynamicMoveFromMap() {
        static std::map<std::string, std::function<void(SwAny&, SwAny&&)>> _dynamicMoveFrom;
        return _dynamicMoveFrom;
    }

    // Map statique : nom du type source -> liste de noms de types cibles
    static std::map<std::string, std::vector<std::string>>& getConversionRules() {
        static std::map<std::string, std::vector<std::string>> conversionRules;
        return conversionRules;
    }

    // Map statique pour stocker les fonctions de conversion
    // Clé : (fromType, toType)
    static std::map<std::pair<std::string, std::string>, std::function<SwAny(const SwAny&)>>& getConverters() {
        static std::map<std::pair<std::string, std::string>, std::function<SwAny(const SwAny&)>> converters;
        return converters;
    }


public:

    /**
 * @brief Convertit la valeur stockée dans SwAny en un entier.
 *
 * Si le type interne n'est pas un entier mais peut être converti en entier
 * (via une règle de conversion préalablement enregistrée), la conversion est tentée.
 *
 * @return int La valeur convertie si possible, sinon retourne 0 avec un message d'erreur dans std::cerr.
 */
    int toInt() const {
        if (typeNameStr == typeid(int).name()) {
            return get<int>();
        } else if (canConvert<int>()) {
            SwAny converted = convert<int>();
            return converted.get<int>();
        } else {
            std::cerr << "Error: Not convertible to int. Current type: " << typeNameStr << std::endl;
            return 0;
        }
    }

    /**
 * @brief Convertit la valeur stockée dans SwAny en un float.
 *
 * Si le type interne n'est pas un float mais peut être converti en float,
 * la conversion est tentée.
 *
 * @return float La valeur convertie si possible, sinon retourne 0.0f avec un message d'erreur dans std::cerr.
 */
    float toFloat() const {
        if (typeNameStr == typeid(float).name()) {
            return get<float>();
        } else if (canConvert<float>()) {
            SwAny converted = convert<float>();
            return converted.get<float>();
        } else {
            std::cerr << "Error: Not convertible to float. Current type: " << typeNameStr << std::endl;
            return 0.0f;
        }
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un double.
     *
     * Si le type interne n'est pas un double mais peut être converti en double,
     * la conversion est tentée.
     *
     * @return double La valeur convertie si possible, sinon retourne 0.0 avec un message d'erreur dans std::cerr.
     */
    double toDouble() const {
        if (typeNameStr == typeid(double).name()) {
            return get<double>();
        } else if (canConvert<double>()) {
            SwAny converted = convert<double>();
            return converted.get<double>();
        } else {
            std::cerr << "Error: Not convertible to double. Current type: " << typeNameStr << std::endl;
            return 0.0;
        }
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un tableau d'octets.
     *
     * Si le type interne n'est pas std::vector<uint8_t> mais peut être converti vers ce type,
     * la conversion est tentée.
     *
     * @return std::vector<uint8_t> La valeur convertie si possible, sinon un tableau vide avec message d'erreur.
     */
    std::vector<uint8_t> toByteArray() const {
        if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            return get<std::vector<uint8_t>>();
        } else if (canConvert<std::vector<uint8_t>>()) {
            SwAny converted = convert<std::vector<uint8_t>>();
            return converted.get<std::vector<uint8_t>>();
        } else {
            std::cerr << "Error: Not convertible to byte array. Current type: " << typeNameStr << std::endl;
            return std::vector<uint8_t>();
        }
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwFont.
     *
     * Si le type interne n'est pas SwFont mais peut être converti en SwFont,
     * la conversion est tentée.
     *
     * @return SwFont La valeur convertie si possible, sinon une instance vide avec message d'erreur.
     */
    SwFont toSwFont() const {
        if (typeNameStr == typeid(SwFont).name()) {
            return get<SwFont>();
        } else if (canConvert<SwFont>()) {
            SwAny converted = convert<SwFont>();
            return converted.get<SwFont>();
        } else {
            std::cerr << "Error: Not convertible to SwFont. Current type: " << typeNameStr << std::endl;
            return SwFont();
        }
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwString.
     *
     * Si le type interne n'est pas SwString mais peut être converti en SwString,
     * la conversion est tentée.
     *
     * @return SwString La valeur convertie si possible, sinon une instance vide avec message d'erreur.
     */
    SwString toString() const {
        if (typeNameStr == typeid(SwString).name()) {
            return get<SwString>();
        } else if (canConvert<SwString>()) {
            SwAny converted = convert<SwString>();
            return converted.get<SwString>();
        } else {
            std::cerr << "Error: Not convertible to SwString. Current type: " << typeNameStr << std::endl;
            return SwString();
        }
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwJsonValue.
     *
     * Si le type interne n'est pas SwJsonValue mais peut être converti en SwJsonValue,
     * la conversion est tentée.
     *
     * @return SwJsonValue La valeur convertie si possible, sinon une instance vide avec message d'erreur.
     */
    SwJsonValue toJsonValue() const {
        if (typeNameStr == typeid(SwJsonValue).name()) {
            return get<SwJsonValue>();
        } else if (canConvert<SwJsonValue>()) {
            SwAny converted = convert<SwJsonValue>();
            return converted.get<SwJsonValue>();
        } else {
            std::cerr << "Error: Not convertible to SwJsonValue. Current type: " << typeNameStr << std::endl;
            return SwJsonValue();
        }
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwJsonObject.
     *
     * Si le type interne n'est pas SwJsonObject mais peut être converti en SwJsonObject,
     * la conversion est tentée.
     *
     * @return SwJsonObject La valeur convertie si possible, sinon une instance vide avec message d'erreur.
     */
    SwJsonObject toJsonObject() const {
        if (typeNameStr == typeid(SwJsonObject).name()) {
            return get<SwJsonObject>();
        } else if (canConvert<SwJsonObject>()) {
            SwAny converted = convert<SwJsonObject>();
            return converted.get<SwJsonObject>();
        } else {
            std::cerr << "Error: Not convertible to SwJsonObject. Current type: " << typeNameStr << std::endl;
            return SwJsonObject();
        }
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwJsonArray.
     *
     * Si le type interne n'est pas SwJsonArray mais peut être converti en SwJsonArray,
     * la conversion est tentée.
     *
     * @return SwJsonArray La valeur convertie si possible, sinon une instance vide avec message d'erreur.
     */
    SwJsonArray toJsonArray() const {
        if (typeNameStr == typeid(SwJsonArray).name()) {
            return get<SwJsonArray>();
        } else if (canConvert<SwJsonArray>()) {
            SwAny converted = convert<SwJsonArray>();
            return converted.get<SwJsonArray>();
        } else {
            std::cerr << "Error: Not convertible to SwJsonArray. Current type: " << typeNameStr << std::endl;
            return SwJsonArray();
        }
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un DrawTextFormats.
     *
     * Si le type interne n'est pas DrawTextFormats mais peut être converti en DrawTextFormats,
     * la conversion est tentée.
     *
     * @return DrawTextFormats La valeur convertie si possible, sinon une instance vide avec message d'erreur.
     */
    DrawTextFormats toDrawTextFormats() const {
        if (typeNameStr == typeid(DrawTextFormats).name()) {
            return get<DrawTextFormats>();
        } else if (canConvert<DrawTextFormats>()) {
            SwAny converted = convert<DrawTextFormats>();
            return converted.get<DrawTextFormats>();
        } else {
            std::cerr << "Error: Not convertible to DrawTextFormats. Current type: " << typeNameStr << std::endl;
            return DrawTextFormats();
        }
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un EchoModeEnum.
     *
     * Si le type interne n'est pas EchoModeEnum mais peut être converti en EchoModeEnum,
     * la conversion est tentée.
     *
     * @return EchoModeEnum La valeur convertie si possible, sinon une instance vide avec message d'erreur.
     */
    EchoModeEnum toEchoModeEnum() const {
        if (typeNameStr == typeid(EchoModeEnum).name()) {
            return get<EchoModeEnum>();
        } else if (canConvert<EchoModeEnum>()) {
            SwAny converted = convert<EchoModeEnum>();
            return converted.get<EchoModeEnum>();
        } else {
            std::cerr << "Error: Not convertible to EchoModeEnum. Current type: " << typeNameStr << std::endl;
            return EchoModeEnum();
        }
    }



};


