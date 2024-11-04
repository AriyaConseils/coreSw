#pragma once

#include "Object.h"
#include "IODescriptor.h"
#include "CoreApplication.h"
#include <functional>
#include <string>
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

protected:
    Timer* m_timerDercriptor;
   

    void addDescriptor(IODescriptor* descriptor) {
        if (descriptor) {
            descriptors.push_back(descriptor);
        }
    }

    void removeDescriptor(IODescriptor* descriptor) {
        if (!descriptor) return;
        auto it = std::remove(descriptors.begin(), descriptors.end(), descriptor);
        if (it != descriptors.end()) {
            descriptors.erase(it, descriptors.end());
            delete descriptor; 
        }
    }

    size_t getDescriptorCount() const {
        return descriptors.size();
    }

private slots:
    void onTimerDescriptor() {
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

private:
    std::vector<IODescriptor*> descriptors;
    bool monitoring;
};
