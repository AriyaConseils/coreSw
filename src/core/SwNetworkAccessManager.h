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

#include "SwObject.h"
#include "SwTcpSocket.h"
#include "SwString.h"

/**
 * @class SwNetworkAccessManager
 * @brief Handles asynchronous HTTP network requests.
 *
 * This class provides functionality to perform HTTP (GET) requests using TCP sockets.
 * It supports adding custom headers, processing HTTP responses (headers and body),
 * and emits signals to notify when requests are completed or errors occur.
 *
 * ### Key Features:
 * - URL parsing and validation for HTTP requests.
 * - Management of custom HTTP headers.
 * - Handles HTTP response parsing, including headers and content body.
 * - Emits signals for asynchronous notification (`finished` and `errorOccurred`).
 *
 * ### Usage:
 * - Use `get(const SwString& url)` to initiate an HTTP GET request.
 * - Add custom headers with `setRawHeader(const SwString& key, const SwString& value)`.
 * - Connect to the `finished` signal to handle the full HTTP response body.
 * - Connect to the `errorOccurred` signal to handle network or protocol errors.
 *
 * @note The class operates asynchronously, leveraging signals and slots for communication.
 */
class SwNetworkAccessManager : public SwObject {
    SW_OBJECT(SwNetworkAccessManager, SwObject)

public:
    /**
     * @brief Constructor for SwNetworkAccessManager.
     *
     * Initializes the network access manager with default values and an optional parent SwObject.
     * Sets up internal state for managing HTTP requests and responses.
     *
     * @param parent Pointer to the parent `SwObject`, if any. Defaults to `nullptr`.
     */
    SwNetworkAccessManager(SwObject* parent = nullptr)
        : SwObject(parent), m_socket(nullptr), m_contentLength(-1), m_bytesReceived(0), m_headersReceived(false)
    {
    }

signals:
    /**
     * @signal finished
     * @brief Emitted when an HTTP response is fully received.
     *
     * This signal is emitted once the HTTP response, including its body, is completely received.
     *
     * @param response The full HTTP response body as a `SwString`.
     */
    DECLARE_SIGNAL(finished, const SwString&) // Réponse HTTP complète reçue

    /**
     * @signal errorOccurred
     * @brief Emitted when an error occurs during the HTTP request process.
     *
     * This signal is emitted whenever an error is encountered, such as a network failure
     * or an invalid response. The error is represented by an integer code.
     *
     * @param errorCode The error code indicating the type of error that occurred.
     */
    DECLARE_SIGNAL(errorOccurred, int)        // Erreur survenue (code d'erreur)

public:
    /**
     * @brief Adds or updates a custom HTTP header for the request.
     *
     * This method allows setting a key-value pair to define or modify HTTP headers
     * that will be sent with the request.
     *
     * @param key The name of the HTTP header as a `SwString` (e.g., `"Content-Type"`).
     * @param value The value of the HTTP header as a `SwString` (e.g., `"application/json"`).
     *
     * @note Headers must be set before initiating the request using `get`.
     */
    void setRawHeader(const SwString &key, const SwString &value){
        m_headerMap[key] = value;
    }

    /**
     * @brief Initiates an asynchronous HTTP GET request to the specified URL.
     *
     * This method parses the provided URL, establishes a connection to the server,
     * and sends a GET request. The process is asynchronous, and the result is
     * delivered via the `finished` or `errorOccurred` signals.
     *
     * @param url The target URL as a `SwString`, formatted as `"http://example.com:8080/test"`
     *            or `"http://example.com/test"`. If the port is not specified, port `80` is used by default.
     *
     * @return `true` if the request was successfully initiated; `false` if there was a failure
     *         (e.g., invalid URL or failed socket connection). Errors are reported via `errorOccurred`.
     *
     * @signals
     * - `finished(const SwString&)`: Emitted when the response is fully received.
     * - `errorOccurred(int)`: Emitted in case of a failure (e.g., network error, invalid response).
     *
     * @note This method clears any previous state before starting a new request.
     * @note Ensure the URL is valid and uses the HTTP protocol.
     */
    bool get(const SwString& url) {
        // Parser l'URL
        SwString host;
        uint16_t port = 80;
        SwString path = "/";

        if (!parseUrl(url, host, port, path)) {
            emit errorOccurred(-1); // erreur parsing URL
            return false;
        }

        // Créer une nouvelle socket pour cette requête
        cleanupSocket();
        m_socket = new SwTcpSocket(this);
        connect(m_socket, SIGNAL(connected), this, &SwNetworkAccessManager::onConnected);
        connect(m_socket, SIGNAL(errorOccurred), this, &SwNetworkAccessManager::onError);
        connect(m_socket, SIGNAL(disconnected), this, &SwNetworkAccessManager::onDisconnected);
        connect(m_socket, SIGNAL(readyRead), this, &SwNetworkAccessManager::onReadyRead);

        // Sauvegarder les informations pour la requête
        m_host = host;
        m_path = path;
        m_contentLength = -1;
        m_bytesReceived = 0;
        m_headersReceived = false;
        m_responseHeaders.clear();
        m_responseBody.clear();

        // Se connecter au serveur
        if (!m_socket->connectToHost(m_host, port)) {
            emit errorOccurred(-2);
            cleanupSocket();
            return false;
        }

        return true;
    }

private slots:
    /**
     * @brief Sends the HTTP GET request to the server upon a successful connection.
     *
     * This slot is triggered when the TCP socket is connected. It constructs the HTTP
     * GET request using the path, host, and custom headers, and sends it through the socket.
     *
     * @note The headers are retrieved from `m_headerMap`, and the request ends with a blank line as per HTTP protocol.
     *
     * @warning Emits `errorOccurred` with code `-3` if the request cannot be written to the socket.
     */
    void onConnected()
    {
        SwString request = "GET " + m_path.chop(1) + " HTTP/1.1\r\n";
        request += "Host: " + m_host + "\r\n";

        for (const auto& header : m_headerMap) {
            request += header.first + ": " + header.second + "\r\n";
        }
        request += "\r\n";


        if (!m_socket->write(request)) {
            emit errorOccurred(-3); // échec d'écriture
            m_socket->close();
        }
    }

    /**
     * @brief Handles incoming data from the TCP socket.
     *
     * This slot is triggered whenever data is available to read from the socket.
     * It processes the HTTP response by:
     * - Reading and buffering the incoming data.
     * - Parsing the headers if they have not yet been fully received.
     * - Accumulating the response body once headers are processed.
     *
     * @note If `Content-Length` is specified in the headers, it ensures the exact amount
     *       of data is read. Otherwise, it waits for the socket to close to consider the response complete.
     *
     * @warning Emits `finished` when the complete HTTP response body is received.
     */
    void onReadyRead() {
        // Lire ce qui est disponible
        SwString data = m_socket->read();
        if (data.isEmpty()) {
            return; // rien de nouveau
        }

        m_buffer.append(data.toStdString());


        // Si les headers ne sont pas encore entièrement reçus, tenter de les séparer du corps
        if (!m_headersReceived) {
            std::size_t pos = m_buffer.find("\r\n\r\n");
            if (pos != std::string::npos) {
                // On a trouvé la fin des headers
                std::string headersPart = m_buffer.substr(0, pos);
                m_buffer.erase(0, pos + 4); // Enlever les headers + la séquence \r\n\r\n

                m_headersReceived = true;
                parseHeaders(headersPart);
            }
        }

        // Si on a déjà reçu les headers, accumuler le body
        if (m_headersReceived) {
            if (m_contentLength >= 0) {
                // On sait combien de données on doit lire
                int toRead = (int)m_buffer.size();
                m_responseBody.append(m_buffer);
                m_buffer.clear();
                m_bytesReceived += toRead;

                if (m_bytesReceived >= m_contentLength) {
                    // On a tout reçu
                    finishedRequest();
                }
            } else {
                // Pas de content-length, on attend la fermeture du socket
                // On accumule tout
                m_responseBody.append(m_buffer);
                m_buffer.clear();
            }
        }
    }

    /**
     * @brief Handles the disconnection of the TCP socket.
     *
     * This slot is triggered when the TCP socket is disconnected. If the HTTP response
     * does not specify a `Content-Length` header, the socket disconnection is considered
     * the end of the response. The response is then finalized, and the socket is cleaned up.
     *
     * @note Emits `finished` if the HTTP response is considered complete upon disconnection.
     */
    void onDisconnected() {
        // Si pas de content-length, on considère la fin du socket comme fin de réponse
        if (!m_headersReceived || (m_contentLength < 0)) {
            // Finir la requête
            finishedRequest();
        }
        cleanupSocket();
    }

    /**
     * @brief Handles errors occurring on the TCP socket.
     *
     * This slot is triggered when an error is encountered during the network operation.
     * It emits the `errorOccurred` signal with the specific error code and performs
     * cleanup of the socket to ensure proper resource management.
     *
     * @param err The error code indicating the type of error encountered.
     *
     * @signal errorOccurred
     */
    void onError(int err) {
        emit errorOccurred(err);
        cleanupSocket();
    }

private:
    SwTcpSocket* m_socket;                          ///< Pointer to the TCP socket used for the network connection.
    SwString m_host;                                ///< The host name or IP address extracted from the URL.
    SwString m_path;                                ///< The HTTP path extracted from the URL.
    SwMap<SwString, SwString> m_headerMap;          ///< Map of HTTP headers to include in the request.
    std::string m_buffer;                           ///< Buffer to temporarily store incoming data from the socket.
    std::string m_responseHeaders;                  ///< Stores the HTTP response headers as a raw string.
    std::string m_responseBody;                     ///< Accumulates the HTTP response body.
    bool m_headersReceived;                         ///< Indicates whether the HTTP headers have been fully received.
    int64_t m_contentLength;                        ///< The expected size of the HTTP response body, if specified by `Content-Length`.
    int64_t m_bytesReceived;                        ///< Tracks the total number of bytes received in the response body.

    /**
     * @brief Cleans up the TCP socket and associated resources.
     *
     * This method disconnects all slots connected to the socket, schedules the socket for deletion,
     * and resets the pointer to `nullptr`. It also clears the internal buffer used for incoming data.
     */
    void cleanupSocket() {
        if (m_socket) {
            m_socket->disconnectAllSlots();
            m_socket->deleteLater();
            m_socket = nullptr;
        }
        m_buffer.clear();
    }

    /**
     * @brief Finalizes the HTTP request and emits the `finished` signal.
     *
     * This method emits the `finished` signal with the complete HTTP response body and
     * cleans up the TCP socket to release resources.
     *
     * @signal finished
     */
    void finishedRequest() {
        // Émettre le signal finished avec le body
        emit finished(SwString(m_responseBody.c_str()));
        cleanupSocket();
    }

    /**
     * @brief Parses a given URL into its components: host, port, and path.
     *
     * This method extracts the host, port (if specified), and path from a URL of the form:
     * `http://host[:port]/path`. If the port is not explicitly specified, it defaults to 80.
     *
     * @param url The URL to parse (e.g., `"http://example.com:8080/test"`).
     * @param host Reference to a `SwString` where the extracted host will be stored.
     * @param port Reference to a `uint16_t` where the extracted port will be stored. Defaults to 80 if not specified in the URL.
     * @param path Reference to a `SwString` where the extracted path will be stored. Defaults to `"/"` if not specified in the URL.
     *
     * @return `true` if the URL was successfully parsed, `false` otherwise.
     *
     * @note The method assumes the URL uses the `http://` scheme. Other schemes are not supported.
     */
    bool parseUrl(const SwString& url, SwString& host, uint16_t& port, SwString& path) {
        // On suppose des URL du type http://host[:port]/path
        // Retirer "http://"
        SwString lower = url.toLower();
        if (lower.startsWith("http://")) {
            SwString remainder = url.mid(7);
            int slashPos = remainder.indexOf("/");
            SwString hostPortPart;
            if (slashPos >= 0) {
                hostPortPart = remainder.left(slashPos);
                path = remainder.mid(slashPos);
            } else {
                hostPortPart = remainder;
                path = "/";
            }

            // Chercher le port
            int colonPos = hostPortPart.indexOf(":");
            if (colonPos >= 0) {
                host = hostPortPart.left(colonPos);
                SwString portStr = hostPortPart.mid(colonPos + 1);
                bool ok = false;
                int p = portStr.toInt(&ok);
                if (!ok || p <= 0 || p > 65535) {
                    return false;
                }
                port = (uint16_t)p;
            } else {
                host = hostPortPart;
                port = 80;
            }

            if (path.isEmpty()) {
                path = "/";
            }

            return !host.isEmpty();
        } else {
            // Pas de http://
            return false;
        }
    }

    /**
     * @brief Parses HTTP response headers and extracts relevant information.
     *
     * This method processes the raw HTTP headers, splits them into individual lines,
     * and extracts specific details such as the `Content-Length` value, storing it
     * in the `m_contentLength` member.
     *
     * @param headersPart The raw HTTP headers as a single string.
     *
     * @details The function:
     * - Splits headers by `\r\n`.
     * - Searches for the `Content-Length` header.
     * - Trims unnecessary whitespace from values.
     * - Converts the `Content-Length` value to an integer and updates `m_contentLength`.
     *
     * @warning If the `Content-Length` header is not found or its value is invalid,
     *          `m_contentLength` is set to `-1`.
     */
    void parseHeaders(const SwString& headersPart) {
        m_responseHeaders = headersPart.toStdString(); // Stocke les headers en tant que chaîne brute

        // Séparer les headers ligne par ligne
        auto lines = headersPart.split("\r\n");
        for (const SwString& line : lines) {
            if (line.isEmpty()) {
                continue;
            }

            // Chercher "content-length"
            SwString lowerLine = line.toLower();
            if (lowerLine.startsWith("content-length:")) {
                // Extraire la valeur après "content-length:"
                SwString val = line.mid(strlen("Content-Length:")).trimmed();

                // Convertir la valeur en entier
                bool ok = false;
                m_contentLength = val.toInt(&ok);
                if (!ok) {
                    m_contentLength = -1; // Erreur de parsing
                }
            }
        }
    }
};
