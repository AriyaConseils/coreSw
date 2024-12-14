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

#include "Object.h"
#include "SwString.h"
#include "SwTcpSocket.h"
#include "SwTimer.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

class SwTcpServer : public Object {
    SW_OBJECT(SwTcpServer, Object)
public:
    SwTcpServer(Object* parent = nullptr)
        : Object(parent), m_listenSocket(INVALID_SOCKET), m_listenEvent(NULL)
    {
        initializeWinsock();
        m_timer = new SwTimer(50, this);
        connect(m_timer, SIGNAL(timeout), this, &SwTcpServer::onCheckEvents);
        m_timer->start();
    }

    virtual ~SwTcpServer() {
        close();
    }

    bool listen(uint16_t port) {
        close();

        m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (m_listenSocket == INVALID_SOCKET) {
            std::cerr << "WSASocket failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        u_long mode = 1;
        ioctlsocket(m_listenSocket, FIONBIO, &mode);

        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (::bind(m_listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
            close();
            return false;
        }

        if (::listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
            close();
            return false;
        }

        m_listenEvent = WSACreateEvent();
        if (m_listenEvent == WSA_INVALID_EVENT) {
            std::cerr << "WSACreateEvent failed: " << WSAGetLastError() << std::endl;
            close();
            return false;
        }

        if (WSAEventSelect(m_listenSocket, m_listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR) {
            std::cerr << "WSAEventSelect failed: " << WSAGetLastError() << std::endl;
            close();
            return false;
        }

        return true;
    }

    void close() {
        if (m_listenSocket != INVALID_SOCKET) {
            closesocket(m_listenSocket);
            m_listenSocket = INVALID_SOCKET;
        }
        if (m_listenEvent) {
            WSACloseEvent(m_listenEvent);
            m_listenEvent = NULL;
        }
    }

    SwTcpSocket* nextPendingConnection() {
        if (m_pendingConnections.isEmpty()) {
            return nullptr;
        }
        SwTcpSocket* sock = m_pendingConnections.first();
        m_pendingConnections.removeFirst();
        return sock;
    }

signals:
    DECLARE_SIGNAL(newConnection)

private slots:
    void onCheckEvents() {
        if (m_listenEvent == NULL || m_listenSocket == INVALID_SOCKET) {
            return;
        }

        DWORD res = WSAWaitForMultipleEvents(1, &m_listenEvent, FALSE, 0, FALSE);
        if (res == WSA_WAIT_TIMEOUT) {
            return;
        }

        WSAResetEvent(m_listenEvent);

        WSANETWORKEVENTS networkEvents;
        if (WSAEnumNetworkEvents(m_listenSocket, m_listenEvent, &networkEvents) == SOCKET_ERROR) {
            std::cerr << "WSAEnumNetworkEvents error: " << WSAGetLastError() << std::endl;
            return;
        }

        if (networkEvents.lNetworkEvents & FD_ACCEPT) {
            if (networkEvents.iErrorCode[FD_ACCEPT_BIT] == 0) {
                SOCKET clientSocket = accept(m_listenSocket, NULL, NULL);
                if (clientSocket == INVALID_SOCKET) {
                    std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
                } else {
                    SwTcpSocket* client = createSocketFromHandle(clientSocket);
                    m_pendingConnections.append(client);
                    emit newConnection();
                }
            } else {
                std::cerr << "FD_ACCEPT error: " << networkEvents.iErrorCode[FD_ACCEPT_BIT] << std::endl;
            }
        }

        if (networkEvents.lNetworkEvents & FD_CLOSE) {
            std::cerr << "Listen socket closed by external event." << std::endl;
            close();
        }
    }

private:
    SOCKET m_listenSocket;
    WSAEVENT m_listenEvent;
    SwTimer* m_timer;
    SwList<SwTcpSocket*> m_pendingConnections;

    static void initializeWinsock() {
        static bool initialized = false;
        if (!initialized) {
            WSADATA wsaData;
            int result = WSAStartup(MAKEWORD(2,2), &wsaData);
            if (result != 0) {
                std::cerr << "WSAStartup failed: " << result << std::endl;
            } else {
                initialized = true;
            }
        }
    }

    SwTcpSocket* createSocketFromHandle(SOCKET sock) {
        SwTcpSocket* client = new SwTcpSocket();
        client->adoptSocket(sock);
        return client;
    }
};
