#pragma once

#include <windows.h>
#include "Object.h"
#include "SwIODescriptor.h"
#include "SwTimer.h"

class SwIODevice : public Object {
public:
    SwIODevice(Object* parent = nullptr) : Object(parent), monitoring(false), m_timerDercriptor(new SwTimer(100, this)){
    
        connect(m_timerDercriptor, SIGNAL(timeout), this, &SwIODevice::onTimerDescriptor);
    }

    virtual ~SwIODevice() {
        m_timerDercriptor->stop();
        delete m_timerDercriptor;
    }

    virtual bool open(HANDLE hFile) {
        SW_UNUSED(hFile)
        return false;
    }

    virtual void close() {
    }

    virtual SwString read(int64_t maxSize = 0) {
        SW_UNUSED(maxSize)
        return "";
    }

    virtual bool write(const SwString& data) {
        SW_UNUSED(data)
        return false;
    }

    virtual bool isOpen() const {
        return false;
    }

    bool exists() const {
        return exists(filePath_);
    }

    static bool exists(const SwString& path) {
        if (path.isEmpty()) {
            return false;
        }
        // Convertir le chemin en Unicode
        DWORD attributes = GetFileAttributesW(path.toStdWString().c_str());

        // Retourner false si les attributs sont INVALID_FILE_ATTRIBUTES
        return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
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
    SwTimer* m_timerDercriptor;
   

    void addDescriptor(SwIODescriptor* descriptor) {
        if (descriptor && !descriptors_.contains(descriptor)) {
            descriptors_.append(descriptor);
        }
    }

    void removeDescriptor(SwIODescriptor*& descriptor) {
          if (!descriptor) return;
          if (descriptors_.removeOne(descriptor)) {
              safeDelete(descriptor);
          }
      }

    size_t getDescriptorCount() const {
        return descriptors_.size();
    }

protected slots:
    virtual void onTimerDescriptor() {
        if (monitoring) {
            checkFileChanges();
        }

        bool readyToRead = false, readyToWrite = false;

        for (auto descriptor : descriptors_) {
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
    SwString filePath_;
    FILETIME lastWriteTime_;

    void checkFileChanges() {
        if (filePath_.isEmpty() || !exists()) {
            return;
        }

        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (GetFileAttributesExW(filePath_.toStdWString().c_str(), GetFileExInfoStandard, &fileInfo)) {
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
            // Récupérer les attributs et les informations du fichier
            if (GetFileAttributesExW(filePath_.toStdWString().c_str(), GetFileExInfoStandard, &fileInfo)) {
                lastWriteTime_ = fileInfo.ftLastWriteTime;
            }
        }
    }

private:
    SwList<SwIODescriptor*> descriptors_;
    bool monitoring;

};
