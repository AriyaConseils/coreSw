#pragma once
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

#include <ctime>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <chrono>

class SwDateTime {
public:
    // Constructeurs
    SwDateTime()
        : tp_(std::chrono::system_clock::now()) {} // Date/heure actuelle

    SwDateTime(std::time_t time)
        : tp_(std::chrono::system_clock::from_time_t(time)) {} // Constructeur avec std::time_t

    SwDateTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0) {
        setDateTime(year, month, day, hour, minute, second);
    }

    // Constructeur de copie
    SwDateTime(const SwDateTime& other) = default;

    // Constructeur de déplacement
    SwDateTime(SwDateTime&& other) noexcept = default;

    // Opérateur d'affectation (copie)
    SwDateTime& operator=(const SwDateTime& other) = default;

    // Opérateur d'affectation (déplacement)
    SwDateTime& operator=(SwDateTime&& other) noexcept = default;

    // Destructeur
    ~SwDateTime() = default;

    operator std::time_t() const {
        return std::chrono::system_clock::to_time_t(tp_);
    }

    operator std::time_t&() {
        // Cette conversion n'a pas trop de sens dans le contexte chrono,
        // mais on va contourner en stockant un statique local.
        static thread_local std::time_t temp = std::chrono::system_clock::to_time_t(tp_);
        return temp;
    }

    operator const std::time_t&() const {
        static thread_local std::time_t temp = std::chrono::system_clock::to_time_t(tp_);
        return temp;
    }

    static SwDateTime currentDateTime() {
        return SwDateTime(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    }

    int msecsTo(const SwDateTime& other) const {
        auto diff = other.tp_ - tp_;
        return (int)std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
    }

    // Setter avec des champs spécifiques
    void setDateTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0) {
        if ((year < 1601) || (month < 1) || (month > 12) || (day < 1) || (day > 31)) {
            throw std::invalid_argument("Invalid date values!");
        }

        std::tm timeInfo = {};
        timeInfo.tm_year = year - 1900; // tm_year est l'année depuis 1900
        timeInfo.tm_mon = month - 1;   // tm_mon est de 0 à 11
        timeInfo.tm_mday = day;
        timeInfo.tm_hour = hour;
        timeInfo.tm_min = minute;
        timeInfo.tm_sec = second;

        std::time_t newTime = std::mktime(&timeInfo);
        if (newTime == -1) {
            throw std::runtime_error("Failed to convert to std::time_t");
        }

        // On crée un time_point correspondant
        tp_ = std::chrono::system_clock::from_time_t(newTime);
    }

    // Conversion vers std::time_t
    std::time_t toTimeT() const {
        return std::chrono::system_clock::to_time_t(tp_);
    }

    // Accès aux composants
    int year() const { return localTime().tm_year + 1900; }
    int month() const { return localTime().tm_mon + 1; }
    int day() const { return localTime().tm_mday; }
    int hour() const { return localTime().tm_hour; }
    int minute() const { return localTime().tm_min; }
    int second() const { return localTime().tm_sec; }

    // Représentation sous forme de chaîne (ISO 8601)
    std::string toString() const {
        const std::tm& timeInfo = localTime();
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(4) << (timeInfo.tm_year + 1900) << '-'
            << std::setw(2) << (timeInfo.tm_mon + 1) << '-'
            << std::setw(2) << timeInfo.tm_mday << ' '
            << std::setw(2) << timeInfo.tm_hour << ':'
            << std::setw(2) << timeInfo.tm_min << ':'
            << std::setw(2) << timeInfo.tm_sec;
        return oss.str();
    }

    // Opérateurs de comparaison
    bool operator==(const SwDateTime& other) const {
        return tp_ == other.tp_;
    }

    bool operator!=(const SwDateTime& other) const {
        return !(*this == other);
    }

    bool operator<(const SwDateTime& other) const {
        return tp_ < other.tp_;
    }

    bool operator<=(const SwDateTime& other) const {
        return tp_ <= other.tp_;
    }

    bool operator>(const SwDateTime& other) const {
        return tp_ > other.tp_;
    }

    bool operator>=(const SwDateTime& other) const {
        return tp_ >= other.tp_;
    }

    // Ajouter ou retirer des jours
    SwDateTime addDays(int days) const {
        // 24h * 60 *60 = 86400 sec, on utilise chrono::hours
        auto newTp = tp_ + std::chrono::hours(days * 24);
        return SwDateTime(std::chrono::system_clock::to_time_t(newTp));
    }

    SwDateTime subtractDays(int days) const {
        return addDays(-days);
    }

    // Ajouter ou retirer des secondes
    SwDateTime addSeconds(int seconds) const {
        auto newTp = tp_ + std::chrono::seconds(seconds);
        return SwDateTime(std::chrono::system_clock::to_time_t(newTp));
    }

    SwDateTime subtractSeconds(int seconds) const {
        return addSeconds(-seconds);
    }

    // Ajouter ou retirer des minutes
    SwDateTime addMinutes(int minutes) const {
        auto newTp = tp_ + std::chrono::minutes(minutes);
        return SwDateTime(std::chrono::system_clock::to_time_t(newTp));
    }

    SwDateTime subtractMinutes(int minutes) const {
        return addMinutes(-minutes);
    }

    // Ajouter ou retirer des mois
    SwDateTime addMonths(int months) const {
        const std::tm& local = localTime();
        int newYear = local.tm_year + 1900 + (local.tm_mon + months) / 12;
        int newMonth = (local.tm_mon + months) % 12;
        if (newMonth < 0) {
            newMonth += 12;
            newYear -= 1;
        }

        int dayToUse = local.tm_mday;
        int dim = daysInMonth(newYear, newMonth + 1);
        if (dayToUse > dim) {
            dayToUse = dim;
        }

        // Recréer un SwDateTime avec ces données
        SwDateTime temp(newYear, newMonth + 1, dayToUse, local.tm_hour, local.tm_min, local.tm_sec);
        return temp;
    }

    SwDateTime subtractMonths(int months) const {
        return addMonths(-months);
    }

    // Ajouter ou retirer des années
    SwDateTime addYears(int years) const {
        return addMonths(years * 12);
    }

    SwDateTime subtractYears(int years) const {
        return addYears(-years);
    }

    static int daysInMonth(int year, int month) {
        static const int daysInMonths[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

        if ((month < 1) || (month > 12)) {
            throw std::invalid_argument("Invalid month value");
        }

        if (month == 2 && isLeapYear(year)) {
            return 29;
        }

        return daysInMonths[month - 1];
    }

    // Vérifie si une année est bissextile
    static bool isLeapYear(int year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

private:
    std::chrono::system_clock::time_point tp_;

    const std::tm& localTime() const {
        static thread_local std::tm timeInfo;
        std::time_t t = std::chrono::system_clock::to_time_t(tp_);
        if (localtime_s(&timeInfo, &t) != 0) {
            throw std::runtime_error("Failed to convert to local time");
        }
        return timeInfo;
    }
};
