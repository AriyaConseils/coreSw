#ifndef SWMAP_H
#define SWMAP_H
/***************************************************************************************************
 * This file is part of a project developed by Ariya Consulting and Eymeric O'Neill.
 *
 * Copyright (C) [year] Ariya Consulting
 * Author/Creator: Eymeric O'Neill
 * Contact: +33 6 52 83 83 31
 * Email: eymeric.oneill@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ***************************************************************************************************/

#include <map>
#include "SwList.h" // Inclusion de SwList

template<typename Key, typename T>
class SwMap {
public:
    using iterator = typename std::map<Key, T>::iterator;
    using const_iterator = typename std::map<Key, T>::const_iterator;

    // Constructors
    SwMap() = default;
    SwMap(std::initializer_list<std::pair<const Key, T>> initList) : m_map(initList) {}

    template<typename InputIterator>
    SwMap(InputIterator first, InputIterator last) : m_map(first, last) {}

    // Copy & Move Constructors
    SwMap(const SwMap& other) = default;
    SwMap(SwMap&& other) noexcept = default;

    // Assignment Operators
    SwMap& operator=(const SwMap& other) = default;
    SwMap& operator=(SwMap&& other) noexcept = default;
    SwMap& operator=(std::initializer_list<std::pair<const Key, T>> initList) {
        m_map = initList;
        return *this;
    }

    // Element Access
    T& operator[](const Key& key) {
        // Vérifie si la clé existe dans la map
        auto it = m_map.find(key);
        if (it == m_map.end()) {
            // Crée une valeur par défaut si la clé n'existe pas
            m_map[key] = T(); // Appelle le constructeur par défaut de T
        }
        return m_map[key];
    }


    const T& operator[](const Key& key) const {
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            return it->second;
        }

        // Retourner une valeur par défaut statique si la clé n'existe pas
        static const T defaultValue = T(); // Valeur par défaut
        return defaultValue;
    }


    T value(const Key& key, const T& defaultValue = T()) const {
        auto it = m_map.find(key);
        return (it != m_map.end()) ? it->second : defaultValue;
    }

    // Insertion
    void insert(const Key& key, const T& value) {
        m_map[key] = value;
    }

    void insert(const std::pair<Key, T>& pair) {
        m_map.insert(pair);
    }

    template<typename InputIterator>
    void insert(InputIterator first, InputIterator last) {
        m_map.insert(first, last);
    }

    // Removal
    void remove(const Key& key) {
        m_map.erase(key);
    }

    // Queries
    bool contains(const Key& key) const {
        return m_map.find(key) != m_map.end();
    }

    bool isEmpty() const {
        return m_map.empty();
    }

    std::size_t size() const {
        return m_map.size();
    }

    void clear() {
        m_map.clear();
    }

    // Iterators
    iterator begin() {
        return m_map.begin();
    }

    const_iterator begin() const {
        return m_map.begin();
    }

    iterator end() {
        return m_map.end();
    }

    const_iterator end() const {
        return m_map.end();
    }

    // Keys and Values
    SwList<Key> keys() const {
        SwList<Key> result;
        for (const auto& pair : m_map) {
            result.append(pair.first);
        }
        return result;
    }

    SwList<T> values() const {
        SwList<T> result;
        for (const auto& pair : m_map) {
            result.append(pair.second);
        }
        return result;
    }

    SwList<T> values(const Key& key) const {
        SwList<T> result;
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            result.append(it->second);
        }
        return result;
    }

private:
    std::map<Key, T> m_map;
};

#endif // SWMAP_H
