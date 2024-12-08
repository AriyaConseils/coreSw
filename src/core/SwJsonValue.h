#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>

class SwJsonObject;
class SwJsonArray;

class SwJsonValue {
public:
    enum class Type { Null, Boolean, Integer, Double, String, Object, Array };

    SwJsonValue() : type_(Type::Null) {}
    SwJsonValue(bool value) : type_(Type::Boolean), boolValue_(value) {}
    SwJsonValue(int value) : type_(Type::Integer), intValue_(value) {}
    SwJsonValue(double value) : type_(Type::Double), doubleValue_(value) {}
    SwJsonValue(const std::string& value) : type_(Type::String), stringValue_(value) {}
    SwJsonValue(const char* value) : type_(Type::String), stringValue_(value ? std::string(value) : "") {}
    SwJsonValue(std::shared_ptr<SwJsonObject> value) : type_(Type::Object), objectValue_(value) {}
    SwJsonValue(std::shared_ptr<SwJsonArray> value) : type_(Type::Array), arrayValue_(value) {}
    SwJsonValue(const SwJsonObject& object)
        : type_(Type::Object), objectValue_(std::make_shared<SwJsonObject>(object)) {}
    SwJsonValue(const SwJsonArray& array)
        : type_(Type::Array), arrayValue_(std::make_shared<SwJsonArray>(array)) {}


    // Constructeur de copie
    SwJsonValue(const SwJsonValue& other)
        : type_(other.type_),
        boolValue_(other.boolValue_),
        intValue_(other.intValue_),
        doubleValue_(other.doubleValue_),
        stringValue_(other.stringValue_),
        objectValue_(other.objectValue_ ? std::make_shared<SwJsonObject>(*other.objectValue_) : nullptr),
        arrayValue_(other.arrayValue_ ? std::make_shared<SwJsonArray>(*other.arrayValue_) : nullptr) {}

    // Opérateur d'affectation par copie
    SwJsonValue& operator=(const SwJsonValue& other) {
        if (this != &other) {
            type_ = other.type_;
            boolValue_ = other.boolValue_;
            intValue_ = other.intValue_;
            doubleValue_ = other.doubleValue_;
            stringValue_ = other.stringValue_;
            objectValue_ = other.objectValue_ ? std::make_shared<SwJsonObject>(*other.objectValue_) : nullptr;
            arrayValue_ = other.arrayValue_ ? std::make_shared<SwJsonArray>(*other.arrayValue_) : nullptr;
        }
        return *this;
    }

    // Constructeur de déplacement
    SwJsonValue(SwJsonValue&& other) noexcept
        : type_(std::move(other.type_)),
        boolValue_(std::move(other.boolValue_)),
        intValue_(std::move(other.intValue_)),
        doubleValue_(std::move(other.doubleValue_)),
        stringValue_(std::move(other.stringValue_)),
        objectValue_(std::move(other.objectValue_)),
        arrayValue_(std::move(other.arrayValue_)) {
        other.type_ = Type::Null;
    }

    // Opérateur d'affectation par déplacement
    SwJsonValue& operator=(SwJsonValue&& other) noexcept {
        if (this != &other) {
            type_ = std::move(other.type_);
            boolValue_ = std::move(other.boolValue_);
            intValue_ = std::move(other.intValue_);
            doubleValue_ = std::move(other.doubleValue_);
            stringValue_ = std::move(other.stringValue_);
            objectValue_ = std::move(other.objectValue_);
            arrayValue_ = std::move(other.arrayValue_);
            other.type_ = Type::Null;
        }
        return *this;
    }

    void setObject(std::shared_ptr<SwJsonObject> object) {
        type_ = Type::Object;
        objectValue_ = object;
    }


    void setArray(std::shared_ptr<SwJsonArray> array) {
        type_ = Type::Array;
        arrayValue_ = array;
    }




    // Méthodes pour tester le type
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Boolean; }
    bool isInt() const { return type_ == Type::Integer; }
    bool isDouble() const { return type_ == Type::Double; }
    bool isString() const { return type_ == Type::String; }
    bool isObject() const { return type_ == Type::Object; }
    bool isArray() const { return type_ == Type::Array; }

    // Accesseurs typés avec conversion
    bool toBool() const {
        if (type_ == Type::Boolean) return boolValue_;
        if (type_ == Type::Integer) return intValue_ != 0;
        if (type_ == Type::Double) return doubleValue_ != 0.0;
        return false;
    }

    int toInt() const {
        if (type_ == Type::Integer) return intValue_;
        if (type_ == Type::Boolean) return boolValue_ ? 1 : 0;
        if (type_ == Type::Double) return static_cast<int>(doubleValue_);
        return 0;
    }

    double toDouble() const {
        if (type_ == Type::Double) return doubleValue_;
        if (type_ == Type::Integer) return static_cast<double>(intValue_);
        if (type_ == Type::Boolean) return boolValue_ ? 1.0 : 0.0;
        return 0.0;
    }

    std::string toString() const {
        if (type_ == Type::String) return stringValue_;
        if (type_ == Type::Boolean) return boolValue_ ? "true" : "false";
        if (type_ == Type::Integer) return std::to_string(intValue_);
        if (type_ == Type::Double) return std::to_string(doubleValue_);
        if (type_ == Type::Null) return "null";
        return "{}";  // Pour Object et Array, retourne une structure vide
    }

    std::shared_ptr<SwJsonObject> toObject() const {
        if (type_ == Type::Object && objectValue_) {
            return objectValue_;
        }
        return std::make_shared<SwJsonObject>();
    }

    std::shared_ptr<SwJsonArray> toArray() const {
        if (type_ == Type::Array && arrayValue_) {
            return arrayValue_;
        }
        return std::make_shared<SwJsonArray>();
    }

    std::string toJsonString(bool compact = true, int indentLevel = 0) const {
        std::ostringstream os;
        std::string indent(indentLevel * 2, ' '); // Indentation pour le niveau actuel

        if (type_ == Type::String) os << "\"" << stringValue_ << "\"";
        else if (type_ == Type::Boolean) os << (boolValue_ ? "true" : "false");
        else if (type_ == Type::Integer) os << intValue_;
        else if (type_ == Type::Double) os << doubleValue_;
        else if (type_ == Type::Null) os << "null";
        else if (type_ == Type::Object) os << "SwJsonValue(Object)";
        else if (type_ == Type::Array) os << "SwJsonValue(Array)";
        else os << "null"; // Par défaut

        return os.str();
    }


    // Comparaisons
    bool operator==(const SwJsonValue& other) const {
        if (type_ != other.type_) return false;
        switch (type_) {
        case Type::Null: return true;
        case Type::Boolean: return boolValue_ == other.boolValue_;
        case Type::Integer: return intValue_ == other.intValue_;
        case Type::Double: return doubleValue_ == other.doubleValue_;
        case Type::String: return stringValue_ == other.stringValue_;
        case Type::Object: return objectValue_ == other.objectValue_;
        case Type::Array: return arrayValue_ == other.arrayValue_;
        }
        return false;
    }

    bool operator!=(const SwJsonValue& other) const {
        return !(*this == other);
    }

    SwJsonValue& operator=(const SwJsonObject& object) {
        type_ = Type::Object;
        objectValue_ = std::make_shared<SwJsonObject>(object);
        return *this;
    }


private:
    Type type_;
    bool boolValue_ = false;
    int intValue_ = 0;
    double doubleValue_ = 0.0;
    std::string stringValue_;
    std::shared_ptr<SwJsonObject> objectValue_;
    std::shared_ptr<SwJsonArray> arrayValue_;
};
