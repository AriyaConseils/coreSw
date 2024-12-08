#pragma once

#include "CoreApplication.h"
#include "IODevice.h"
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <iostream>
#include <stdexcept>
#include <ctime>




class SwFile : public IODevice {
public:
    enum OpenMode {
        Read,
        Write,
        Append
    };

    SwFile(Object* parent = nullptr)
        : IODevice(parent), currentMode_(Read) {
        ZeroMemory(&lastWriteTime_, sizeof(FILETIME));
    }

    explicit SwFile(const std::string& filePath, Object* parent = nullptr)
        : IODevice(parent), currentMode_(Read) {
        filePath_ = filePath;
        ZeroMemory(&lastWriteTime_, sizeof(FILETIME));
    }

    virtual ~SwFile() {
        close();
    }


    // Définir le chemin du fichier
    void setFilePath(const std::string& filePath) {
        filePath_ = filePath;
    }


    // Ouvrir un fichier
    bool open(OpenMode mode) {
        if (filePath_.empty()) {
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
    bool write(const std::string& data) {
        if (currentMode_ != Write && currentMode_ != Append) {
            throw std::runtime_error("Fichier non ouvert en mode écriture.");
        }
        fileStream_ << data;
        fileStream_.flush();
        return fileStream_.good();
    }

    std::string readAll() {
        if (currentMode_ != Read) {
            throw std::runtime_error("Fichier non ouvert en mode lecture.");
        }

        std::string content;
        fileStream_.seekg(0, std::ios::end);
        content.resize(fileStream_.tellg());
        fileStream_.seekg(0, std::ios::beg);
        fileStream_.read(&content[0], content.size());

        // Trouver la dernière position non-nulle
        size_t lastNonNull = content.find_last_not_of('\0');
        if (lastNonNull != std::string::npos) {
            content.resize(lastNonNull + 1);
        } else {
            content.clear(); // Si tout est nul, vider la chaîne
        }


        return content;
    }


    // Vérifier si le fichier est ouvert
    bool isOpen() const override {
        return fileStream_.is_open();
    }

    std::string getDirectory() const {
        if (filePath_.empty()) {
            throw std::runtime_error("Chemin du fichier non défini.");
        }

        size_t lastSlash = filePath_.find_last_of("/\\");
        if (lastSlash == std::string::npos) {
            return ""; // Aucun répertoire trouvé (le chemin peut être un fichier sans répertoire)
        }

        return filePath_.substr(0, lastSlash); // Retourne le répertoire sans le dernier séparateur
    }

    static bool isFile(const std::string& path) {
        struct stat info;

        // Utilisation de stat pour obtenir des informations sur le chemin
        if (stat(path.c_str(), &info) != 0) {
            // Retourne false si le chemin est inaccessible ou n'existe pas
            return false;
        }

        // Vérifie si c'est un fichier
        return (info.st_mode & S_IFREG) != 0;
    }


    bool contains(const std::string& keyword) {
        if (currentMode_ != Read) {
            throw std::runtime_error("Fichier non ouvert en mode lecture.");
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

    std::string readLine(std::size_t lineNumber) {
        if (currentMode_ != Read) {
            throw std::runtime_error("Fichier non ouvert en mode lecture.");
        }

        std::string line;
        fileStream_.seekg(0); // Revenir au début du fichier
        for (std::size_t currentLine = 0; currentLine <= lineNumber; ++currentLine) {
            if (!std::getline(fileStream_, line)) {
                throw std::out_of_range("Ligne hors limites.");
            }
        }
        return line;
    }

    std::string readChunk(std::size_t chunkSize) {
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


    std::string readLinesInRangeLazy(std::size_t startLine, std::size_t endLine) {
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

        std::string result;
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


    static bool copy(const std::string& source, const std::string& destination, bool nonBlocking = true) {
        // Vérifier si le fichier source existe
        std::ifstream srcStream(source, std::ios::binary);
        if (!srcStream.is_open()) {
            std::cerr << "Failed to open source file: " << source << std::endl;
            return false;
        }

        // Vérifier si la destination est un répertoire
        std::string finalDestination = destination;
        struct stat info;
        if (stat(destination.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) {
            size_t lastSlash = source.find_last_of("/\\");
            std::string fileName = (lastSlash == std::string::npos) ? source : source.substr(lastSlash + 1);

            if (destination.back() != '/' && destination.back() != '\\') {
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

        // Copier par blocs
        constexpr size_t bufferSize = 1024 * 1024; // 1 Mo
        char buffer[bufferSize];
        while (true) {
            srcStream.read(buffer, bufferSize); // Lire un bloc
            std::streamsize bytesRead = srcStream.gcount(); // Obtenir le nombre d'octets lus
            if (bytesRead == 0) {
                break; // Rien à lire, fin de la boucle
            }
            if(nonBlocking) CoreApplication::instance()->processEvent();
            destStream.write(buffer, bytesRead);
        }

        if (!destStream.good()) {
            std::cerr << "Error occurred during file copy from " << source << " to " << finalDestination << std::endl;
            return false;
        }
        return true;
    }

    // Lire les métadonnées d'un fichier
    bool getFileMetadata(std::time_t& creationTime, std::time_t& lastAccessTime, std::time_t& lastWriteTime) {
        HANDLE fileHandle = CreateFileA(
            filePath_.c_str(),
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
    bool setCreationTime(std::time_t creationTime) {
        HANDLE fileHandle = CreateFileA(
            filePath_.c_str(),
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

        FILETIME ftCreation = SystemTimeToFileTime(creationTime);
        if (!SetFileTime(fileHandle, &ftCreation, nullptr, nullptr)) {
            std::cerr << "Erreur lors de la mise à jour de la date de création : " << GetLastError() << std::endl;
            CloseHandle(fileHandle);
            return false;
        }

        CloseHandle(fileHandle);
        return true;
    }

    // Modifier la date de dernière modification
    bool setLastWriteDate(std::time_t lastWriteTime) {
        HANDLE fileHandle = CreateFileA(
            filePath_.c_str(),
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

        FILETIME ftLastWrite = SystemTimeToFileTime(lastWriteTime);
        if (!SetFileTime(fileHandle, nullptr, nullptr, &ftLastWrite)) {
            std::cerr << "Erreur lors de la mise à jour de la date de modification : " << GetLastError() << std::endl;
            CloseHandle(fileHandle);
            return false;
        }

        CloseHandle(fileHandle);
        return true;
    }

    // Modifier la date de dernier accès
    bool setLastAccessDate(std::time_t lastAccessTime) {
        HANDLE fileHandle = CreateFileA(
            filePath_.c_str(),
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

        FILETIME ftLastAccess = SystemTimeToFileTime(lastAccessTime);
        if (!SetFileTime(fileHandle, nullptr, &ftLastAccess, nullptr)) {
            std::cerr << "Erreur lors de la mise à jour de la date de dernier accès : " << GetLastError() << std::endl;
            CloseHandle(fileHandle);
            return false;
        }

        CloseHandle(fileHandle);
        return true;
    }

    // Modifier toutes les dates (création, dernier accès, dernière modification)
    bool setAllDates(std::time_t creationTime, std::time_t lastAccessTime, std::time_t lastWriteTime) {
        HANDLE fileHandle = CreateFileA(
            filePath_.c_str(),
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

        FILETIME ftCreation = SystemTimeToFileTime(creationTime);
        FILETIME ftLastAccess = SystemTimeToFileTime(lastAccessTime);
        FILETIME ftLastWrite = SystemTimeToFileTime(lastWriteTime);

        if (!SetFileTime(fileHandle, &ftCreation, &ftLastAccess, &ftLastWrite)) {
            std::cerr << "Erreur lors de la mise à jour des dates : " << GetLastError() << std::endl;
            CloseHandle(fileHandle);
            return false;
        }

        CloseHandle(fileHandle);
        return true;
    }

    std::string fileChecksum()
    {
        std::string checksum;
        if(filePath_ != ""){
            try {
                checksum = SwCrypto::calculateFileChecksum(filePath_);
            } catch (const std::exception& ex) {
                std::cerr << "File checksum erreur on: " << filePath_ << "\n" << ex.what() << std::endl;
            }
        }
        return checksum;
    }
signals:
    DECLARE_SIGNAL(fileChanged, const std::string&);

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
private:
    std::fstream fileStream_;
    OpenMode currentMode_;


};
