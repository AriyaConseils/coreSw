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
public:

    static void registerAllType() {
        static bool isInit = false;
        if(isInit) return;
        isInit = true;
        registerMetaType<SwFont>();
        registerMetaType<SwString>();
        registerMetaType<SwJsonValue>();
        registerMetaType<SwJsonObject>();
        registerMetaType<SwJsonArray>();
        registerMetaType<SwJsonDocument>();
        registerMetaType<DrawTextFormats>();
        registerMetaType<EchoModeEnum>();
    }

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

        registerAllType();

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
        // Enregistrement des types si nécessaire
        registerAllType();
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


public:
    /**
     * @brief Convertit la valeur stockée dans SwAny en un entier.
     *
     * @return int La valeur convertie si le type est int, sinon retourne 0 avec un message d'erreur dans std::cerr.
     */
    int toInt() const {
        return typeNameStr == typeid(int).name()
                   ? get<int>()
                   : (std::cerr << "Error: Not an int. Current type: " << typeNameStr << std::endl, 0);
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un float.
     *
     * @return float La valeur convertie si le type est float, sinon retourne 0.0f avec un message d'erreur dans std::cerr.
     */
    float toFloat() const {
        return typeNameStr == typeid(float).name()
                   ? get<float>()
                   : (std::cerr << "Error: Not a float. Current type: " << typeNameStr << std::endl, 0.0f);
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un double.
     *
     * @return double La valeur convertie si le type est double, sinon retourne 0.0 avec un message d'erreur dans std::cerr.
     */
    double toDouble() const {
        return typeNameStr == typeid(double).name()
                   ? get<double>()
                   : (std::cerr << "Error: Not a double. Current type: " << typeNameStr << std::endl, 0.0);
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un tableau d'octets.
     *
     * @return std::vector<uint8_t> La valeur convertie si le type est std::vector<uint8_t>, sinon retourne un tableau vide avec un message d'erreur dans std::cerr.
     */
    std::vector<uint8_t> toByteArray() const {
        return typeNameStr == typeid(std::vector<uint8_t>).name()
                   ? get<std::vector<uint8_t>>()
                   : (std::cerr << "Error: Not a byte array. Current type: " << typeNameStr << std::endl, std::vector<uint8_t>());
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwFont.
     *
     * @return SwFont La valeur convertie si le type est SwFont, sinon retourne une instance vide avec un message d'erreur dans std::cerr.
     */
    SwFont toSwFont() const {
        return typeNameStr == typeid(SwFont).name()
                   ? get<SwFont>()
                   : (std::cerr << "Error: Not a SwFont. Current type: " << typeNameStr << std::endl, SwFont());
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwString.
     *
     * @return SwString La valeur convertie si le type est SwString, sinon retourne une instance vide avec un message d'erreur dans std::cerr.
     */
    SwString toString() const {
        return typeNameStr == typeid(SwString).name()
                   ? get<SwString>()
                   : (std::cerr << "Error: Not a SwString. Current type: " << typeNameStr << std::endl, SwString());
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwJsonValue.
     *
     * @return SwJsonValue La valeur convertie si le type est SwJsonValue, sinon retourne une instance vide avec un message d'erreur dans std::cerr.
     */
    SwJsonValue toJsonValue() const {
        return typeNameStr == typeid(SwJsonValue).name()
                   ? get<SwJsonValue>()
                   : (std::cerr << "Error: Not a SwJsonValue. Current type: " << typeNameStr << std::endl, SwJsonValue());
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwJsonObject.
     *
     * @return SwJsonObject La valeur convertie si le type est SwJsonObject, sinon retourne une instance vide avec un message d'erreur dans std::cerr.
     */
    SwJsonObject toJsonObject() const {
        return typeNameStr == typeid(SwJsonObject).name()
                   ? get<SwJsonObject>()
                   : (std::cerr << "Error: Not a SwJsonObject. Current type: " << typeNameStr << std::endl, SwJsonObject());
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un SwJsonArray.
     *
     * @return SwJsonArray La valeur convertie si le type est SwJsonArray, sinon retourne une instance vide avec un message d'erreur dans std::cerr.
     */
    SwJsonArray toJsonArray() const {
        return typeNameStr == typeid(SwJsonArray).name()
                   ? get<SwJsonArray>()
                   : (std::cerr << "Error: Not a SwJsonArray. Current type: " << typeNameStr << std::endl, SwJsonArray());
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un DrawTextFormats.
     *
     * @return DrawTextFormats La valeur convertie si le type est DrawTextFormats, sinon retourne une instance vide avec un message d'erreur dans std::cerr.
     */
    DrawTextFormats toDrawTextFormats() const {
        return typeNameStr == typeid(DrawTextFormats).name()
                   ? get<DrawTextFormats>()
                   : (std::cerr << "Error: Not a DrawTextFormats. Current type: " << typeNameStr << std::endl, DrawTextFormats());
    }

    /**
     * @brief Convertit la valeur stockée dans SwAny en un EchoModeEnum.
     *
     * @return EchoModeEnum La valeur convertie si le type est EchoModeEnum, sinon retourne une instance vide avec un message d'erreur dans std::cerr.
     */
    EchoModeEnum toEchoModeEnum() const {
        return typeNameStr == typeid(EchoModeEnum).name()
                   ? get<EchoModeEnum>()
                   : (std::cerr << "Error: Not an EchoModeEnum. Current type: " << typeNameStr << std::endl, EchoModeEnum());
    }


};


