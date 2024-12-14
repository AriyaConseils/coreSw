#ifndef SWFILEINFO_H
#define SWFILEINFO_H
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

#include <string>
#include <windows.h>
#include <stdexcept>

class SwFileInfo {
public:
    explicit SwFileInfo(const std::string& filePath = "") : m_filePath(filePath) {
        normalizePath();
    }

    ~SwFileInfo() = default;

    bool exists() const {
        DWORD attributes = GetFileAttributesA(m_filePath.c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES);
    }

    bool isFile() const {
        DWORD attributes = GetFileAttributesA(m_filePath.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) return false;
        return !(attributes & FILE_ATTRIBUTE_DIRECTORY); // Un fichier n'est pas un répertoire
    }

    bool isDir() const {
        DWORD attributes = GetFileAttributesA(m_filePath.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) return false;
        return (attributes & FILE_ATTRIBUTE_DIRECTORY); // Un répertoire a cet attribut
    }

    std::string fileName() const {
        auto pos = m_filePath.find_last_of("/\\");
        return (pos == std::string::npos) ? m_filePath : m_filePath.substr(pos + 1);
    }

    std::string baseName() const {
        auto name = fileName();
        auto pos = name.find_last_of('.');
        return (pos == std::string::npos) ? name : name.substr(0, pos);
    }

    std::string suffix() const {
        auto name = fileName();
        auto pos = name.find_last_of('.');
        return (pos == std::string::npos) ? "" : name.substr(pos + 1);
    }

    std::string absoluteFilePath() const {
        char buffer[MAX_PATH];
        if (_fullpath(buffer, m_filePath.c_str(), MAX_PATH)) {
            return std::string(buffer);
        }
        throw std::runtime_error("Failed to resolve absolute file path.");
    }

    size_t size() const {
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (!GetFileAttributesExA(m_filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
            return 0;
        }
        LARGE_INTEGER fileSize;
        fileSize.LowPart = fileInfo.nFileSizeLow;
        fileSize.HighPart = fileInfo.nFileSizeHigh;
        return static_cast<size_t>(fileSize.QuadPart);
    }

private:
    std::string m_filePath;

    void normalizePath() {
        for (char& c : m_filePath) {
            if (c == '/') {
                c = '\\'; // Convertir les séparateurs Unix en Windows
            }
        }
    }
};

#endif // SWFILEINFO_H
