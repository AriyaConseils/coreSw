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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCKAPI_

#include "SwAbstractSocket.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")


/**
 * @class SwTcpSocket
 * @brief Provides an implementation of a TCP socket using the Winsock2 library.
 *
 * This class extends `SwAbstractSocket` to implement TCP communication functionalities.
 * It supports non-blocking operations, event-based monitoring, and integration with
 * the custom signal/slot system for asynchronous handling of connections, data transfers, and errors.
 *
 * ### Key Features:
 * - Establishes TCP connections with `connectToHost`.
 * - Supports non-blocking reads and writes.
 * - Allows monitoring of socket events like `FD_CONNECT`, `FD_READ`, `FD_WRITE`, and `FD_CLOSE`.
 * - Emits signals for key events such as connection established, data ready to read, errors, and disconnections.
 * - Manages write buffering to handle partial sends in non-blocking mode.
 *
 * @note This implementation is specific to Windows platforms, leveraging Winsock2 for socket operations.
 */
class SwTcpSocket : public SwAbstractSocket {
    SW_OBJECT(SwTcpSocket, SwAbstractSocket)

public:
    /**
     * @brief Constructs a `SwTcpSocket` object and initializes Winsock.
     *
     * This constructor sets up the TCP socket object with default values, initializes the Winsock library,
     * and starts monitoring for socket events.
     *
     * @param parent A pointer to the parent object. Defaults to `nullptr`.
     *
     * @note The socket is initialized to `INVALID_SOCKET`, and no connection is established at construction.
     * @warning Ensure that the parent object properly manages the lifecycle of this socket.
     */
    SwTcpSocket(Object* parent = nullptr)
        : SwAbstractSocket(parent), m_socket(INVALID_SOCKET), m_event(NULL) {
        initializeWinsock();
        startMonitoring();
    }

    /**
     * @brief Destructor for `SwTcpSocket`.
     *
     * Cleans up resources associated with the socket, including stopping event monitoring
     * and closing the underlying socket connection.
     *
     * @note This ensures proper release of Winsock resources and internal states.
     */
    virtual ~SwTcpSocket() {
        stopMonitoring();
        close();
    }

    /**
     * @brief Establishes a TCP connection to the specified host and port.
     *
     * This method creates a non-blocking socket, sets up event monitoring, resolves the
     * host's DNS, and initiates the connection. It supports IPv4 and emits appropriate
     * error signals if any step fails.
     *
     * @param host The hostname or IP address to connect to.
     * @param port The port number on the host.
     *
     * @return `true` if the connection is successfully initiated, `false` if an error occurs.
     *
     * @details
     * - The method closes any existing socket before initiating a new connection.
     * - It resolves the hostname using `getaddrinfo`.
     * - The socket is set to non-blocking mode to handle asynchronous connections.
     * - Emits `errorOccurred` for DNS resolution errors or connection failures.
     *
     * @note If the connection cannot be immediately established, the method returns `true`
     *       and waits for the `FD_CONNECT` event to finalize the connection.
     */
    bool connectToHost(const SwString& host, uint16_t port) override {
        close(); // Fermer toute connexion précédente

        m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (m_socket == INVALID_SOCKET) {
            int wsaErr = WSAGetLastError();
            std::cerr << "[DEBUG] WSASocket failed: " << wsaErr << std::endl;
            emit errorOccurred(wsaErr);
            return false;
        }
        std::cerr << "[DEBUG] Socket créé avec succès." << std::endl;

        // Mettre la socket en non-bloquant
        u_long mode = 1;
        if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
            int wsaErr = WSAGetLastError();
            std::cerr << "[DEBUG] ioctlsocket(FIONBIO) failed: " << wsaErr << std::endl;
            emit errorOccurred(wsaErr);
            close();
            return false;
        }

        m_event = WSACreateEvent();
        if (m_event == WSA_INVALID_EVENT) {
            int wsaErr = WSAGetLastError();
            std::cerr << "[DEBUG] WSACreateEvent failed: " << wsaErr << std::endl;
            emit errorOccurred(wsaErr);
            close();
            return false;
        }

        if (WSAEventSelect(m_socket, m_event, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR) {
            int wsaErr = WSAGetLastError();
            std::cerr << "[DEBUG] WSAEventSelect failed: " << wsaErr << std::endl;
            emit errorOccurred(wsaErr);
            close();
            return false;
        }

        // Résolution du nom d'hôte
        std::string hostAnsi = host.toStdString();
        struct addrinfo hints = {0};
        hints.ai_family = AF_INET;          // IPv4
        hints.ai_socktype = SOCK_STREAM;    // TCP
        hints.ai_protocol = IPPROTO_TCP;

        struct addrinfo* result = nullptr;
        std::string portStr = std::to_string(port);
        int rc = getaddrinfo(hostAnsi.c_str(), portStr.c_str(), &hints, &result);
        if (rc != 0 || !result) {
            std::cerr << "[DEBUG] getaddrinfo failed for host: " << hostAnsi
                      << ", port: " << port << " Error: " << rc << std::endl;
            emit errorOccurred(-1); // Erreur de résolution DNS
            close();
            return false;
        }

        std::cerr << "[DEBUG] Résolution DNS réussie pour " << hostAnsi << ":" << port << std::endl;

        // On tente la connexion avec la première adresse retournée
        sockaddr_in* addr_in = reinterpret_cast<sockaddr_in*>(result->ai_addr);

        setState(ConnectingState);
        int resultConnect = ::connect(m_socket, (sockaddr*)addr_in, (int)result->ai_addrlen);
        freeaddrinfo(result);

        if (resultConnect == SOCKET_ERROR) {
            int err = WSAGetLastError();
            // WSAEWOULDBLOCK et WSAEINPROGRESS sont normaux en mode non-bloquant
            if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
                std::cerr << "[DEBUG] connect failed: " << err << std::endl;
                emit errorOccurred(err);
                close();
                return false;
            }
            std::cerr << "[DEBUG] Connexion en cours (non bloquante), en attente d'événement FD_CONNECT..." << std::endl;
        } else {
            std::cerr << "[DEBUG] Connexion établie immédiatement." << std::endl;
        }

        return true;
    }

    /**
     * @brief Waits for the socket to establish a connection within the specified timeout.
     *
     * This method blocks the calling thread until the socket transitions to the `ConnectedState`
     * or the timeout duration expires.
     *
     * @param msecs The maximum time to wait for the connection, in milliseconds. Defaults to 30,000 ms (30 seconds).
     *
     * @return `true` if the socket successfully connects within the specified time, `false` if the timeout expires.
     *
     * @details
     * - Uses a condition function to monitor the socket's state.
     * - Continuously checks for the connection state within the specified timeout.
     *
     * @note This method is blocking and should not be used in the main GUI thread to avoid freezing the application.
     */
    bool waitForConnected(int msecs = 30000) override {
        return waitForCondition([this]() { return state() == ConnectedState; }, msecs);
    }

    /**
     * @brief Closes the TCP socket and cleans up associated resources.
     *
     * This method shuts down the socket, releases the event handle, clears the write buffer,
     * and resets the socket state to `UnconnectedState`. If the socket was in a connected or
     * connecting state, it emits the `disconnected` signal.
     *
     * @details
     * - Ensures graceful shutdown by disabling linger options if not already set.
     * - Calls `closesocket` to close the underlying Winsock socket.
     * - Clears any remaining data in the internal write buffer.
     * - Emits `disconnected` if the socket was previously in `ConnectedState`, `ConnectingState`,
     *   or `ClosingState`.
     *
     * @note This method is safe to call multiple times, as it checks the socket state before attempting cleanup.
     */
    void close() override {
        if (m_socket != INVALID_SOCKET) {
            // Remettre linger par défaut (si pas déjà fait)
            disableLinger();

            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
        if (m_event) {
            WSACloseEvent(m_event);
            m_event = NULL;
        }

        m_writeBuffer.clear();

        if (state() == ConnectedState || state() == ConnectingState || state() == ClosingState) {
            emit disconnected();
        }

        setState(UnconnectedState);
    }

    /**
     * @brief Reads data from the socket up to the specified maximum size.
     *
     * This method attempts to read data from the TCP socket. If successful, it returns the received data
     * as a `SwString`. If the socket is closed or an error occurs, it handles the situation appropriately.
     *
     * @param maxSize The maximum number of bytes to read. Defaults to 0, which allows reading up to 1024 bytes.
     *
     * @return A `SwString` containing the data read from the socket. Returns an empty string if no data is available or an error occurs.
     *
     * @details
     * - If `maxSize` is greater than 0 and less than 1024, it reads up to `maxSize` bytes; otherwise, it reads up to 1024 bytes.
     * - If the connection is closed by the peer (`recv` returns 0), the socket is closed locally.
     * - If an error occurs, it emits the `errorOccurred` signal unless the error is `WSAEWOULDBLOCK` (non-blocking read).
     *
     * @note This method is non-blocking and relies on the socket's state being `ConnectedState`.
     */
    SwString read(int64_t maxSize = 0) override {
        if (m_socket == INVALID_SOCKET || state() != ConnectedState)
            return "";

        char buffer[1024];
        int flags = 0;
        int sizeToRead = (maxSize > 0 && maxSize < 1024) ? (int)maxSize : 1024;
        int ret = ::recv(m_socket, buffer, sizeToRead, flags);
        if (ret > 0) {
            return SwString::fromLatin1(buffer, ret);
        } else if (ret == 0) {
            close();
        } else {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                emit errorOccurred(err);
            }
        }
        return "";
    }

    /**
     * @brief Writes data to the socket.
     *
     * This method queues the provided data into the internal write buffer and attempts
     * to send it to the socket. If the socket cannot immediately send all data, the remaining
     * data is retained in the buffer for subsequent writes.
     *
     * @param data The data to be sent, provided as a `SwString`.
     *
     * @return `true` if the data was successfully queued for sending, `false` if the socket is invalid or not connected.
     *
     * @details
     * - Appends the data to the internal write buffer (`m_writeBuffer`).
     * - Calls `tryFlushWriteBuffer` to attempt immediate sending of the data.
     * - If the socket is non-blocking and cannot send all data at once, the remaining data stays in the buffer.
     *
     * @note Emits `writeFinished` when the buffer is fully flushed to the socket.
     */
    bool write(const SwString& data) override {
        if (m_socket == INVALID_SOCKET || state() != ConnectedState)
            return false;

        m_writeBuffer.append(data.toStdString());

        tryFlushWriteBuffer();

        return true;
    }

    /**
     * @brief Waits for all data in the write buffer to be sent to the socket.
     *
     * This method blocks the calling thread until all data in the internal write buffer is sent,
     * an error occurs, or the timeout expires.
     *
     * @param msecs The maximum time to wait for the data to be written, in milliseconds. Defaults to 30,000 ms (30 seconds).
     *              A negative value indicates no timeout (wait indefinitely).
     *
     * @return `true` if all data was successfully sent within the specified time, `false` if the timeout expired or an error occurred.
     *
     * @details
     * - Continuously monitors the socket's write events using `WSAWaitForMultipleEvents`.
     * - Updates the remaining time and exits if the timeout expires.
     * - Resets the socket event and processes pending socket events via `checkSocketEvents`.
     * - Emits `errorOccurred` if an error occurs during the process.
     *
     * @note This method is blocking and should be used cautiously in the main GUI thread to avoid freezing the application.
     */
    bool waitForBytesWritten(int msecs = 30000) {
        using namespace std::chrono;
        auto start = steady_clock::now();
        int timeout = (msecs < 0) ? -1 : msecs;

        // Tant qu'il reste des données à envoyer
        while (!m_writeBuffer.empty()) {
            // Calcul du temps restant
            int remainingTime = -1;
            if (timeout >= 0) {
                auto now = steady_clock::now();
                auto elapsed = duration_cast<milliseconds>(now - start).count();
                if ((int)elapsed >= timeout) {
                    // Délai dépassé
                    return false;
                }
                remainingTime = timeout - (int)elapsed;
            }

            DWORD waitTime = (remainingTime < 0) ? WSA_INFINITE : (DWORD)remainingTime;
            DWORD result = WSAWaitForMultipleEvents(1, &m_event, FALSE, waitTime, FALSE);
            if (result == WSA_WAIT_FAILED) {
                emit errorOccurred(WSAGetLastError());
                return false;
            } else if (result == WSA_WAIT_TIMEOUT) {
                // Délai expiré sans que tout ait été envoyé
                return false;
            }

            // Un événement s’est produit
            WSAResetEvent(m_event);
            if (!checkSocketEvents()) {
                // Une erreur est survenue
                return false;
            }
        }
        Sleep(1);
        return true;
    }

    /**
     * @brief Signals the end of data transmission and ensures the client receives all sent data.
     *
     * This method shuts down the write operation of the socket, indicating that no more data
     * will be sent. It optionally enables the linger option to ensure the socket waits for
     * acknowledgment (ACK) of all sent data before closing.
     *
     * @param lingerSeconds The duration in seconds to wait for the client to acknowledge the sent data.
     *                      Defaults to 5 seconds.
     *
     * @return `true` if the write operation was successfully shut down, `false` if the socket is invalid,
     *         not connected, or an error occurs.
     *
     * @details
     * - Calls `shutdown` with `SD_SEND` to terminate the write side of the socket.
     * - If `shutdown` fails, emits the `errorOccurred` signal with the corresponding error code.
     * - Enables the linger option with the specified timeout to ensure proper socket closure.
     *
     * @note This method should be called before invoking `close()` to gracefully terminate the connection.
     */
    bool shutdownWrite(int lingerSeconds = 5) {
        if (m_socket == INVALID_SOCKET || state() != ConnectedState)
            return false;

        // Terminer proprement l'envoi
        if (shutdown(m_socket, SD_SEND) == SOCKET_ERROR) {
            emit errorOccurred(WSAGetLastError());
            return false;
        }

        // Activer SO_LINGER pour s'assurer que close() attendra l'ACK des données
        enableLinger(lingerSeconds);
        return true;
    }

    /**
     * @brief Adopts an existing socket and integrates it with the `SwTcpSocket` instance.
     *
     * This method takes ownership of an already created Winsock socket, configures it for non-blocking
     * mode, sets up event monitoring, and updates the internal state to reflect the connection status.
     *
     * @param sock The existing `SOCKET` to be adopted by this instance.
     *
     * @details
     * - Closes any previously managed socket before adopting the new one.
     * - Sets the adopted socket to non-blocking mode using `ioctlsocket`.
     * - Creates an event handle for the socket and configures it to monitor `FD_READ`, `FD_WRITE`, and `FD_CLOSE` events.
     * - Updates the internal state to `ConnectedState` and starts monitoring for events.
     * - Emits the `connected` signal to indicate that the socket is now managed and connected.
     *
     * @note The caller must ensure the validity of the socket being passed.
     */
    void adoptSocket(SOCKET sock) {
        close();
        m_socket = sock;
        if (m_socket != INVALID_SOCKET) {
            u_long mode = 1;
            ioctlsocket(m_socket, FIONBIO, &mode);

            m_event = WSACreateEvent();
            WSAEventSelect(m_socket, m_event, FD_READ | FD_WRITE | FD_CLOSE);

            setState(ConnectedState);
            startMonitoring();
            emit connected();
        }
    }

protected:
    /**
     * @brief Handles periodic checks for socket events.
     *
     * This method overrides the base class implementation to perform additional
     * monitoring of socket events by calling `checkSocketEvents`.
     *
     * @details
     * - Calls the base class `onTimerDescriptor` for standard descriptor management.
     * - Invokes `checkSocketEvents` to process pending network events for the socket.
     *
     * @note This method is part of the event loop and is called periodically to handle
     *       socket-related updates.
     */
    void onTimerDescriptor() override {
        SwIODevice::onTimerDescriptor();
        checkSocketEvents();
    }

private:
    SOCKET m_socket;               ///< The Winsock socket handle used for TCP communication.
    WSAEVENT m_event;              ///< The event handle used for monitoring socket events.
    std::string m_writeBuffer;     ///< Internal buffer to store data for partial writes in non-blocking mode.

    /**
     * @brief Initializes the Winsock library for network operations.
     *
     * This static method ensures that Winsock is initialized only once during the program's
     * execution. It calls `WSAStartup` to load the Winsock library and prepares it for use.
     *
     * @details
     * - Uses a static boolean flag to prevent multiple initializations.
     * - Logs an error message to `std::cerr` if `WSAStartup` fails.
     * - Must be called before any Winsock-dependent functionality is used.
     *
     * @note This method is thread-safe due to the static variable ensuring one-time initialization.
     */
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

    /**
     * @brief Checks and processes pending socket events.
     *
     * This method monitors the socket for network events such as connection completion,
     * data availability, write readiness, or disconnection. It emits appropriate signals
     * and updates the socket's state based on the events detected.
     *
     * @return `true` if the socket is in a valid state after processing events, `false` if an error occurs.
     *
     * @details
     * - Waits for events associated with the socket's event handle (`m_event`).
     * - Resets the event after processing to prepare for future monitoring.
     * - Handles the following events:
     *   - `FD_CONNECT`: Emits `connected` if successful, or `errorOccurred` if an error occurs.
     *   - `FD_READ`: Emits `readyRead` if data is available, or `errorOccurred` if an error occurs.
     *   - `FD_WRITE`: Flushes the write buffer or emits `errorOccurred` if an error occurs.
     *   - `FD_CLOSE`: Transitions the socket to `ClosingState` and closes it.
     * - If `WSAEnumNetworkEvents` fails, emits the `errorOccurred` signal with the error code.
     *
     * @note This method should be called periodically to handle pending socket events.
     */
    bool checkSocketEvents() {
        if (m_event == NULL || m_socket == INVALID_SOCKET) {
            return true;
        }

        DWORD res = WSAWaitForMultipleEvents(1, &m_event, FALSE, 0, FALSE);
        if (res == WSA_WAIT_TIMEOUT) {
            return true;
        }

        WSAResetEvent(m_event);

        WSANETWORKEVENTS networkEvents;
        if (WSAEnumNetworkEvents(m_socket, m_event, &networkEvents) == SOCKET_ERROR) {
            emit errorOccurred(WSAGetLastError());
            return false;
        }

        if (networkEvents.lNetworkEvents & FD_CONNECT) {
            if (networkEvents.iErrorCode[FD_CONNECT_BIT] == 0) {
                setState(ConnectedState);
                emit connected();
            } else {
                emit errorOccurred(networkEvents.iErrorCode[FD_CONNECT_BIT]);
                close();
            }
        }

        if (networkEvents.lNetworkEvents & FD_READ) {
            if (networkEvents.iErrorCode[FD_READ_BIT] == 0) {
                emit readyRead();
            } else {
                emit errorOccurred(networkEvents.iErrorCode[FD_READ_BIT]);
            }
        }

        if (networkEvents.lNetworkEvents & FD_WRITE) {
            if (networkEvents.iErrorCode[FD_WRITE_BIT] != 0) {
                emit errorOccurred(networkEvents.iErrorCode[FD_WRITE_BIT]);
            } else {
                tryFlushWriteBuffer();
            }
        }

        if (networkEvents.lNetworkEvents & FD_CLOSE) {
            setState(ClosingState);
            close();
        }

        return true;
    }

    /**
     * @brief Attempts to flush the internal write buffer to the socket.
     *
     * This method sends data from the internal write buffer (`m_writeBuffer`) to the socket.
     * If the data cannot be fully sent, the remaining data stays in the buffer for future attempts.
     *
     * @details
     * - If the socket is invalid, not connected, or the buffer is empty, the method exits immediately.
     * - Sends data using `send` and removes the transmitted portion from the buffer.
     * - Emits the `writeFinished` signal when the buffer is fully flushed.
     * - If `send` fails due to a non-recoverable error, emits the `errorOccurred` signal.
     * - If the error is `WSAEWOULDBLOCK`, it waits for the `FD_WRITE` event to retry the operation.
     *
     * @note This method does not block; it immediately returns after attempting to send data.
     */
    void tryFlushWriteBuffer() {
        if (m_socket == INVALID_SOCKET || m_writeBuffer.empty() || state() != ConnectedState)
            return;

        const char* dataPtr = m_writeBuffer.data();
        int dataSize = (int)m_writeBuffer.size();

        int ret = ::send(m_socket, dataPtr, dataSize, 0);
        if (ret > 0) {
            m_writeBuffer.erase(0, ret);
            if (m_writeBuffer.empty()) {
                emit writeFinished();
            }
        } else if (ret == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                emit errorOccurred(err);
            }
            // On attend FD_WRITE pour réessayer
        }
    }

    /**
     * @brief Waits for a specified condition to be met within a given timeout.
     *
     * This method repeatedly checks a user-defined condition and blocks the calling thread
     * until the condition is satisfied or the timeout expires.
     *
     * @tparam Condition A callable that returns a boolean indicating whether the condition is met.
     *
     * @param condition The condition to be evaluated in each iteration.
     * @param msecs The maximum time to wait for the condition, in milliseconds.
     *              A negative value indicates no timeout (wait indefinitely).
     *
     * @return `true` if the condition is satisfied within the timeout, `false` if the timeout expires.
     *
     * @details
     * - Continuously evaluates the condition in a loop with a 10 ms delay between checks.
     * - Uses `std::chrono` to measure elapsed time for timeout calculations.
     * - Calls `checkSocketEvents` periodically to process pending network events during the wait.
     *
     * @note This method is blocking and should be used cautiously to avoid freezing the application.
     */
    template<typename Condition>
    bool waitForCondition(Condition condition, int msecs) {
        using namespace std::chrono;
        auto start = steady_clock::now();
        int timeout = (msecs < 0) ? -1 : msecs;

        while (!condition()) {
            if (timeout >= 0) {
                auto now = steady_clock::now();
                auto elapsed = duration_cast<milliseconds>(now - start).count();
                if (elapsed >= timeout) {
                    return false;
                }
            }

            Sleep(10);
            checkSocketEvents();
        }
        return true;
    }

    /**
     * @brief Enables the linger option on the socket to control its closure behavior.
     *
     * This method sets the linger option, causing the socket to block during closure until
     * all pending data is sent or the specified timeout expires.
     *
     * @param seconds The linger timeout in seconds. If set to `0`, the socket closes immediately.
     *
     * @details
     * - Configures the socket using the `setsockopt` function with the `SO_LINGER` option.
     * - Setting the linger option is particularly useful for ensuring that the peer receives all
     *   transmitted data before the socket is closed.
     *
     * @note This method should be called only when the socket is in a valid state.
     */
    void enableLinger(int seconds) {
        linger lng;
        lng.l_onoff = 1;
        lng.l_linger = seconds;
        setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (const char*)&lng, sizeof(lng));
    }

    /**
     * @brief Disables the linger option on the socket.
     *
     * This method configures the socket to close immediately without waiting for pending data
     * to be transmitted or acknowledged by the peer.
     *
     * @details
     * - Sets the `SO_LINGER` option to off using `setsockopt`.
     * - Ensures the socket closes without delay, regardless of unacknowledged data.
     *
     * @note This method should be called when immediate socket closure is required.
     */
    void disableLinger() {
        linger lng;
        lng.l_onoff = 0;
        lng.l_linger = 0;
        setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (const char*)&lng, sizeof(lng));
    }
};
