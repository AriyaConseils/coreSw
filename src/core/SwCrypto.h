#pragma once

#include <Windows.h>
#include <bcrypt.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <fstream>


// Lien avec bcrypt.lib pour utiliser les fonctions de hachage et cryptage
#pragma comment(lib, "bcrypt.lib")

class SwCrypto {
public:
    // Génération de hachage SHA256
    static std::vector<unsigned char> generateHashSHA256(const std::string& input) {
        return generateHash(input, BCRYPT_SHA256_ALGORITHM);
    }

    // Génération de hachage SHA512
    static std::vector<unsigned char> generateHashSHA512(const std::string& input) {
        return generateHash(input, BCRYPT_SHA512_ALGORITHM);
    }

    // Génération d'un hachage SHA256 en string
    static std::string hashSHA256(const std::string& input) {
        return hashToString(input, BCRYPT_SHA256_ALGORITHM);
    }

    // Génération d'un hachage SHA512 en string
    static std::string hashSHA512(const std::string& input) {
        return hashToString(input, BCRYPT_SHA512_ALGORITHM);
    }

    // Génération de hachage HMAC-SHA256 basé sur une clé
    static std::vector<unsigned char> generateKeyedHashSHA256(const std::string& data, const std::string& key) {
        return generateKeyedHash(data, key, BCRYPT_SHA256_ALGORITHM);
    }

    // Cryptage AES
    static std::vector<unsigned char> encryptAES(const std::vector<unsigned char>& data, const std::vector<unsigned char>& key) {
        auto validKey = normalizeKey(key);
        return cryptAES(data, validKey, true);
    }

    static std::vector<unsigned char> decryptAES(const std::vector<unsigned char>& data, const std::vector<unsigned char>& key) {
        auto validKey = normalizeKey(key);

        auto decrypted = cryptAES(data, validKey, false);
        auto unpaddedData = removePKCS7Padding(decrypted);

        return unpaddedData;
    }

    // Cryptage AES avec std::string
    static std::string encryptAES(const std::string& data, const std::string& key) {
        auto validKey = normalizeKey(std::vector<unsigned char>(key.begin(), key.end()));
        auto encrypted = cryptAES(std::vector<unsigned char>(data.begin(), data.end()), validKey, true);
        return base64Encode(encrypted);
    }

    // Décryptage AES avec std::string
    static std::string decryptAES(const std::string& data, const std::string& key) {
        auto validKey = normalizeKey(std::vector<unsigned char>(key.begin(), key.end()));
        auto decoded = base64Decode(data);
        auto decrypted = cryptAES(decoded, validKey, false);
        auto unpaddedData = removePKCS7Padding(decrypted);
        return std::string(unpaddedData.begin(), unpaddedData.end());
    }


    // Encode en base64
    static std::string base64Encode(const std::vector<unsigned char>& data) {
        const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded;
        int val = 0, valb = -6;

        for (unsigned char c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                encoded.push_back(base64Chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }

        if (valb > -6) encoded.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (encoded.size() % 4) encoded.push_back('=');

        return encoded;
    }

    // Surcharge : Encode un const char*
    static std::string base64Encode(const char* data) {
        if (!data) throw std::invalid_argument("Null pointer passed to base64Encode.");
        std::vector<unsigned char> vec(data, data + std::strlen(data));
        return base64Encode(vec);
    }

    // Surcharge : Encode un char*
    static std::string base64Encode(char* data) {
        if (!data) throw std::invalid_argument("Null pointer passed to base64Encode.");
        std::vector<unsigned char> vec(data, data + std::strlen(data));
        return base64Encode(vec);
    }

    // Surcharge : Encode un std::string
    static std::string base64Encode(const std::string& data) {
        std::vector<unsigned char> vec(data.begin(), data.end());
        return base64Encode(vec);
    }

    // Décode une chaîne base64
    static std::vector<unsigned char> base64Decode(const std::string& encoded) {
        const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[base64Chars[i]] = i;

        std::vector<unsigned char> decoded;
        int val = 0, valb = -8;

        for (unsigned char c : encoded) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                decoded.push_back((val >> valb) & 0xFF);
                valb -= 8;
            }
        }

        return decoded;
    }


    // Calculer le checksum SHA256 d'un fichier
    static std::string calculateFileChecksum(const std::string& filePath) {
        BCRYPT_ALG_HANDLE hAlgorithm = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        DWORD hashObjectSize = 0, hashSize = 0, cbData = 0;
        std::vector<unsigned char> hashObject, hashValue;

        // Ouvrir l'algorithme SHA256
        if (BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
            throw std::runtime_error("Failed to open SHA256 algorithm provider.");
        }

        try {
            // Obtenir la taille de l'objet de hachage
            if (BCryptGetProperty(hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get hash object size.");
            }

            // Obtenir la taille du hachage
            if (BCryptGetProperty(hAlgorithm, BCRYPT_HASH_LENGTH, (PUCHAR)&hashSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get hash size.");
            }

            hashObject.resize(hashObjectSize);
            hashValue.resize(hashSize);

            // Créer le hachage
            if (BCryptCreateHash(hAlgorithm, &hHash, hashObject.data(), hashObjectSize, nullptr, 0, 0) != 0) {
                throw std::runtime_error("Failed to create hash.");
            }

            // Lire le fichier par blocs
            constexpr size_t bufferSize = 1024 * 1024; // 1 Mo
            std::vector<unsigned char> buffer(bufferSize);
            std::ifstream file(filePath, std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("Failed to open the file: " + filePath);
            }

            while (file.good()) {
                file.read(reinterpret_cast<char*>(buffer.data()), bufferSize);
                std::streamsize bytesRead = file.gcount();

                if (bytesRead > 0) {
                    if (BCryptHashData(hHash, buffer.data(), static_cast<ULONG>(bytesRead), 0) != 0) {
                        throw std::runtime_error("Failed to hash file data.");
                    }
                }
            }

            // Finaliser le hachage
            if (BCryptFinishHash(hHash, hashValue.data(), hashSize, 0) != 0) {
                throw std::runtime_error("Failed to finalize hash.");
            }
        } catch (...) {
            if (hHash) BCryptDestroyHash(hHash);
            if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
            throw;
        }

        if (hHash) BCryptDestroyHash(hHash);
        if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);

        // Convertir le hachage en chaîne hexadécimale
        std::ostringstream oss;
        for (auto byte : hashValue) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }

        return oss.str();
    }
private:
    static std::vector<unsigned char> normalizeKey(const std::vector<unsigned char>& key) {
        // Si la clé est déjà de 32 octets, aucune transformation n'est nécessaire
        if (key.size() == 32) {
            return key;
        }

        // Transforme la clé en SHA-256 pour garantir une taille fixe de 32 octets
        std::string keyStr(key.begin(), key.end());
        auto hashedKey = generateHashSHA256(keyStr);
        return hashedKey;
    }

    // Méthode générique pour générer un hachage
    static std::vector<unsigned char> generateHash(const std::string& input, LPCWSTR algorithm) {
        BCRYPT_ALG_HANDLE hAlgorithm = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        DWORD hashObjectSize = 0, hashSize = 0, cbData = 0;
        std::vector<unsigned char> hashObject, hashValue;

        if (BCryptOpenAlgorithmProvider(&hAlgorithm, algorithm, nullptr, 0) != 0) {
            throw std::runtime_error("Failed to open algorithm provider.");
        }

        try {
            if (BCryptGetProperty(hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get hash object size.");
            }

            if (BCryptGetProperty(hAlgorithm, BCRYPT_HASH_LENGTH, (PUCHAR)&hashSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get hash size.");
            }

            hashObject.resize(hashObjectSize);
            hashValue.resize(hashSize);

            if (BCryptCreateHash(hAlgorithm, &hHash, hashObject.data(), hashObjectSize, nullptr, 0, 0) != 0) {
                throw std::runtime_error("Failed to create hash.");
            }

            if (BCryptHashData(hHash, (PUCHAR)input.data(), input.size(), 0) != 0) {
                throw std::runtime_error("Failed to hash data.");
            }

            if (BCryptFinishHash(hHash, hashValue.data(), hashSize, 0) != 0) {
                throw std::runtime_error("Failed to finish hash.");
            }
        } catch (...) {
            if (hHash) BCryptDestroyHash(hHash);
            if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
            throw;
        }

        if (hHash) BCryptDestroyHash(hHash);
        if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);

        return hashValue;
    }

    // Méthode générique pour générer un HMAC
    static std::vector<unsigned char> generateKeyedHash(const std::string& data, const std::string& key, LPCWSTR algorithm) {
        BCRYPT_ALG_HANDLE hAlgorithm = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        DWORD hashObjectSize = 0, hashSize = 0, cbData = 0;
        std::vector<unsigned char> hashObject, hashValue;

        if (BCryptOpenAlgorithmProvider(&hAlgorithm, algorithm, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) {
            throw std::runtime_error("Failed to open HMAC algorithm provider.");
        }

        try {
            if (BCryptGetProperty(hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get HMAC object size.");
            }

            if (BCryptGetProperty(hAlgorithm, BCRYPT_HASH_LENGTH, (PUCHAR)&hashSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get HMAC hash size.");
            }

            hashObject.resize(hashObjectSize);
            hashValue.resize(hashSize);

            if (BCryptCreateHash(hAlgorithm, &hHash, hashObject.data(), hashObjectSize, (PUCHAR)key.data(), key.size(), 0) != 0) {
                throw std::runtime_error("Failed to create HMAC hash.");
            }

            if (BCryptHashData(hHash, (PUCHAR)data.data(), data.size(), 0) != 0) {
                throw std::runtime_error("Failed to hash data.");
            }

            if (BCryptFinishHash(hHash, hashValue.data(), hashSize, 0) != 0) {
                throw std::runtime_error("Failed to finish HMAC hash.");
            }
        } catch (...) {
            if (hHash) BCryptDestroyHash(hHash);
            if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
            throw;
        }

        if (hHash) BCryptDestroyHash(hHash);
        if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);

        return hashValue;
    }

    // Méthode générique pour le cryptage/décryptage AES
    static std::vector<unsigned char> cryptAES(const std::vector<unsigned char>& data, const std::vector<unsigned char>& key, bool encrypt) {
        BCRYPT_ALG_HANDLE hAlgorithm = nullptr;
        BCRYPT_KEY_HANDLE hKey = nullptr;
        DWORD keyObjectSize = 0, blockSize = 0, cbData = 0;
        std::vector<unsigned char> keyObject, output;

        try {

            // Ouvrir l'algorithme AES
            if (BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_AES_ALGORITHM, nullptr, 0) != 0) {
                throw std::runtime_error("Failed to open AES algorithm provider.");
            }

            // Définir le mode de chiffrement sur ECB
            if (BCryptSetProperty(hAlgorithm, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0) != 0) {
                throw std::runtime_error("Failed to set AES chaining mode to ECB.");
            }

            // Obtenir la taille de l'objet clé
            if (BCryptGetProperty(hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&keyObjectSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get AES key object size.");
            }

            // Obtenir la taille des blocs AES
            if (BCryptGetProperty(hAlgorithm, BCRYPT_BLOCK_LENGTH, (PUCHAR)&blockSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get AES block size.");
            }

            // Vérifier la taille de la clé
            if (key.size() != 16 && key.size() != 24 && key.size() != 32) {
                throw std::invalid_argument("Invalid AES key size. Key must be 16, 24, or 32 bytes.");
            }

            keyObject.resize(keyObjectSize);

            // Ajouter du padding aux données
            std::vector<unsigned char> paddedData = data;
            if (paddedData.size() % blockSize != 0) {
                size_t paddingSize = blockSize - (paddedData.size() % blockSize);
                paddedData.insert(paddedData.end(), paddingSize, static_cast<unsigned char>(paddingSize)); // PKCS7 padding
            }

            output.resize(paddedData.size() + blockSize);

            // Générer la clé AES
            if (BCryptGenerateSymmetricKey(hAlgorithm, &hKey, keyObject.data(), keyObjectSize, (PUCHAR)key.data(), key.size(), 0) != 0) {
                throw std::runtime_error("Failed to generate AES key.");
            }

            if (encrypt) {
                // Crypter les données
                if (BCryptEncrypt(hKey, (PUCHAR)paddedData.data(), paddedData.size(), nullptr, nullptr, 0, output.data(), output.size(), &cbData, 0) != 0) {
                    throw std::runtime_error("Failed to encrypt data.");
                }
            } else {
                // Décrypter les données
                if (BCryptDecrypt(hKey, (PUCHAR)data.data(), data.size(), nullptr, nullptr, 0, output.data(), output.size(), &cbData, 0) != 0) {
                    throw std::runtime_error("Failed to decrypt data.");
                }
            }

            output.resize(cbData);
        } catch (const std::exception& e) {
            std::cerr << "Exception caught: " << e.what() << std::endl;
            if (hKey) BCryptDestroyKey(hKey);
            if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
            throw;
        }

        if (hKey) BCryptDestroyKey(hKey);
        if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);

        return output;
    }

    // Méthode interne pour générer un hachage et le convertir en string
    static std::string hashToString(const std::string& input, LPCWSTR algorithm) {
        BCRYPT_ALG_HANDLE hAlgorithm = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        DWORD hashObjectSize = 0, hashSize = 0, cbData = 0;
        std::vector<unsigned char> hashObject, hashValue;

        // Ouvrir l'algorithme
        if (BCryptOpenAlgorithmProvider(&hAlgorithm, algorithm, nullptr, 0) != 0) {
            throw std::runtime_error("Failed to open algorithm provider.");
        }

        try {
            // Obtenir la taille de l'objet de hachage
            if (BCryptGetProperty(hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get hash object size.");
            }

            // Obtenir la taille du hachage
            if (BCryptGetProperty(hAlgorithm, BCRYPT_HASH_LENGTH, (PUCHAR)&hashSize, sizeof(DWORD), &cbData, 0) != 0) {
                throw std::runtime_error("Failed to get hash size.");
            }

            hashObject.resize(hashObjectSize);
            hashValue.resize(hashSize);

            // Créer un hachage
            if (BCryptCreateHash(hAlgorithm, &hHash, hashObject.data(), hashObjectSize, nullptr, 0, 0) != 0) {
                throw std::runtime_error("Failed to create hash.");
            }

            // Ajouter des données au hachage
            if (BCryptHashData(hHash, (PUCHAR)input.data(), input.size(), 0) != 0) {
                throw std::runtime_error("Failed to hash data.");
            }

            // Finaliser le hachage
            if (BCryptFinishHash(hHash, hashValue.data(), hashSize, 0) != 0) {
                throw std::runtime_error("Failed to finish hash.");
            }

        } catch (...) {
            if (hHash) BCryptDestroyHash(hHash);
            if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
            throw;
        }

        if (hHash) BCryptDestroyHash(hHash);
        if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);

        // Convertir le résultat en string hexadécimal
        std::ostringstream oss;
        for (auto byte : hashValue) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }

        return oss.str();
    }


    static std::vector<unsigned char> removePKCS7Padding(const std::vector<unsigned char>& data) {
        if (data.empty() || (data.size() != 16 && data.size() != 24 && data.size() != 32)) {
            throw std::invalid_argument("Invalid AES key size. Key must be 16, 24, or 32 bytes.");
        }

        // Obtenir la valeur du dernier octet (taille du padding)
        unsigned char paddingSize = data.back();

        // Vérifier que les derniers octets correspondent à la valeur du padding
        for (size_t i = data.size() - paddingSize; i < data.size(); ++i) {
            if (data[i] != paddingSize) {
                return std::vector<unsigned char>(data.begin(), data.end());
            }
        }

        // Retourner les données sans le padding
        return std::vector<unsigned char>(data.begin(), data.end() - paddingSize);
    }

};
