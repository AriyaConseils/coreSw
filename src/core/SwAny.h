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




#define SWANY_REGISTER_TYPE(Type) \
        SwAny::register##Type##Type();
        
#define SWANY_DECLARE_TYPE(Type)                                        \
    SwAny(const Type& value) { store(value); }                          \
    void store(const Type& val) {                                       \
        clear();                                                        \
        new (&storage.Type) Type(val);                                  \
        typeNameStr = typeid(Type).name();                              \
    }                                                                   \
    static void register##Type##Type() {                                \
        SwAny::_actionMoveFrom[typeid(Type).name()] = [](SwAny& self, SwAny&& other) {  \
            new (&self.storage.Type) Type(std::move(other.storage.Type));        \
        };                                                              \
        SwAny::_actionClear[typeid(Type).name()] = [](SwAny& self) {    \
            self.storage.Type.~Type();                                  \
        };                                                              \
        SwAny::_actionCopyFrom[typeid(Type).name()] = [](SwAny& self, const SwAny& other) { \
            self.store(other.storage.Type);                             \
        };                                                              \
        SwAny::_actionData[typeid(Type).name()] = [](const SwAny& self) -> void* { \
            return static_cast<void*>(const_cast<Type*>(&self.storage.Type)); \
        };                                                              \
        SwAny::_actionFromVoidPtr[typeid(Type).name()] = [](void* ptr) -> SwAny { \
            return SwAny(*static_cast<Type*>(ptr));                     \
        };                                                              \
    }                                                                   \
    struct register##Type##TypeHelper {                                 \
        register##Type##TypeHelper() {                                  \
            SwAny::register##Type##Type();                              \
        }                                                               \
    } static _register##Type##TypeHelperInstance;









class SwAny {
public:

    SWANY_DECLARE_TYPE(SwFont)
    SWANY_DECLARE_TYPE(SwString)
    SWANY_DECLARE_TYPE(SwJsonValue)
    SWANY_DECLARE_TYPE(SwJsonObject)
    SWANY_DECLARE_TYPE(SwJsonArray)
    SWANY_DECLARE_TYPE(DrawTextFormats)
    SWANY_DECLARE_TYPE(EchoModeEnum)

    static void registerAllType() {
        static bool isInit = false;
        if(isInit) return;
        SWANY_REGISTER_TYPE(SwFont)
        SWANY_REGISTER_TYPE(SwString)
        SWANY_REGISTER_TYPE(SwJsonValue)
        SWANY_REGISTER_TYPE(SwJsonObject)
        SWANY_REGISTER_TYPE(SwJsonArray)
        SWANY_REGISTER_TYPE(DrawTextFormats)
        SWANY_REGISTER_TYPE(EchoModeEnum)
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
        SwFont SwFont;
        SwString SwString;
        SwJsonValue SwJsonValue;
        SwJsonObject SwJsonObject;
        SwJsonArray SwJsonArray;
        DrawTextFormats DrawTextFormats;
        EchoModeEnum EchoModeEnum;

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
        copyFrom(other);  // Utiliser la méthode copyFrom pour copier l'objet
    }

    SwAny() : typeNameStr("") {

    }

    // Opérateur d'assignation pour copier les valeurs
    SwAny& operator=(const SwAny& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    // Opérateur d'assignation par déplacement (move)
    SwAny& operator=(SwAny&& other) noexcept {
        if (this != &other) {
            clear();
            moveFrom(std::move(other));
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
        _dynamicMoveFrom[typeName] = [](SwAny& self, SwAny&& other) {
            if (self.storage.dynamic) {
                delete static_cast<T*>(self.storage.dynamic); // Nettoyer si nécessaire
            }
            self.storage.dynamic = other.storage.dynamic; // Déplacer les données
            other.storage.dynamic = nullptr;             // Vider l'ancien stockage
            self.typeNameStr = std::move(other.typeNameStr);
        };

        // Clear dynamique
        _dynamicClear[typeName] = [](SwAny& self) {
            delete static_cast<T*>(self.storage.dynamic);
            self.storage.dynamic = nullptr;
        };

        // CopyFrom dynamique
        _dynamicCopyFrom[typeName] = [](SwAny& self, const SwAny& other) {
            self.storage.dynamic = new T(*static_cast<const T*>(other.storage.dynamic));
            self.typeNameStr = other.typeNameStr;
        };

        // Data dynamique
        _dynamicData[typeName] = [](const SwAny& self) -> void* {
            return const_cast<void*>(reinterpret_cast<const void*>(self.storage.dynamic));
        };

        // FromVoidPtr dynamique
        _dynamicFromVoidPtr[typeName] = [](void* ptr) -> SwAny {
            SwAny any;
            any.store(static_cast<T*>(ptr));
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
        if (typeNameStr == typeid(int).name()) {
            return SwAny(*static_cast<int*>(ptr));
        }
        else if (typeNameStr == typeid(float).name()) {
            return SwAny(*static_cast<float*>(ptr));
        }
        else if (typeNameStr == typeid(double).name()) {
            return SwAny(*static_cast<double*>(ptr));
        }
        else if (typeNameStr == typeid(std::string).name()) {
            return SwAny(*static_cast<std::string*>(ptr));
        }
        else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            return SwAny(*static_cast<std::vector<uint8_t>*>(ptr));
        }
        else if (_actionFromVoidPtr.find(typeNameStr) != _actionFromVoidPtr.end()) {
            return _actionFromVoidPtr[typeNameStr](ptr);
        }
        else if (_dynamicFromVoidPtr.find(typeNameStr) != _dynamicFromVoidPtr.end()) {
            return _dynamicFromVoidPtr[typeNameStr](ptr);
        }
        else {
            return SwAny();
        }
    }

    // Récupérer la valeur avec cast explicite
    template <typename T>
    T get() const {
        if (typeNameStr != typeid(T).name()) {
            throw std::runtime_error("Type mismatch: cannot cast " + typeNameStr + " to " + typeid(T).name());
        }
        return *reinterpret_cast<T*>(data());
    }

    // Méthode pour obtenir un pointeur générique vers les données
    void* data() const {
        registerAllType();
        if (typeNameStr == typeid(int).name()) {
            return static_cast<void*>(const_cast<int*>(&storage.i));
        }
        else if (typeNameStr == typeid(float).name()) {
            return static_cast<void*>(const_cast<float*>(&storage.f));
        }
        else if (typeNameStr == typeid(double).name()) {
            return static_cast<void*>(const_cast<double*>(&storage.d));
        }
        else if (typeNameStr == typeid(std::string).name()) {
            return static_cast<void*>(const_cast<std::string*>(&storage.str));
        }
        else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            return static_cast<void*>(const_cast<std::vector<uint8_t>*>(&storage.byteArray));
        }
        else if (_actionData.find(typeNameStr) != _actionData.end()) {
            return _actionData[typeNameStr](*this);
        }
        else if (_dynamicData.find(typeNameStr) != _dynamicData.end()) {
            return _dynamicData.at(typeNameStr)(*this);
        }
        else {
            return nullptr;
        }
    }




    std::string typeNameStr;  // Sauvegarde du nom du type

    // Méthode pour copier les données
    void copyFrom(const SwAny& other) {
        typeNameStr = other.typeNameStr;
        if (typeNameStr == typeid(int).name()) {
            store(other.storage.i);
        }
        else if (typeNameStr == typeid(float).name()) {
            store(other.storage.f);
        }
        else if (typeNameStr == typeid(double).name()) {
            store(other.storage.d);
        }
        else if (typeNameStr == typeid(std::string).name()) {
            store(other.storage.str);
        }
        else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            store(other.storage.byteArray);
        }
        else if (_actionCopyFrom.find(typeNameStr) != _actionCopyFrom.end()) {
            _actionCopyFrom[typeNameStr](*this, other);
        }
        else if (_dynamicCopyFrom.find(typeNameStr) != _dynamicCopyFrom.end()) {
            _dynamicCopyFrom[typeNameStr](*this, other); // Appel de la fonction dynamique pour copier
        }
        else if (other.storage.dynamic) {
            // Si aucune fonction n'est définie, effectuer une copie générique des données dynamiques
            storage.dynamic = other.storage.dynamic;
        }
        else {

        }
    }

    // Méthode pour déplacer les données (move)
    void moveFrom(SwAny&& other) {
        typeNameStr = std::move(other.typeNameStr);
        if (typeNameStr == typeid(int).name()) {
            storage.i = other.storage.i;
        }
        else if (typeNameStr == typeid(float).name()) {
            storage.f = other.storage.f;
        }
        else if (typeNameStr == typeid(double).name()) {
            storage.d = other.storage.d;
        }
        else if (typeNameStr == typeid(std::string).name()) {
            new (&storage.str) std::string(std::move(other.storage.str));
        }
        else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            new (&storage.byteArray) std::vector<uint8_t>(std::move(other.storage.byteArray));
        }
        if (_actionMoveFrom.find(typeNameStr) != _actionMoveFrom.end()) {
            _actionMoveFrom[typeNameStr](*this, std::move(other));
        }
        else if (_dynamicMoveFrom.find(typeNameStr) != _dynamicMoveFrom.end()) {
            _dynamicMoveFrom[typeNameStr](*this, std::move(other));
        }
        else if (other.storage.dynamic) {
            // Déplacement brut des données dynamiques non enregistrées
            storage.dynamic = other.storage.dynamic;
            other.storage.dynamic = nullptr;
        }
        else {

        }
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
    //void store(const SwFont& val) { clear(); new (&storage.font) SwFont(val); typeNameStr = typeid(SwFont).name(); }

    // Libérer les ressources allouées
    void clear() {
        if (_actionClear.find(typeNameStr) != _actionClear.end()) {
            _actionClear[typeNameStr](*this);
        } else if (typeNameStr == typeid(std::string).name()) {
            storage.str.~basic_string();
        } else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            storage.byteArray.~vector();
        } else if (_dynamicClear.find(typeNameStr) != _dynamicClear.end()) {
            _dynamicClear[typeNameStr](*this); // Passer l'objet SwAny
            storage.dynamic = nullptr; // Remettre le pointeur à nul
        }
        typeNameStr.clear();
    }







    // Maps pour stocker les actions par type
    static std::map<std::string, std::function<void(SwAny&, SwAny&&)>> _actionMoveFrom;
    static std::map<std::string, std::function<void(SwAny&)>> _actionClear;
    static std::map<std::string, std::function<void(SwAny&, const SwAny&)>> _actionCopyFrom;
    static std::map<std::string, std::function<void* (const SwAny&)>> _actionData;
    static std::map<std::string, std::function<SwAny(void*)>> _actionFromVoidPtr;

    // Maps pour les types dynamiques
    static std::map<std::string, std::function<void(SwAny&)>> _dynamicClear;
    static std::map<std::string, std::function<void(SwAny&, const SwAny&)>> _dynamicCopyFrom;
    static std::map<std::string, std::function<void*(const SwAny&)>> _dynamicData;
    static std::map<std::string, std::function<SwAny(void*)>> _dynamicFromVoidPtr;
    static std::map<std::string, std::function<void(SwAny&, SwAny&&)>> _dynamicMoveFrom;




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
    SwString toSwString() const {
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


