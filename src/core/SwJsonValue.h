#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <sstream>

class SwJsonObject;
class SwJsonArray;

/**
 * @class SwJsonValue
 * @brief Represents a versatile JSON value that can hold different types of data.
 */
class SwJsonValue {
public:

    /**
     * @enum Type
     * @brief Enumeration of possible JSON value types.
     */
    enum class Type { Null, Boolean, Integer, Double, String, Object, Array };

    /**
     * @brief Default constructor, initializes the value as Null.
     */
    SwJsonValue() : type_(Type::Null) {}

    /**
     * @brief Constructs a SwJsonValue from a boolean.
     * @param value The boolean value.
     */
    SwJsonValue(bool value) : type_(Type::Boolean), boolValue_(value) {}

    /**
     * @brief Constructs a SwJsonValue from an integer.
     * @param value The integer value.
     */
    SwJsonValue(int value) : type_(Type::Integer), intValue_(value) {}

    /**
     * @brief Constructs a SwJsonValue from a double.
     * @param value The double value.
     */
    SwJsonValue(double value) : type_(Type::Double), doubleValue_(value) {}

    /**
     * @brief Constructs a SwJsonValue from a string.
     * @param value The string value.
     */
    SwJsonValue(const std::string& value) : type_(Type::String), stringValue_(value) {}

    /**
     * @brief Constructs a SwJsonValue from a C-style string.
     * @param value The C-style string value.
     */
    SwJsonValue(const char* value) : type_(Type::String), stringValue_(value ? std::string(value) : "") {}

    /**
     * @brief Constructs a SwJsonValue from a shared pointer to a SwJsonObject.
     * @param value The shared pointer to a JSON object.
     */
    SwJsonValue(std::shared_ptr<SwJsonObject> value) : type_(Type::Object), objectValue_(value) {}

    /**
     * @brief Constructs a SwJsonValue from a shared pointer to a SwJsonArray.
     * @param value The shared pointer to a JSON array.
     */
    SwJsonValue(std::shared_ptr<SwJsonArray> value) : type_(Type::Array), arrayValue_(value) {}

    /**
     * @brief Constructs a SwJsonValue from a SwJsonObject.
     * @param object The JSON object.
     */
    SwJsonValue(const SwJsonObject& object)
        : type_(Type::Object), objectValue_(std::make_shared<SwJsonObject>(object)) {}

    /**
     * @brief Constructs a SwJsonValue from a SwJsonArray.
     * @param array The JSON array.
     */
    SwJsonValue(const SwJsonArray& array)
        : type_(Type::Array), arrayValue_(std::make_shared<SwJsonArray>(array)) {}


    /**
     * @brief Copy constructor.
     * @param other The SwJsonValue to copy.
     */
    SwJsonValue(const SwJsonValue& other)
        : type_(other.type_),
        boolValue_(other.boolValue_),
        intValue_(other.intValue_),
        doubleValue_(other.doubleValue_),
        stringValue_(other.stringValue_),
        objectValue_(other.objectValue_ ? std::make_shared<SwJsonObject>(*other.objectValue_) : nullptr),
        arrayValue_(other.arrayValue_ ? std::make_shared<SwJsonArray>(*other.arrayValue_) : nullptr) {}

    /**
     * @brief Copy assignment operator.
     * @param other The SwJsonValue to assign from.
     * @return A reference to this SwJsonValue.
     */
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

    /**
     * @brief Move constructor.
     * @param other The SwJsonValue to move from.
     */
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

    /**
     * @brief Move assignment operator.
     * @param other The SwJsonValue to assign from.
     * @return A reference to this SwJsonValue.
     */
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

    /**
     * @brief Sets the value to a JSON object.
     * @param object A shared pointer to the JSON object.
     */
    void setObject(std::shared_ptr<SwJsonObject> object) {
        type_ = Type::Object;
        objectValue_ = object;
    }

    /**
     * @brief Sets the value to a JSON array.
     * @param array A shared pointer to the JSON array.
     */
    void setArray(std::shared_ptr<SwJsonArray> array) {
        type_ = Type::Array;
        arrayValue_ = array;
    }




    /**
     * @brief Checks if the value is Null.
     * @return true if the value is Null, false otherwise.
     */
    bool isNull() const { return type_ == Type::Null; }

    /**
     * @brief Checks if the value is a boolean.
     * @return true if the value is a boolean, false otherwise.
     */
    bool isBool() const { return type_ == Type::Boolean; }

    /**
     * @brief Checks if the value is an integer.
     * @return true if the value is an integer, false otherwise.
     */
    bool isInt() const { return type_ == Type::Integer; }

    /**
     * @brief Checks if the value is a double.
     * @return true if the value is a double, false otherwise.
     */
    bool isDouble() const { return type_ == Type::Double; }

    /**
     * @brief Checks if the value is a string.
     * @return true if the value is a string, false otherwise.
     */
    bool isString() const { return type_ == Type::String; }

    /**
     * @brief Checks if the value is a JSON object.
     * @return true if the value is a JSON object, false otherwise.
     */
    bool isObject() const { return type_ == Type::Object; }

    /**
     * @brief Checks if the value is a JSON array.
     * @return true if the value is a JSON array, false otherwise.
     */
    bool isArray() const { return type_ == Type::Array; }

    /**
     * @brief Converts the value to a boolean.
     * @return The boolean representation of the value.
     */
    bool toBool() const {
        if (type_ == Type::Boolean) return boolValue_;
        if (type_ == Type::Integer) return intValue_ != 0;
        if (type_ == Type::Double) return doubleValue_ != 0.0;
        return false;
    }

    /**
     * @brief Converts the value to an integer.
     * @return The integer representation of the value.
     */
    int toInt() const {
        if (type_ == Type::Integer) return intValue_;
        if (type_ == Type::Boolean) return boolValue_ ? 1 : 0;
        if (type_ == Type::Double) return static_cast<int>(doubleValue_);
        return 0;
    }

    /**
     * @brief Converts the value to a double.
     * @return The double representation of the value.
     */
    double toDouble() const {
        if (type_ == Type::Double) return doubleValue_;
        if (type_ == Type::Integer) return static_cast<double>(intValue_);
        if (type_ == Type::Boolean) return boolValue_ ? 1.0 : 0.0;
        return 0.0;
    }

    /**
     * @brief Converts the value to a string.
     * @return The string representation of the value.
     */
    std::string toString() const {
        if (type_ == Type::String) return stringValue_;
        if (type_ == Type::Boolean) return boolValue_ ? "true" : "false";
        if (type_ == Type::Integer) return std::to_string(intValue_);
        if (type_ == Type::Double) return std::to_string(doubleValue_);
        if (type_ == Type::Null) return "null";
        return "{}";  // Pour Object et Array, retourne une structure vide
    }

    /**
     * @brief Converts the value to a shared pointer to a JSON object.
     * @return A shared pointer to the JSON object.
     */
    std::shared_ptr<SwJsonObject> toObject() const {
        if (type_ == Type::Object && objectValue_) {
            return objectValue_;
        }
        return std::make_shared<SwJsonObject>();
    }

    /**
     * @brief Converts the value to a shared pointer to a JSON array.
     * @return A shared pointer to the JSON array.
     */
    std::shared_ptr<SwJsonArray> toArray() const {
        if (type_ == Type::Array && arrayValue_) {
            return arrayValue_;
        }
        return std::make_shared<SwJsonArray>();
    }

    /**
     * @brief Converts the value to a JSON-formatted string.
     * @param compact If true, produces a compact JSON string. Otherwise, formats with indentation.
     * @param indentLevel The current level of indentation for nested structures.
     * @return A JSON-formatted string representation of the value.
     */
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

    /**
     * @brief Compares two SwJsonValue objects for equality.
     * @param other The SwJsonValue to compare to.
     * @return true if the values are equal, false otherwise.
     */
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

    /**
     * @brief Compares two SwJsonValue objects for inequality.
     * @param other The SwJsonValue to compare to.
     * @return true if the values are not equal, false otherwise.
     */
    bool operator!=(const SwJsonValue& other) const {
        return !(*this == other);
    }

    /**
     * @brief Assigns a SwJsonObject to this value.
     * @param object The SwJsonObject to assign.
     * @return A reference to this SwJsonValue.
     */
    SwJsonValue& operator=(const SwJsonObject& object) {
        type_ = Type::Object;
        objectValue_ = std::make_shared<SwJsonObject>(object);
        return *this;
    }


private:
    Type type_; ///< The type of the JSON value.
    bool boolValue_; ///< The boolean value.
    int intValue_; ///< The integer value.
    double doubleValue_; ///< The double value.
    std::string stringValue_; ///< The string value.
    std::shared_ptr<SwJsonObject> objectValue_; ///< The JSON object value.
    std::shared_ptr<SwJsonArray> arrayValue_; ///< The JSON array value.
};
