#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <vector>
#include <cstring>
#include "SwFont.h"
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
        SWANY_DECLARE_TYPE(DrawTextFormats)
        SWANY_DECLARE_TYPE(EchoModeEnum)

    static void registerAllType() {
        SWANY_REGISTER_TYPE(SwFont)
            SWANY_REGISTER_TYPE(DrawTextFormats)
            SWANY_REGISTER_TYPE(EchoModeEnum)
    }

private:
    // Union pour stocker plusieurs types
    union Storage {
        int i;
        float f;
        double d;
        std::string str;
        std::wstring wstr;
        std::vector<uint8_t> byteArray;
        SwFont SwFont;
        DrawTextFormats DrawTextFormats;
        EchoModeEnum EchoModeEnum;

        // Constructeur et destructeur de l'union
        Storage() {}
        ~Storage() {}
    } storage;

public:
    // Constructeurs pour les types de base et complexes
    SwAny(int value) { store(value); }
    SwAny(float value) { store(value); }
    SwAny(double value) { store(value); }
    SwAny(const std::string& value) { store(value); }
    SwAny(const std::wstring& value) { store(value); }
    SwAny(const char* value) { store(std::string(value)); }
    SwAny(const wchar_t* value) { store(std::wstring(value)); }
    SwAny(const std::vector<uint8_t>& value) { store(value); }
    //SwAny(const SwFont& value) { store(value); }


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
        else if (typeNameStr == typeid(std::wstring).name()) {
            return SwAny(*static_cast<std::wstring*>(ptr));
        }
        else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            return SwAny(*static_cast<std::vector<uint8_t>*>(ptr));
        }
        else if (_actionFromVoidPtr.find(typeNameStr) != _actionFromVoidPtr.end()) {
            return _actionFromVoidPtr[typeNameStr](ptr);
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
        else if (typeNameStr == typeid(std::wstring).name()) {
            return static_cast<void*>(const_cast<std::wstring*>(&storage.wstr));
        }
        else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            return static_cast<void*>(const_cast<std::vector<uint8_t>*>(&storage.byteArray));
        }
        else if (_actionData.find(typeNameStr) != _actionData.end()) {
            return _actionData[typeNameStr](*this);
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
        else if (typeNameStr == typeid(std::wstring).name()) {
            store(other.storage.wstr);
        }
        else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            store(other.storage.byteArray);
        }
        else if (_actionCopyFrom.find(typeNameStr) != _actionCopyFrom.end()) {
            _actionCopyFrom[typeNameStr](*this, other);
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
        else if (typeNameStr == typeid(std::wstring).name()) {
            new (&storage.wstr) std::wstring(std::move(other.storage.wstr));
        }
        else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            new (&storage.byteArray) std::vector<uint8_t>(std::move(other.storage.byteArray));
        }
        if (_actionMoveFrom.find(typeNameStr) != _actionMoveFrom.end()) {
            _actionMoveFrom[typeNameStr](*this, std::move(other));
        }
        else {

        }
        other.clear();
    }


  

    // Méthodes pour stocker les données
    void store(int val) { clear(); storage.i = val; typeNameStr = typeid(int).name(); }
    void store(float val) { clear(); storage.f = val; typeNameStr = typeid(float).name(); }
    void store(double val) { clear(); storage.d = val; typeNameStr = typeid(double).name(); }
    void store(const std::string& val) { clear(); new (&storage.str) std::string(val); typeNameStr = typeid(std::string).name(); }
    void store(const std::wstring& val) { clear(); new (&storage.wstr) std::wstring(val); typeNameStr = typeid(std::wstring).name(); }
    void store(const std::vector<uint8_t>& val) { clear(); new (&storage.byteArray) std::vector<uint8_t>(val); typeNameStr = typeid(std::vector<uint8_t>).name(); }
    //void store(const SwFont& val) { clear(); new (&storage.font) SwFont(val); typeNameStr = typeid(SwFont).name(); }

    // Libérer les ressources allouées
    void clear() {
        if (typeNameStr == typeid(std::string).name()) {
            storage.str.~basic_string();
        }
        else if (typeNameStr == typeid(std::wstring).name()) {
            storage.wstr.~basic_string();
        }
        else if (typeNameStr == typeid(std::vector<uint8_t>).name()) {
            storage.byteArray.~vector();
        }
        if (_actionClear.find(typeNameStr) != _actionClear.end()) {
            _actionClear[typeNameStr](*this);
        }
        else {

        }
        typeNameStr.clear();
    }






    // Maps pour stocker les actions par type
    static std::map<std::string, std::function<void(SwAny&, SwAny&&)>> _actionMoveFrom;
    static std::map<std::string, std::function<void(SwAny&)>> _actionClear;
    static std::map<std::string, std::function<void(SwAny&, const SwAny&)>> _actionCopyFrom;
    static std::map<std::string, std::function<void* (const SwAny&)>> _actionData;
    static std::map<std::string, std::function<SwAny(void*)>> _actionFromVoidPtr;
};


