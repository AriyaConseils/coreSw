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

#include <windows.h>
#include "SwObject.h"
#include "SwIODescriptor.h"
#include "SwTimer.h"
#include "SwString.h"
#include "SwEventLoop.h"
#include <vector>
#include <atomic>
#include <SwDateTime.h>
#include <SwEventLoop.h>

// Contexte pour les opérations asynchrones
struct IOContext {
    OVERLAPPED ovl;
    std::vector<char> buffer;
    bool isRead;
    IOContext(size_t bufSize, bool readOp)
        : buffer(bufSize), isRead(readOp)
    {
        ZeroMemory(&ovl, sizeof(ovl));
    }
};

class SwIODevice : public SwObject {
    SW_OBJECT(SwIODevice, SwObject)
public:
    SwIODevice(SwObject* parent = nullptr)
        : SwObject(parent),
        m_timerDercriptor(new SwTimer(100, this)),
        iocpHandle_(NULL),
        fileHandle_(INVALID_HANDLE_VALUE),
        monitoring(false)
    {
        connect(m_timerDercriptor, SIGNAL(timeout), this, &SwIODevice::onTimerDescriptor);
    }

    virtual ~SwIODevice() {
        close();
        if (m_timerDercriptor) {
            m_timerDercriptor->stop();
            delete m_timerDercriptor;
            m_timerDercriptor = nullptr;
        }
    }

    virtual bool open(const SwString& filePath,
                      DWORD desiredAccess = GENERIC_READ | GENERIC_WRITE,
                      DWORD shareMode = 0,
                      DWORD creationDisposition = OPEN_EXISTING,
                      DWORD flagsAndAttributes = FILE_FLAG_OVERLAPPED)
    {
        filePath_ = filePath;
        fileHandle_ = ::CreateFileW(filePath_.toStdWString().c_str(),
                                    desiredAccess,
                                    shareMode,
                                    NULL,
                                    creationDisposition,
                                    flagsAndAttributes,
                                    NULL);
        if (fileHandle_ == INVALID_HANDLE_VALUE) {
            return false;
        }

        // Créer ou associer le IOCP
        iocpHandle_ = CreateIoCompletionPort(fileHandle_, NULL, (ULONG_PTR)this, 0);
        if (!iocpHandle_) {
            ::CloseHandle(fileHandle_);
            fileHandle_ = INVALID_HANDLE_VALUE;
            return false;
        }

        return true;
    }

    virtual void close() {
        if (fileHandle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(fileHandle_);
            fileHandle_ = INVALID_HANDLE_VALUE;
        }
        if (iocpHandle_) {
            ::CloseHandle(iocpHandle_);
            iocpHandle_ = NULL;
        }
    }

    virtual bool isOpen() const {
        return (fileHandle_ != INVALID_HANDLE_VALUE);
    }

    static bool exists(const SwString& path) {
        if (path.isEmpty()) {
            return false;
        }
        DWORD attributes = GetFileAttributesW(path.toStdWString().c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    // Lire de façon asynchrone
    // offset : position dans le fichier (0 par défaut)
    virtual bool read(size_t size, LONGLONG offset = 0) {
        if (!isOpen()) return false;
        IOContext* context = new IOContext(size, true);
        context->ovl.Offset = (DWORD)(offset & 0xFFFFFFFF);
        context->ovl.OffsetHigh = (DWORD)((offset >> 32) & 0xFFFFFFFF);

        BOOL result = ::ReadFile(fileHandle_, context->buffer.data(), (DWORD)size, NULL, &context->ovl);
        if (!result) {
            DWORD err = GetLastError();
            if (err != ERROR_IO_PENDING) {
                // Erreur
                delete context;
                return false;
            }
        }
        // Opération en cours, le polling dans onTimerDescriptor récupérera le résultat
        return true;
    }

    // Ecriture asynchrone
    virtual bool write(const SwString& data, LONGLONG offset = 0) {
        if (!isOpen()) return false;

        // Créer un contexte d'I/O
        IOContext* context = new IOContext(data.size(), false);

        // Copier les données de SwString dans le buffer
        memcpy(context->buffer.data(), data.toStdString().data(), data.size());

        // Configurer l'offset pour l'écriture
        context->ovl.Offset = (DWORD)(offset & 0xFFFFFFFF);
        context->ovl.OffsetHigh = (DWORD)((offset >> 32) & 0xFFFFFFFF);

        // Appeler l'API WriteFile
        BOOL result = ::WriteFile(fileHandle_, context->buffer.data(), (DWORD)data.size(), NULL, &context->ovl);
        if (!result) {
            DWORD err = GetLastError();
            if (err != ERROR_IO_PENDING) {
                // En cas d'erreur
                delete context;
                return false;
            }
        }
        m_timerDercriptor->start();
        swhile(!_writeFinished);
        m_timerDercriptor->stop();
        return true;
    }

    // Fonctions de surveillance du fichier
    void startMonitoring() {
        monitoring = true;
        updateLastWriteTime();
        m_timerDercriptor->start();
    }

    void stopMonitoring() {
        monitoring = false;
        m_timerDercriptor->stop();
    }

protected slots:
    virtual void onTimerDescriptor() {
        // Vérifier les modifications de fichier
        if (monitoring) {
            checkFileChanges();
        }

        // Polling IOCP : récupération des événements complétés
        pollIOCP();

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
    SwTimer* m_timerDercriptor;
    SwString filePath_;
    FILETIME lastWriteTime_;

    HANDLE iocpHandle_;
    HANDLE fileHandle_;
    bool monitoring;
    SwList<SwIODescriptor*> descriptors_;
    bool _readyRead = false;
    bool _writeFinished = false;

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

    void checkFileChanges() {
        if (filePath_.isEmpty() || !exists(filePath_)) {
            return;
        }

        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (GetFileAttributesExW(filePath_.toStdWString().c_str(), GetFileExInfoStandard, &fileInfo)) {
            if (CompareFileTime(&fileInfo.ftLastWriteTime, &lastWriteTime_) != 0) {
                lastWriteTime_ = fileInfo.ftLastWriteTime;
                emitSignal("fileChanged", filePath_);
            }
        }
    }

    void updateLastWriteTime() {
        if (exists(filePath_)) {
            WIN32_FILE_ATTRIBUTE_DATA fileInfo;
            if (GetFileAttributesExW(filePath_.toStdWString().c_str(), GetFileExInfoStandard, &fileInfo)) {
                lastWriteTime_ = fileInfo.ftLastWriteTime;
            }
        }
    }

    // Récupère les IO terminées par l'IOCP (polling)
    void pollIOCP() {
        if (!iocpHandle_) return;

        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        LPOVERLAPPED pOverlapped = nullptr;

        // On récupère autant d'événements que possible sans bloquer
        // en mettant un timeout à 0
        while (true) {
            BOOL res = GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &completionKey, &pOverlapped, 0);
            if (!res && !pOverlapped) {
                // Pas d'événement disponible
                DWORD err = GetLastError();
                if (err == WAIT_TIMEOUT) {
                    // Aucun événement prêt
                    break;
                } else {
                    // Autre erreur
                    break;
                }
            }

            if (pOverlapped == nullptr) {
                // Aucun Overlapped, on arrête
                break;
            }

            IOContext* context = CONTAINING_RECORD(pOverlapped, IOContext, ovl);
            if (!res) {
                // Opération échouée
                DWORD err = GetLastError();
                // Gérer l'erreur si nécessaire
                delete context;
                continue;
            }

            // Opération réussie
            if (context->isRead) {
                _readyRead = true;
            } else {
                _writeFinished = true;
                onWriteCompleted(bytesTransferred);
            }
        }
    }

    void waitForReadyRead(int timeout = 3000) {
        SwDateTime startTime = SwDateTime::currentDateTime();
        bool timedOut = false;

        swhile (!_readyRead && !timedOut) {
            SwDateTime currentTime = SwDateTime::currentDateTime();
            int elapsed = startTime.msecsTo(currentTime);
            if (elapsed >= timeout) {
                timedOut = true;
            }
        }
    }

    virtual void onWriteCompleted(size_t bytesWritten) {
        // Par défaut rien, à surcharger
        // ex: emitSignal("writeCompleted", SwString::number((int64_t)bytesWritten));
    }
};
