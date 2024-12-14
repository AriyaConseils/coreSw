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

#include <iostream>
#include <sstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include "Object.h"
#include "SwTcpSocket.h"
#include <windows.h> // GetCurrentProcessId()

// Include des classes JSON
#include "SwJsonObject.h"
#include "SwJsonDocument.h"
#include "SwJsonValue.h"
#include "SwJsonArray.h"

enum class SwDebugLevel {
    Debug,
    Warning,
    Error
};

struct SwDebugContext {
    const char* file;
    int line;
    const char* function;
    SwDebugLevel level;
};

class SwDebugMessage; // forward declaration

class SwDebug : public Object {
    SW_OBJECT(SwDebug, Object)
public:
    static SwDebug& instance() {
        static SwDebug _instance;
        return _instance;
    }

    static void setAppName(const SwString& name) {
        instance().m_appName = name;
    }

    static void setVersion(const SwString& version) {
        instance().m_version = version;
    }

    static void setPid(uint32_t pid) {
        instance().m_pid = pid;
    }

    bool connectToHostAndIdentify(const SwString& host, uint16_t port) {
        if (!m_socket) {
            m_socket = new SwTcpSocket(&instance());
            connect(m_socket, SIGNAL(connected), this, &SwDebug::onSocketConnected);
            connect(m_socket, SIGNAL(errorOccurred), this, &SwDebug::onSocketError);
        }

        return m_socket->connectToHost(host, port);
    }

    void logMessage(const SwDebugContext& ctx, const SwString& msg) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Création d'un objet JSON pour le message log
        SwJsonObject jsonObj;
        jsonObj.insert("type", SwJsonValue("log"));
        jsonObj.insert("level", SwJsonValue(levelToString(ctx.level)));
        jsonObj.insert("appName", SwJsonValue(m_appName));
        jsonObj.insert("version", SwJsonValue(m_version));
        jsonObj.insert("pid", SwJsonValue((int)m_pid));
        jsonObj.insert("file", SwJsonValue(ctx.file));
        jsonObj.insert("line", SwJsonValue(ctx.line));
        jsonObj.insert("function", SwJsonValue(ctx.function));
        jsonObj.insert("message", SwJsonValue(msg));

        SwJsonDocument doc(jsonObj);
        SwString finalMsg = doc.toJson() + "\n";

        if (m_socket && m_socket->state() == SwAbstractSocket::ConnectedState) {
            m_socket->write(finalMsg);
        }
        // Fallback console
        switch(ctx.level) {
        case SwDebugLevel::Debug:
            std::cerr << "[DEBUG] ";
            break;
        case SwDebugLevel::Warning:
            std::cerr << "[WARNING] ";
            break;
        case SwDebugLevel::Error:
            std::cerr << "[ERROR] ";
            break;
        }
        std::cerr << ctx.file << ":" << ctx.line << " (" << ctx.function << ") " << msg << std::endl;
    }

private:
    SwDebug() {
        m_appName = "UnknownApp";
        m_version = "0.0.1";
        m_pid = GetCurrentProcessId();
        m_socket = nullptr;
    }
    ~SwDebug() {
        if (m_socket) {
            m_socket->close();
            delete m_socket;
        }
    }
    SwDebug(const SwDebug&) = delete;
    SwDebug& operator=(const SwDebug&) = delete;

    SwString m_appName;
    SwString m_version;
    uint32_t m_pid;
    SwTcpSocket* m_socket;
    std::mutex m_mutex;

    void onSocketConnected() {
        // Envoi du message d'identification via JSON
        SwJsonObject initObj;
        initObj.insert("type", SwJsonValue("init"));
        initObj.insert("appName", SwJsonValue(m_appName));
        initObj.insert("version", SwJsonValue(m_version));
        initObj.insert("pid", SwJsonValue((int)m_pid));

        SwJsonDocument initDoc(initObj);
        m_socket->write(initDoc.toJson() + "\n");
    }

    void onSocketError(int errCode) {
        std::cerr << "[ERROR] Socket error: " << errCode << std::endl;
    }

    static SwString levelToString(SwDebugLevel level) {
        switch(level) {
        case SwDebugLevel::Debug: return "DEBUG";
        case SwDebugLevel::Warning: return "WARNING";
        case SwDebugLevel::Error: return "ERROR";
        }
        return "UNKNOWN";
    }

    friend class SwDebugMessage;
};

class SwDebugMessage {
public:
    SwDebugMessage(const SwDebugContext& ctx) : m_ctx(ctx) {}

    template<typename T>
    SwDebugMessage& operator<<(const T& value) {
        m_stream << value;
        return *this;
    }

    ~SwDebugMessage() {
        SwDebug::instance().logMessage(m_ctx, m_stream.str());
    }

private:
    SwDebugContext m_ctx;
    std::ostringstream m_stream;
};

#define swDebug() SwDebugMessage({__FILE__, __LINE__, __FUNCTION__, SwDebugLevel::Debug})
#define swWarning() SwDebugMessage({__FILE__, __LINE__, __FUNCTION__, SwDebugLevel::Warning})
#define swError() SwDebugMessage({__FILE__, __LINE__, __FUNCTION__, SwDebugLevel::Error})
