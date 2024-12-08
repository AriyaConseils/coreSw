#ifndef SWLIST_H
#define SWLIST_H

#include <vector>
#include <initializer_list>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <sstream>

template<typename T>
class SwList {
public:
    // Constructeurs
    SwList() = default;
    SwList(const SwList& other) = default;
    SwList(SwList&& other) noexcept = default;
    SwList(std::initializer_list<T> init) : data_(init) {}

    // Destructeur
    ~SwList() = default;

    // Opérateurs
    SwList& operator=(const SwList& other) = default;
    SwList& operator=(SwList&& other) noexcept = default;

    T& operator[](size_t index) {
        return data_[index];
    }

    const T& operator[](size_t index) const {
        return data_[index];
    }

    bool operator==(const SwList& other) const {
        return data_ == other.data_;
    }

    bool operator!=(const SwList& other) const {
        return data_ != other.data_;
    }

    // Méthodes principales
    void append(const T& value) {
        data_.push_back(value);
    }

    void prepend(const T& value) {
        data_.insert(data_.begin(), value);
    }

    void insert(size_t index, const T& value) {
        if (index > data_.size()) {
            throw std::out_of_range("Index out of range");
        }
        data_.insert(data_.begin() + index, value);
    }

    void removeAt(size_t index) {
        if (index >= data_.size()) {
            throw std::out_of_range("Index out of range");
        }
        data_.erase(data_.begin() + index);
    }

    void clear() {
        data_.clear();
    }

    void deleteAll() {
        for (auto& element : data_) {
            delete element;
            element = nullptr;
        }
        data_.clear();
    }

    size_t size() const {
        return data_.size();
    }

    bool isEmpty() const {
        return data_.empty();
    }

    // Accès aux éléments
    T& at(size_t index) {
        if (index >= data_.size()) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    const T& at(size_t index) const {
        if (index >= data_.size()) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    // Itérateurs
    typename std::vector<T>::iterator begin() {
        return data_.begin();
    }

    typename std::vector<T>::iterator end() {
        return data_.end();
    }

    typename std::vector<T>::const_iterator begin() const {
        return data_.begin();
    }

    typename std::vector<T>::const_iterator end() const {
        return data_.end();
    }

    template<typename SeparatorType>
    std::string join(const SeparatorType& delimiter) const {
        if (data_.empty()) return "";

        std::ostringstream oss;
        auto it = data_.begin();
        oss << *it; // Premier élément
        ++it;

        for (; it != data_.end(); ++it) {
            oss << delimiter << *it; // Ajout des délimiteurs
        }
        return oss.str();
    }

    // Renvoie une copie du premier élément
    T first() const {
        if (data_.empty()) {
            throw std::runtime_error("Cannot access first element of an empty container");
        }
        return data_.front(); // Retourne une copie
    }

    // Renvoie une copie du dernier élément
    T last() const {
        if (data_.empty()) {
            throw std::runtime_error("Cannot access last element of an empty container");
        }
        return data_.back(); // Retourne une copie
    }

    // Nouvelles fonctionnalités pour SwList
    T& firstRef() {
        if (data_.empty()) {
            throw std::runtime_error("Cannot access first element of an empty container");
        }
        return data_.front();
    }

    const T& firstRef() const {
        if (data_.empty()) {
            throw std::runtime_error("Cannot access first element of an empty container");
        }
        return data_.front();
    }

    T& lastRef() {
        if (data_.empty()) {
            throw std::runtime_error("Cannot access last element of an empty container");
        }
        return data_.back();
    }

    const T& lastRef() const {
        if (data_.empty()) {
            throw std::runtime_error("Cannot access last element of an empty container");
        }
        return data_.back();
    }


    bool startsWith(const T& value) const {
        return !data_.empty() && data_.front() == value;
    }

    bool endsWith(const T& value) const {
        return !data_.empty() && data_.back() == value;
    }

    SwList<T> mid(size_t index, size_t length = std::string::npos) const {
        if (index >= data_.size()) return SwList<T>(); // Renvoie une liste vide en cas d'erreur

        size_t end = (length == std::string::npos) ? data_.size() : min(index + length, data_.size());
        return SwList<T>(data_.begin() + index, data_.begin() + end);
    }

    void swap(size_t index1, size_t index2) {
        if (index1 < data_.size() && index2 < data_.size()) {
            std::swap(data_[index1], data_[index2]);
        }
    }

    bool contains(const T& value) const {
        return std::find(data_.begin(), data_.end(), value) != data_.end();
    }

    size_t count(const T& value) const {
        return std::count(data_.begin(), data_.end(), value);
    }

    void removeAll(const T& value) {
        data_.erase(std::remove(data_.begin(), data_.end(), value), data_.end());
    }

    bool replace(size_t index, const T& value) {
        if (index < data_.size()) {
            data_[index] = value;
            return true; // Remplacement réussi
        }
        return false; // Remplacement échoué
    }

    int indexOf(const T& value) const {
        auto it = std::find(data_.begin(), data_.end(), value);
        return (it != data_.end()) ? std::distance(data_.begin(), it) : -1; // -1 si non trouvé
    }

    int lastIndexOf(const T& value) const {
        auto it = std::find(data_.rbegin(), data_.rend(), value);
        return (it != data_.rend()) ? std::distance(data_.begin(), it.base()) - 1 : -1; // -1 si non trouvé
    }

private:
    std::vector<T> data_;
};

#endif // SWLIST_H
