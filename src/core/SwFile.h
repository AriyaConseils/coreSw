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


#include "SwCoreApplication.h"
#include "SwIODevice.h"
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <iostream>
#include <stdexcept>
#include <ctime>
#include "SwDateTime.h"
#include "SwStandardLocation.h"


class SwFile : public SwIODevice {
public:
    enum OpenMode {
        Read,
        Write,
        Append
    };

    SwFile(Object* parent = nullptr)
        : SwIODevice(parent), currentMode_(Read) {
        ZeroMemory(&lastWriteTime_, sizeof(FILETIME));
    }

    explicit SwFile(const SwString& filePath, Object* parent = nullptr)
        : SwIODevice(parent), currentMode_(Read) {
        filePath_ = filePath;
        ZeroMemory(&lastWriteTime_, sizeof(FILETIME));
    }

    virtual ~SwFile() {
        close();
    }


    // Définir le chemin du fichier
    void setFilePath(const SwString& filePath) {
        filePath_ = filePath;
    }

    SwString fileName() const {
        if (filePath_.isEmpty()) {
            std::cerr << "Error: File path is empty!" << std::endl;
        }

        // Retourner la partie après le dernier séparateur
        return filePath_.split("/").last();
    }

    // Ouvrir un fichier
    bool open(OpenMode mode) {
        if (filePath_.isEmpty()) {
            std::cerr << "Chemin du fichier non défini." << std::endl;
        }

        std::ios::openmode openMode;
        switch (mode) {
        case Read:
            openMode = std::ios::in;
            break;
        case Write:
            openMode = std::ios::out | std::ios::trunc;
            break;
        case Append:
            openMode = std::ios::out | std::ios::app;
            break;
        default:
            std::cerr << "Mode d'ouverture invalide." << std::endl;
        }

        fileStream_.open(filePath_, openMode);
        if (fileStream_.is_open()) {
            currentMode_ = mode;
        }
        return fileStream_.is_open();
    }

    // Fermer le fichier
    void close() override {
        if (fileStream_.is_open()) {
            fileStream_.close();
        }
        stopMonitoring();
    }

    // Écrire dans le fichier
    bool write(const SwString& data) {
        if (currentMode_ != Write && currentMode_ != Append) {
            std::cerr << "Fichier non ouvert en mode écriture." << std::endl;
        }
        fileStream_ << data;
        fileStream_.flush();
        return fileStream_.good();
    }

    SwString readAll() {
        if (currentMode_ != Read) {
            std::cerr << "Fichier non ouvert en mode lecture." << std::endl;
        }

        // Aller à la fin du fichier pour déterminer la taille
        fileStream_.seekg(0, std::ios::end);
        std::streamsize size = fileStream_.tellg();
        if (size <= 0) {
            return SwString(); // Retourne une chaîne vide si le fichier est vide ou invalide
        }

        fileStream_.seekg(0, std::ios::beg);

        // Redimensionner le buffer interne de SwString
        SwString content;
        content.resize(size); // Ajoutez cette méthode dans SwString

        // Lire les données directement dans le tampon
        fileStream_.read(content.data(), size);

        return content;
    }

    // Vérifier si le fichier est ouvert
    bool isOpen() const override {
        return fileStream_.is_open();
    }

    SwString getDirectory() const {
        if (filePath_.isEmpty()) {
            std::cerr << "Chemin du fichier non défini." << std::endl;
            return SwString(); // Retourne une chaîne vide en cas d'erreur
        }

        SwStringList parts = filePath_.split("/");
        parts.removeLast(); // Supprime la dernière partie (le nom du fichier)

        return parts.join("/"); // Rejoint les parties pour reformer le chemin
    }


    static bool isFile(const SwString& path) {
        struct stat info;

        // Utilisation de stat pour obtenir des informations sur le chemin
        if (stat(path.toStdString().c_str(), &info) != 0) {
            // Retourne false si le chemin est inaccessible ou n'existe pas
            return false;
        }

        // Vérifie si c'est un fichier
        return (info.st_mode & S_IFREG) != 0;
    }


    bool contains(const SwString& keyword) {
        if (currentMode_ != Read) {
            std::cerr << "Fichier non ouvert en mode lecture." << std::endl;
        }

        std::string line;
        fileStream_.seekg(0); // Revenir au début du fichier
        while (std::getline(fileStream_, line)) {
            if (line.find(keyword) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    SwString readLine(std::size_t lineNumber) {
        if (currentMode_ != Read) {
            std::cerr << "Fichier non ouvert en mode lecture." << std::endl;
        }

        std::string line;
        fileStream_.seekg(0); // Revenir au début du fichier
        for (std::size_t currentLine = 0; currentLine <= lineNumber; ++currentLine) {
            if (!std::getline(fileStream_, line)) {
                std::cerr << "Ligne hors limites." << std::endl;
            }
        }
        return line;
    }

    SwString readLine() {
        if (currentMode_ != Read) {
            std::cerr << "Fichier non ouvert en mode lecture." << std::endl;
        }

        std::string line;
        if (!std::getline(fileStream_, line)) {
            return "";
        }
        return line;
    }

    bool atEnd() const {
        if (!fileStream_.is_open()) {
            std::cerr << "Fichier non ouvert." << std::endl;
        }
        return fileStream_.eof();
    }

    SwString readChunk(std::size_t chunkSize) {
        if (currentMode_ != Read) {
            throw std::runtime_error("Fichier non ouvert en mode lecture.");
        }

        std::string buffer(chunkSize, '\0');
        fileStream_.read(&buffer[0], chunkSize);
        buffer.resize(fileStream_.gcount()); // Ajuster à la taille réelle lue

        return buffer;
    }

    void seek(std::streampos position) {
        if (!isOpen()) {
            throw std::runtime_error("Fichier non ouvert.");
        }
        fileStream_.seekg(position);
    }

    std::streampos currentPosition() {
        if (!isOpen()) {
            throw std::runtime_error("Fichier non ouvert.");
        }
        return fileStream_.tellg();
    }


    SwString readLinesInRangeLazy(std::size_t startLine, std::size_t endLine) {
        if (currentMode_ != Read) {
            throw std::runtime_error("Fichier non ouvert en mode lecture.");
        }

        if (startLine > endLine) {
            throw std::invalid_argument("La ligne de début doit être inférieure ou égale à la ligne de fin.");
        }

        if (!fileStream_.is_open()) {
            throw std::runtime_error("Le flux du fichier n'est pas ouvert.");
        }

        // Réinitialiser le flux pour éviter les problèmes d'état
        fileStream_.clear();
        fileStream_.seekg(0, std::ios::beg); // Revenir au début du fichier

        SwString result;
        std::string line;
        std::size_t currentLine = 0;

        while (std::getline(fileStream_, line)) {
            if (currentLine >= startLine && currentLine <= endLine) {
                result += line + '\n'; // Ajouter la ligne avec un saut de ligne
            }
            if (currentLine > endLine) {
                break; // On arrête dès que la plage est dépassée
            }
            ++currentLine;
        }

        return result;
    }

    static bool copy(const SwString& source, const SwString& destination, bool overwrite = true) {
        // Convertir les chemins en wide strings pour l'API Windows
        std::wstring sourceWide(source.begin(), source.end());
        std::wstring destinationWide(destination.begin(), destination.end());

        // Utiliser la fonction CopyFile pour effectuer la copie
        if (!CopyFileW(sourceWide.c_str(), destinationWide.c_str(), !overwrite)) {
            DWORD error = GetLastError();

            // Afficher un message d'erreur en cas de problème
            if (error == ERROR_FILE_NOT_FOUND) {
                std::cerr << "Source file not found: " << source << std::endl;
            } else if (error == ERROR_ACCESS_DENIED) {
                std::cerr << "Access denied for destination file: " << destination << std::endl;
            } else {
                std::cerr << "Failed to copy file. Error code: " << error << std::endl;
            }

            return false;
        }
        return true;
    }

    static bool copyByChunk(const SwString& source, const SwString& destination,
                            bool nonBlocking = true, int chunkSize = 1024) {
        // Vérifier si le fichier source existe
        std::ifstream srcStream(source, std::ios::binary);
        if (!srcStream.is_open()) {
            std::cerr << "Failed to open source file: " << source << std::endl;
            return false;
        }

        // Vérifier si la destination est un répertoire
        std::string finalDestination = destination;
        struct stat info;
        if (stat(finalDestination.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) {
            size_t lastSlash = source.toStdString().find_last_of("/\\");
            std::string fileName = (lastSlash == std::string::npos) ? source : source.toStdString().substr(lastSlash + 1);

            if (finalDestination.back() != '/' && finalDestination.back() != '\\') {
                finalDestination += "\\";
            }
            finalDestination += fileName;
        }

        // Ouvrir le fichier de destination
        std::ofstream destStream(finalDestination, std::ios::binary | std::ios::trunc);
        if (!destStream.is_open()) {
            std::cerr << "Failed to open destination file: " << finalDestination << std::endl;
            return false;
        }

        // Utiliser le tas pour le buffer
        size_t bufferSize = chunkSize * 1024;
        auto buffer = std::make_unique<char[]>(bufferSize); // Allocation sur le tas

        // Copier par blocs
        while (true) {
            srcStream.read(buffer.get(), bufferSize); // Lire un bloc
            std::streamsize bytesRead = srcStream.gcount(); // Obtenir le nombre d'octets lus
            if (bytesRead == 0) {
                break; // Rien à lire, fin de la boucle
            }
            if (nonBlocking) { SwCoreApplication::instance()->processEvent(); }

            destStream.write(buffer.get(), bytesRead); // Écriture des données
        }

        if (!destStream.good()) {
            std::cerr << "Error occurred during file copy from " << source << " to " << finalDestination << std::endl;
            return false;
        }
        return true;
    }


    bool copyByChunk(const SwString& destination, bool nonBlocking = true, int chunkSize = 1024) {
        if (filePath_.isEmpty()) {
            std::cerr << "Source file path is not set." << std::endl;
            return false;
        }

        // Vérifier si le fichier source existe
        std::ifstream srcStream(filePath_, std::ios::binary);
        if (!srcStream.is_open()) {
            std::cerr << "Failed to open source file: " << filePath_ << std::endl;
            return false;
        }

        // Vérifier si la destination est un répertoire
        std::string finalDestination = destination;
        struct stat info;
        if (stat(destination.toStdString().c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) {
            size_t lastSlash = filePath_.toStdString().find_last_of("/\\");
            SwString fileName = (lastSlash == std::string::npos) ? filePath_ : filePath_.toStdString().substr(lastSlash + 1);

            if (destination.toStdString().back() != '/' && destination.toStdString().back() != '\\') {
                finalDestination += "/";
            }
            finalDestination += fileName;
        }

        // Ouvrir le fichier de destination
        std::ofstream destStream(finalDestination, std::ios::binary | std::ios::trunc);
        if (!destStream.is_open()) {
            std::cerr << "Failed to open destination file: " << finalDestination << std::endl;
            return false;
        }

        // Utiliser un buffer alloué sur le tas
        size_t bufferSize = chunkSize * 1024;
        auto buffer = std::make_unique<char[]>(bufferSize);

        // Copier par blocs
        while (true) {
            srcStream.read(buffer.get(), bufferSize); // Lire un bloc
            std::streamsize bytesRead = srcStream.gcount(); // Obtenir le nombre d'octets lus
            if (bytesRead == 0) {
                break; // Rien à lire, fin de la boucle
            }

            // Écriture dans le fichier destination
            destStream.write(buffer.get(), bytesRead);
            if (!destStream.good()) {
                std::cerr << "Write error occurred during file copy to: " << finalDestination << std::endl;
                return false;
            }

            if (nonBlocking) { SwCoreApplication::instance()->processEvent(); }
        }

        // Vérifier si tout est bien écrit
        if (!destStream.good()) {
            std::cerr << "Error occurred during file copy from " << filePath_ << " to " << finalDestination << std::endl;
            return false;
        }

        std::cout << "File successfully copied from " << filePath_ << " to " << finalDestination << std::endl;
        return true;
    }



    // Lire les métadonnées d'un fichier
    bool getFileMetadata(SwDateTime& creationTime, SwDateTime& lastAccessTime, SwDateTime& lastWriteTime) {
        HANDLE fileHandle = CreateFileW(
            filePath_.toStdWString().c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
            );

        if (fileHandle == INVALID_HANDLE_VALUE) {
            std::cerr << "Erreur lors de l'ouverture du fichier : " << GetLastError() << std::endl;
            return false;
        }

        FILETIME ftCreation, ftLastAccess, ftLastWrite;
        if (GetFileTime(fileHandle, &ftCreation, &ftLastAccess, &ftLastWrite)) {
            creationTime = FileTimeToSystemTime(ftCreation);
            lastAccessTime = FileTimeToSystemTime(ftLastAccess);
            lastWriteTime = FileTimeToSystemTime(ftLastWrite);
        } else {
            std::cerr << "Erreur lors de la lecture des métadonnées : " << GetLastError() << std::endl;
            CloseHandle(fileHandle);
            return false;
        }

        CloseHandle(fileHandle);
        return true;
    }

    // Modifier la date de création
    bool setCreationTime(SwDateTime creationTime) {
        HANDLE fileHandle = CreateFileW(
            filePath_.toStdWString().c_str(),
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
            );

        if (fileHandle == INVALID_HANDLE_VALUE) {
            std::cerr << "Erreur lors de l'ouverture du fichier : " << GetLastError() << std::endl;
            return false;
        }

        FILETIME ftCreation = SystemTimeToFileTime(creationTime.toTimeT());
        if (!SetFileTime(fileHandle, &ftCreation, nullptr, nullptr)) {
            std::cerr << "Erreur lors de la mise à jour de la date de création : " << GetLastError() << std::endl;
            CloseHandle(fileHandle);
            return false;
        }

        CloseHandle(fileHandle);
        return true;
    }

    // Modifier la date de dernière modification
    bool setLastWriteDate(SwDateTime lastWriteTime) {
        HANDLE fileHandle = CreateFileW(
            filePath_.toStdWString().c_str(),
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
            );

        if (fileHandle == INVALID_HANDLE_VALUE) {
            std::cerr << "Erreur lors de l'ouverture du fichier : " << GetLastError() << std::endl;
            return false;
        }

        FILETIME ftLastWrite = SystemTimeToFileTime(lastWriteTime.toTimeT());
        if (!SetFileTime(fileHandle, nullptr, nullptr, &ftLastWrite)) {
            std::cerr << "Erreur lors de la mise à jour de la date de modification : " << GetLastError() << std::endl;
            CloseHandle(fileHandle);
            return false;
        }

        CloseHandle(fileHandle);
        return true;
    }

    // Modifier la date de dernier accès
    bool setLastAccessDate(SwDateTime lastAccessTime) {
        HANDLE fileHandle = CreateFileW(
            filePath_.toStdWString().c_str(),
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
            );

        if (fileHandle == INVALID_HANDLE_VALUE) {
            std::cerr << "Erreur lors de l'ouverture du fichier : " << GetLastError() << std::endl;
            return false;
        }

        FILETIME ftLastAccess = SystemTimeToFileTime(lastAccessTime.toTimeT());
        if (!SetFileTime(fileHandle, nullptr, &ftLastAccess, nullptr)) {
            std::cerr << "Erreur lors de la mise à jour de la date de dernier accès : " << GetLastError() << std::endl;
            CloseHandle(fileHandle);
            return false;
        }

        CloseHandle(fileHandle);
        return true;
    }

    // Modifier toutes les dates (création, dernier accès, dernière modification)
    bool setAllDates(SwDateTime creationTime, SwDateTime lastAccessTime, SwDateTime lastWriteTime) {
        HANDLE fileHandle = CreateFileW(
            filePath_.toStdWString().c_str(),
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
            );

        if (fileHandle == INVALID_HANDLE_VALUE) {
            std::cerr << "Erreur lors de l'ouverture du fichier : " << GetLastError() << std::endl;
            return false;
        }

        FILETIME ftCreation = SystemTimeToFileTime(creationTime.toTimeT());
        FILETIME ftLastAccess = SystemTimeToFileTime(lastAccessTime.toTimeT());
        FILETIME ftLastWrite = SystemTimeToFileTime(lastWriteTime.toTimeT());

        if (!SetFileTime(fileHandle, &ftCreation, &ftLastAccess, &ftLastWrite)) {
            std::cerr << "Erreur lors de la mise à jour des dates : " << GetLastError() << std::endl;
            CloseHandle(fileHandle);
            return false;
        }

        CloseHandle(fileHandle);
        return true;
    }

    SwString fileChecksum()
    {
        SwString checksum;
        if(filePath_ != ""){
            try {
                checksum = SwCrypto::calculateFileChecksum(filePath_);
            } catch (const std::exception& ex) {
                std::cerr << "File checksum erreur on: " << filePath_ << "\n" << ex.what() << std::endl;
            }
        }
        return checksum;
    }

    bool writeMetadata(const SwString& key, const SwString& value) {
        // Vérifier si le fichier principal existe
        if (!isFile(filePath_)) {
            std::cerr << "Le fichier principal n'existe pas : " << filePath_.toStdString() << std::endl;
            return false;
        }

        // Vérifier si le volume est NTFS
        if (!isNTFS(filePath_)) {
            std::cerr << "Le fichier n'est pas sur un volume NTFS, impossible d'utiliser les flux ADS." << std::endl;
            return false;
        }

        // Construire le chemin complet pour le flux ADS en utilisant la clé comme nom du flux
        SwString normalizedPath = SwStandardLocation::convertPath(filePath_, SwStandardLocation::PathType::WindowsLong);
        std::wstring adsPath = normalizedPath.toStdWString() + L":" + key.toStdWString();

        // Ouvrir le flux ADS en mode écriture
        HANDLE hFile = CreateFileW(
            adsPath.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            std::cerr << "Erreur lors de l'ouverture du flux ADS : " << GetLastError() << std::endl;
            return false;
        }

        // Écrire la valeur directement dans le flux ADS
        DWORD bytesWritten;
        BOOL success = WriteFile(hFile, value.toStdWString().c_str(), value.size() * sizeof(wchar_t), &bytesWritten, nullptr);

        CloseHandle(hFile);

        if (!success) {
            std::cerr << "Erreur lors de l'écriture dans le flux ADS : " << GetLastError() << std::endl;
            return false;
        }

        return true;
    }



    SwString readMetadata(const SwString& key) {
        // Construire le chemin vers le flux ADS
        SwString normalizedPath = SwStandardLocation::convertPath(filePath_, SwStandardLocation::PathType::WindowsLong);
        std::wstring adsPath = normalizedPath.toStdWString() + L":" + key.toStdWString();

        // Ouvrir le flux ADS en mode lecture
        HANDLE hFile = CreateFileW(
            adsPath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            std::cerr << "Erreur lors de l'ouverture du flux ADS : " << GetLastError() << std::endl;
            return SwString();
        }

        // Lire tout le contenu du flux ADS
        std::wstring buffer(1024, L'\0'); // Buffer pour lire les données
        DWORD bytesRead = 0;
        BOOL success = ReadFile(hFile, &buffer[0], buffer.size() * sizeof(wchar_t), &bytesRead, nullptr);

        CloseHandle(hFile);

        if (!success) {
            std::cerr << "Erreur lors de la lecture du flux ADS : " << GetLastError() << std::endl;
            return SwString();
        }

        // Réduire la taille du buffer à la taille effectivement lue
        buffer.resize(bytesRead / sizeof(wchar_t));

        // Convertir le contenu lu en SwString et le retourner
        std::string convertedStr(buffer.begin(), buffer.end());
        return SwString(convertedStr);
    }



signals:
    DECLARE_SIGNAL(fileChanged, const SwString&)

protected:
    // Convertir un temps système (std::time_t) en FILETIME
    FILETIME SystemTimeToFileTime(std::time_t t) {
        ULONGLONG ull = static_cast<ULONGLONG>(t) * 10000000ULL + 116444736000000000ULL;
        FILETIME ft;
        ft.dwLowDateTime = static_cast<DWORD>(ull);
        ft.dwHighDateTime = static_cast<DWORD>(ull >> 32);
        return ft;
    }

    // Convertir un FILETIME en temps système (std::time_t)
    std::time_t FileTimeToSystemTime(const FILETIME& ft) {
        ULONGLONG ull = (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
        return static_cast<std::time_t>((ull - 116444736000000000ULL) / 10000000ULL);
    }

    std::wstring getVolumeRoot(const SwString& filePath) {
        wchar_t volumeRoot[MAX_PATH] = {0};
        SwString normalizedPath = SwStandardLocation::convertPath(filePath, SwStandardLocation::PathType::WindowsLong);
        if (GetVolumePathNameW(normalizedPath.toStdWString().c_str(), volumeRoot, MAX_PATH)) {
            return std::wstring(volumeRoot);
        } else {
            std::wcerr << L"Erreur lors de la récupération du chemin racine : " << GetLastError() << std::endl;
            return L"";
        }
    }

    bool isNTFS(const SwString& filePath) {
        SwString normalizedPath = SwStandardLocation::convertPath(filePath, SwStandardLocation::PathType::WindowsLong);
        std::wstring volumeRoot = getVolumeRoot(normalizedPath);
        if (volumeRoot.empty()) {
            std::cerr << "Impossible de récupérer le volume racine pour : " << filePath.toStdString() << std::endl;
            return false;
        }

        wchar_t fileSystemName[MAX_PATH] = {0};
        DWORD serialNumber = 0, maxComponentLength = 0, fileSystemFlags = 0;

        if (!GetVolumeInformationW(
            volumeRoot.c_str(),
            nullptr,
            0,
            &serialNumber,
            &maxComponentLength,
            &fileSystemFlags,
            fileSystemName,
            MAX_PATH)) {
            std::cerr << "Impossible de déterminer le type de système de fichiers. Erreur : " << GetLastError() << std::endl;
            return false;
        }

        return std::wstring(fileSystemName) == L"NTFS";
    }



private:
    std::fstream fileStream_;
    OpenMode currentMode_;


};
