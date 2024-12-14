#ifndef SWDIR_H
#define SWDIR_H
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
#include <vector>
#include <stdexcept>
#include <windows.h>
#include <iostream>
#include "Sw.h"
#include "SwStandardLocation.h"
#include "SwString.h"
#include "SwFile.h"
#include <regex>


class SwDir {
public:
    explicit SwDir(const std::string& path = ".") {
        setPath(path);
    }

    ~SwDir() = default;

    bool exists() const {
        return exists(m_path);
    }

    SwString path() const {
        return m_path;
    }

    static bool exists(const SwString& path) {
        if (path.isEmpty()) {
            std::cerr << "Error: Path is empty!" << std::endl;
            return false;
        }

        // Normaliser le chemin
        SwString normalizedPath = normalizePath(path);

        // V?rifier les attributs du chemin
        DWORD attributes = GetFileAttributesW(normalizedPath.toStdWString().c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
    }


    static SwString normalizePath(const SwString& path) {
        return SwStandardLocation::convertPath(path, SwStandardLocation::PathType::WindowsLong);
    }

    bool setPath(const SwString& path) {
        if (exists(path)) {
            m_path = path;
            if (!m_path.endsWith('\\')) {
                m_path += '\\';
            }
            return true;
        }
        std::cerr << "Error set path failed!" << std::endl;
        return false;
    }


    SwStringList entryList(EntryTypes flags) const {
        SwStringList entries;

        // Construire le chemin de recherche
        SwString searchPath = m_path + "*";
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPath.toStdWString().c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open directory:" << m_path << std::endl;
        }

        do {
            SwString name = SwString::fromWString(findData.cFileName);

            // Ignorer "." et ".."
            if (name == "." || name == "..") {
                continue;
            }

            // D?terminer si c'est un r?pertoire
            bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            // Filtrer selon les flags
            if ((isDir && flags.testFlag(EntryType::Directories)) ||
                (!isDir && flags.testFlag(EntryType::Files))) {
                entries.append(name);
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
        return entries;
    }

    SwStringList entryList(const SwStringList& filters, EntryTypes flags = EntryType::AllEntries) const {
        SwStringList entries;

        // Construire le chemin de recherche
        SwString searchPath = m_path + "*";
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPath.toStdWString().c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open directory: " << m_path << std::endl;
        }

        do {
            SwString name = SwString::fromWString(findData.cFileName);

            // Ignorer "." et ".."
            if (name == "." || name == "..") {
                continue;
            }

            // Déterminer si c'est un répertoire
            bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            // Filtrer selon les flags
            if ((isDir && flags.testFlag(EntryType::Directories)) ||
                (!isDir && flags.testFlag(EntryType::Files))) {

                // Appliquer les filtres si présents
                if (!filters.isEmpty()) {
                    bool matchesFilter = false;
                    for (const auto& filter : filters) {
                        if (matchesPattern(name, filter)) {
                            matchesFilter = true;
                            break;
                        }
                    }
                    if (!matchesFilter) {
                        continue; // Ignorer les fichiers/répertoires qui ne correspondent à aucun filtre
                    }
                }

                entries.append(name);
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
        return entries;
    }

    SwString absolutePath() const {
        if (m_path.isEmpty()) {
            std::cerr << "Path is empty, cannot resolve absolute path." << std::endl;
        }

        // Buffer pour stocker le chemin absolu
        wchar_t absolutePathBuffer[MAX_PATH];

        // Utiliser la fonction Windows GetFullPathName pour résoudre le chemin absolu
        DWORD result = GetFullPathNameW(m_path.toStdWString().c_str(), MAX_PATH, absolutePathBuffer, nullptr);

        if (result == 0 || result > MAX_PATH) {
            std::cerr << "Failed to resolve absolute path for: "<< m_path << std::endl;
        }

        return SwString::fromWCharArray(absolutePathBuffer);
    }


    bool cd(const SwString& directoryName) {
        SwString newPath = m_path + directoryName + SwString("\\");
        DWORD attributes = GetFileAttributesW(newPath.toStdWString().c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            m_path = newPath;
            return true;
        }
        return false;
    }

    static SwString currentPath() {
        char buffer[MAX_PATH];
        DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);
        if (length > 0 && length < MAX_PATH) {
            return SwString(buffer) + SwString("\\");
        }
        std::cerr << "Failed to get current directory" << std::endl;

    }

    SwString absoluteFilePath(const SwString& relativePath) const {
        if (relativePath.isEmpty()) {
            std::cerr << "Relative path cannot be empty." << std::endl;
        }

        // V?rifier si le chemin est d?j? absolu
        if (relativePath.size() > 2 && (relativePath[1] == ':' || relativePath.left(2) == "\\\\")) {
            return relativePath;
        }


        // Ajouter le chemin relatif au chemin courant
        SwString absPath = normalizePath(m_path + relativePath);

        // Retourner en string
        return absPath;
    }

    static bool mkdir(const SwString& path) {
        // Normaliser le chemin
        SwString normalizedPath = normalizePath(path);

        // Vérifier si le répertoire existe déjà
        DWORD attributes = GetFileAttributesW(normalizedPath.toStdWString().c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return true; // Répertoire déjà existant
        }

        // Parcourir chaque composante du chemin
        int pos = 0;
        SwString currentPath;

        while ((pos = normalizedPath.indexOf('\\', pos + 1)) != -1) {
            currentPath = normalizedPath.left(pos);

            // Ignorer les cas où le chemin est \\?\, la racine (\), ou une lettre de disque suivie de :
            if (currentPath.isEmpty() || currentPath == "\\\\?" || currentPath == "\\" || currentPath.endsWith(':')) {
                continue;
            }

            SwDir dir(currentPath);

            if (dir.exists()) {
                continue;
            }

            // Créer le répertoire s'il n'existe pas
            if (CreateDirectoryW(currentPath.toStdWString().c_str(), nullptr) == 0) {
                DWORD error = GetLastError();
                if (error != ERROR_ALREADY_EXISTS) {
                    std::cerr << "Failed to create directory: " << currentPath.toStdString()
                               << " (Error Code: " << error << ")" << std::endl;
                    return false;
                }
            }
        }

        // Créer le répertoire final (si ce n'était pas un sous-répertoire couvert)
        if (CreateDirectoryW(normalizedPath.toStdWString().c_str(), nullptr) == 0) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                std::wcerr << L"Failed to create final directory: " << normalizedPath.toStdWString()
                           << L" (Error Code: " << error << L")" << std::endl;
                return false;
            }
        }

        return true;
    }


    static bool removeRecursively(const SwString& path) {
        // Créer une instance locale de SwDir pour gérer le chemin
        SwDir dir(path);

        if (!dir.exists()) {
            std::cerr << "Directory does not exist: " << path << std::endl;
            return false;
        }

        // Utiliser la méthode d'instance pour supprimer récursivement
        return dir.removeRecursively();
    }


    bool removeRecursively() {
        WIN32_FIND_DATAW findData;
        HANDLE hFind;

        // Construire le chemin de recherche
        SwString searchPath = normalizePath(m_path) + SwString("*");
        hFind = FindFirstFileW(searchPath.toStdWString().c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open directory: " << m_path << std::endl;
            return false;
        }

        do {
            SwString name = SwString::fromWString(findData.cFileName);

            // Ignorer "." et ".."
            if (name == "." || name == "..") {
                continue;
            }

            // Construire le chemin absolu
            SwString fullPath = normalizePath(m_path) + name;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Si c'est un répertoire, le supprimer récursivement
                SwDir subDir(fullPath + "\\");
                if (!subDir.removeRecursively()) {
                    FindClose(hFind);
                    return false;
                }
            } else {
                // Si c'est un fichier, le supprimer
                if (!DeleteFileW(fullPath.toStdWString().c_str())) {
                    std::cerr << "Failed to delete file: " << fullPath << std::endl;
                    FindClose(hFind);
                    return false;
                }
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);

        // Supprimer le répertoire lui-même
        if (!RemoveDirectoryW(normalizePath(m_path).toStdWString().c_str())) {
            std::cerr << "Failed to remove directory: " << normalizePath(m_path) << std::endl;
            return false;
        }

        return true;
    }


    static bool copy(const SwString& sourcePath, const SwString& destinationPath) {
        try {
            // Normaliser les chemins
            SwString sourceSearchPath = normalizePath(sourcePath) + "\\*";
            SwString destinationDir = normalizePath(destinationPath);

            WIN32_FIND_DATAW findData;
            HANDLE hFind = FindFirstFileW(sourceSearchPath.toStdWString().c_str(), &findData);
            if (hFind == INVALID_HANDLE_VALUE) {
                std::cerr << "Failed to open directory: " << sourceSearchPath << std::endl;
                return false;
            }

            // Cr?er le r?pertoire de destination s'il n'existe pas
            if (!mkdir(destinationDir)) {
                std::cerr << "Failed to create destination directory: " << destinationDir << std::endl;
                FindClose(hFind);
                return false;
            }


            do {
                SwString entryName = SwString::fromWString(findData.cFileName);

                // Ignorer "." et ".."
                if (entryName == "." || entryName == "..") {
                    continue;
                }

                SwString sourceEntry = normalizePath(sourcePath) + SwString("\\") + entryName;
                SwString destinationEntry = normalizePath(destinationPath) + SwString("\\") + entryName;

                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // Cr?er le r?pertoire de destination s'il n'existe pas
                    if (!mkdir(destinationEntry)) {
                        std::cerr << "Failed to create destination directory: " << destinationDir << std::endl;
                        FindClose(hFind);
                        return false;
                    }
                    // Copier le sous-r?pertoire r?cursivement
                    if (!copy(sourceEntry.begin(),destinationEntry)) {
                        FindClose(hFind);
                        return false;
                    }
                }
                else {
                    // Copier le fichier
                    if (!SwFile::copy(sourceEntry, destinationEntry)) {
                        std::cerr << "Failed to copy file: " << sourceEntry << " to " << destinationEntry << std::endl;
                        FindClose(hFind);
                        return false;
                    }
                }
            } while (FindNextFileW(hFind, &findData));

            FindClose(hFind);
            return true;

        }
        catch (const std::exception& e) {
            std::cerr << "Error during copy operation: " << e.what() << std::endl;
            return false;
        }
    }

    SwStringList findFiles(const SwString& filter) const {
        SwStringList foundFiles;
        SwString searchPath = normalizePath(m_path + "*");

        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPath.toStdWString().c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open directory: " << m_path << std::endl;
        }

        do {
            SwString name = SwString::fromWString(findData.cFileName);

            // Ignorer "." et ".."
            if (name == "." || name == "..") {
                continue;
            }

            // Construire le chemin absolu
            SwString absolutePath = m_path + name;

            // Vérifier si c'est un répertoire
            bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            if (isDir) {
                // Parcours récursif des sous-répertoires
                SwDir subDir(absolutePath);
                SwStringList subDirFiles = subDir.findFiles(filter);
                foundFiles.append(subDirFiles);
            } else {
                // Vérifier si le fichier correspond au filtre
                if (name.endsWith(filter)) {
                    absolutePath = SwStandardLocation::convertPath(absolutePath, SwStandardLocation::PathType::Mixed);
                    foundFiles.append(absolutePath);
                }
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
        return foundFiles;
    }


    SwString dirName() const {
        if (m_path.isEmpty()) {
            std::cerr << "Path is empty, cannot retrieve directory name." << std::endl;
            return SwString();
        }
        SwString path = m_path;
        path.replace("\\", "/");
        return path.split("/").last();
    }

private:
    SwString m_path;


    bool matchesPattern(const std::string& name, const std::string& pattern) const {
        // Convertir les jokers "*" en une expression régulière
        std::string regexPattern = "^" + std::regex_replace(pattern, std::regex(R"(\*)"), ".*") + "$";
        std::regex regex(regexPattern, std::regex_constants::icase); // Ignorer la casse
        return std::regex_match(name, regex);
    }

};

#endif // SWDIR_H
