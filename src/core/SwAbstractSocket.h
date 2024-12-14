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

#define _WINSOCKAPI_

#include "SwIODevice.h"
#include "SwString.h"

/**
 * @class SwAbstractSocket
 * @brief Abstract base class for socket-based communication.
 *
 * This class defines the interface and common behavior for socket operations,
 * including connecting to a host, reading and writing data, and handling socket state transitions.
 *
 * ### Key Features:
 * - Abstract methods for connecting, reading, writing, and waiting for socket events.
 * - Signal-based architecture to notify about important events such as connection establishment, data availability, and errors.
 * - Manages the socket's state with an internal `SocketState` enum.
 *
 * @note This class is intended to be subclassed to implement specific socket protocols (e.g., TCP, UDP).
 */
class SwAbstractSocket : public SwIODevice {
    SW_OBJECT(SwAbstractSocket, SwIODevice)
public:

    /**
     * @enum SocketState
     * @brief Represents the various states a socket can be in.
     *
     * - `UnconnectedState`: The socket is not connected.
     * - `HostLookupState`: The socket is resolving the host address.
     * - `ConnectingState`: The socket is attempting to connect to a host.
     * - `ConnectedState`: The socket is connected to a host.
     * - `BoundState`: The socket is bound to an address and port.
     * - `ListeningState`: The socket is in listening mode (server sockets).
     * - `ClosingState`: The socket is closing.
     */
    enum SocketState {
        UnconnectedState,
        HostLookupState,
        ConnectingState,
        ConnectedState,
        BoundState,
        ListeningState,
        ClosingState
    };

    /**
     * @brief Constructs an abstract socket object.
     *
     * @param parent A pointer to the parent object. Defaults to `nullptr`.
     *
     * @details Initializes the socket state to `UnconnectedState`.
     */
    SwAbstractSocket(Object* parent = nullptr)
        : SwIODevice(parent), m_state(UnconnectedState)
    {
    }

    /**
     * @brief Virtual destructor for `SwAbstractSocket`.
     *
     * Ensures proper cleanup in derived classes.
     */
    virtual ~SwAbstractSocket() {}

    /**
     * @brief Attempts to connect to the specified host and port.
     *
     * @param host The hostname or IP address to connect to.
     * @param port The port number on the host.
     *
     * @return `true` if the connection is initiated successfully, `false` otherwise.
     *
     * @note This is a pure virtual method and must be implemented in derived classes.
     */
    virtual bool connectToHost(const SwString& host, uint16_t port) = 0;

    /**
     * @brief Waits for the socket to establish a connection within the specified timeout.
     *
     * @param msecs The maximum time to wait, in milliseconds. Defaults to 30,000 ms (30 seconds).
     *
     * @return `true` if the socket successfully connects within the timeout, `false` otherwise.
     *
     * @note This is a pure virtual method and must be implemented in derived classes.
     */
    virtual bool waitForConnected(int msecs = 30000) = 0;

    /**
     * @brief Waits for data in the write buffer to be sent within the specified timeout.
     *
     * @param msecs The maximum time to wait, in milliseconds. Defaults to 30,000 ms (30 seconds).
     *
     * @return `true` if the data is sent within the timeout, `false` otherwise.
     *
     * @note This is a pure virtual method and must be implemented in derived classes.
     */
    virtual bool waitForBytesWritten(int msecs = 30000) = 0;

    /**
     * @brief Closes the socket and releases associated resources.
     *
     * @note This is a pure virtual method and must be implemented in derived classes.
     */
    virtual void close() override = 0;

    /**
     * @brief Reads data from the socket.
     *
     * @param maxSize The maximum number of bytes to read. Defaults to `0`, which indicates no limit.
     *
     * @return A `SwString` containing the data read from the socket.
     *
     * @note This is a pure virtual method and must be implemented in derived classes.
     */
    virtual SwString read(int64_t maxSize = 0) override = 0;

    /**
     * @brief Writes data to the socket.
     *
     * @param data The data to write, provided as a `SwString`.
     *
     * @return `true` if the data was successfully queued for sending, `false` otherwise.
     *
     * @note This is a pure virtual method and must be implemented in derived classes.
     */
    virtual bool write(const SwString& data) override = 0;

    /**
     * @brief Checks if the socket is currently open.
     *
     * @return `true` if the socket is in the `ConnectedState`, `false` otherwise.
     */
    virtual bool isOpen() const override {
        return (m_state == ConnectedState);
    }

    /**
     * @brief Returns the current state of the socket.
     *
     * @return The current `SocketState` of the socket.
     */
    SocketState state() const {
        return m_state;
    }

signals:
    DECLARE_SIGNAL(connected)            ///< Emitted when the socket successfully establishes a connection.
    DECLARE_SIGNAL(disconnected)         ///< Emitted when the socket is disconnected.
    DECLARE_SIGNAL(readyRead)            ///< Emitted when data is available for reading.
    DECLARE_SIGNAL(errorOccurred, int)   ///< Emitted when a socket error occurs. The error code is passed as an argument.
    DECLARE_SIGNAL(writeFinished)        ///< Emitted when all data in the write buffer has been sent.

protected:
    /**
     * @brief Sets the socket's state.
     *
     * @param newState The new state to assign to the socket.
     *
     * @details This method is protected and is intended to be used by derived classes to update the socket's state.
     */
    void setState(SocketState newState) {
        m_state = newState;
    }

private:
    SocketState m_state; ///< Holds the current state of the socket, represented by the `SocketState` enum.
};
