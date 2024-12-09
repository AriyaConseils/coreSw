#pragma once

#include "Object.h"
#include "IODescriptor.h"
#include "CoreApplication.h"
#include <functional>
#include <string>
#include <windows.h>
#include "Timer.h"

class IODevice : public Object {
public:
    IODevice(Object* parent = nullptr) : Object(parent), monitoring(false), m_timerDercriptor(new Timer(100, this)){
    
        connect(m_timerDercriptor, "timeout", this, &IODevice::onTimerDescriptor);
    }

    virtual ~IODevice() {

    }

    virtual bool open(HANDLE hFile) {
        return false;
    }

    virtual void close() {
    }

    virtual std::string read(int64_t maxSize = 0) {

        return "";
    }

    virtual bool write(const std::string& data) {

        return false;
    }

    virtual bool isOpen() const {
        return false;
    }

    bool exists() const {
        // Convertir le chemin en Unicode (UTF-16) pour la compatibilité avec les fonctions Windows
        std::wstring widePath = std::wstring(filePath_.begin(), filePath_.end());

        // Vérifier les attributs du fichier
        DWORD attributes = GetFileAttributesW(widePath.c_str());

        // Retourner false si les attributs sont INVALID_FILE_ATTRIBUTES
        return (attributes != INVALID_FILE_ATTRIBUTES);
    }

    static bool exists(const std::string& path) {
        // Convertir le chemin en Unicode
        std::wstring widePath(path.begin(), path.end());

        // Vérifier les attributs du fichier
        DWORD attributes = GetFileAttributesW(widePath.c_str());

        // Retourner true si le fichier ou le répertoire existe
        return (attributes != INVALID_FILE_ATTRIBUTES);
    }

    // Démarrer la surveillance
    void startMonitoring() {
        monitoring = true;
        updateLastWriteTime();
        m_timerDercriptor->start();
    }

    // Arrêter la surveillance
    void stopMonitoring() {
        monitoring = false;
        m_timerDercriptor->stop();
    }
protected:
    Timer* m_timerDercriptor;
   

    void addDescriptor(IODescriptor* descriptor) {
        if (descriptor) {
            descriptors.push_back(descriptor);
        }
    }

    void removeDescriptor(IODescriptor* &descriptor) {
        if (!descriptor) return;
        auto it = std::remove(descriptors.begin(), descriptors.end(), descriptor);
        if (it != descriptors.end()) {
            descriptors.erase(it, descriptors.end());
            safeDelete(descriptor);
        }
    }

    size_t getDescriptorCount() const {
        return descriptors.size();
    }

private slots:
    void onTimerDescriptor() {
        if (monitoring) {
            checkFileChanges();
        }

        bool readyToRead = false, readyToWrite = false;

        for (auto descriptor : descriptors) {
            if (descriptor->waitForEvent(readyToRead, readyToWrite, 1)) {
                if (readyToRead) {
                    emitSignal("readyRead" + descriptor->descriptorName());
                }
                if (readyToWrite) {
                    emitSignal("readyWrite" + descriptor->descriptorName());
                }
            }
        }
    }

protected:
    std::string filePath_;
    FILETIME lastWriteTime_;

    void checkFileChanges() {
        if (filePath_.empty() || !exists()) {
            return;
        }

        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        std::wstring widePath = std::wstring(filePath_.begin(), filePath_.end());

        if (GetFileAttributesExW(widePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
            // Vérifier si la dernière écriture a changé
            if (CompareFileTime(&fileInfo.ftLastWriteTime, &lastWriteTime_) != 0) {
                lastWriteTime_ = fileInfo.ftLastWriteTime;
                emitSignal("fileChanged", filePath_);
            }
        }
    }

    // Mettre à jour l'horodatage de dernière modification
    void updateLastWriteTime() {
        if (exists()) {
            WIN32_FILE_ATTRIBUTE_DATA fileInfo;
            std::wstring widePath = std::wstring(filePath_.begin(), filePath_.end());

            // Récupérer les attributs et les informations du fichier
            if (GetFileAttributesExW(widePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
                lastWriteTime_ = fileInfo.ftLastWriteTime;
            }
        }
    }

private:
    std::vector<IODescriptor*> descriptors;
    bool monitoring;

};
