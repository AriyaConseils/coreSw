#pragma once

#include <windows.h>
#include <iostream>
#include <functional>
#include <string>
#include <algorithm>

#ifndef NOMINMAX
    #define NOMINMAX
#endif


class IODescriptor {
public:
    IODescriptor(HANDLE hFile, std::string descriptorName = "") : handle(hFile), m_descriptorName(descriptorName) {
    }

    virtual ~IODescriptor() {
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    }

    bool waitForEvent(bool& readyToRead, bool& readyToWrite, int timeoutMs = -1) {
        DWORD bytesAvailable = 0;
        if (!PeekNamedPipe(handle, NULL, 0, NULL, &bytesAvailable, NULL)) {
            std::cerr << "PeekNamedPipe failed: " << GetLastError() << std::endl;
            return false;
        }

        readyToRead = (bytesAvailable > 0);
        readyToWrite = false;  

        if (readyToRead || timeoutMs == INFINITE) {
            return true;
        }

        DWORD result = WaitForSingleObject(handle, (timeoutMs == -1) ? INFINITE : timeoutMs);

        if (result == WAIT_FAILED) {
            std::cerr << "WaitForSingleObject failed: " << GetLastError() << std::endl;
            return false;
        }

        readyToRead = (result == WAIT_OBJECT_0 && bytesAvailable > 0);
        return true;
    }

    // Lire avec OVERLAPPED pour ne pas bloquer
    // Lire directement depuis le handle avec gestion asynchrone
    virtual std::string read() {
        char buffer[1024];
        DWORD bytesRead = 0;
        OVERLAPPED overlapped = { 0 };
        overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); 

        if (overlapped.hEvent == NULL) {
            std::cerr << "CreateEvent failed: " << GetLastError() << std::endl;
            return "";
        }

        BOOL success = ReadFile(handle, buffer, sizeof(buffer) - 1, &bytesRead, &overlapped);

        if (!success && GetLastError() == ERROR_IO_PENDING) {
            if (WaitForSingleObject(overlapped.hEvent, INFINITE) == WAIT_OBJECT_0) {
                if (GetOverlappedResult(handle, &overlapped, &bytesRead, FALSE)) {
                    buffer[bytesRead] = '\0';
                    CloseHandle(overlapped.hEvent);
                    return std::string(buffer);
                }
            }
        }

        else if (success && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            CloseHandle(overlapped.hEvent);
            return std::string(buffer);
        }
        else {
            std::cerr << "ReadFile failed: " << GetLastError() << std::endl;
        }

        CloseHandle(overlapped.hEvent);
        return "";
    }


    virtual bool write(const std::string& data) {
        DWORD bytesWritten;
        BOOL success = WriteFile(handle, data.c_str(), data.size(), &bytesWritten, nullptr);
        return success && (bytesWritten == data.size());
    }

    HANDLE descriptor() const {
        return handle;
    }

    void setDescriptorName(const std::string& name) {
        m_descriptorName = name;
    }

    std::string descriptorName() const {
        return m_descriptorName;
    }

protected:
    HANDLE handle;
    std::string m_descriptorName;
};
