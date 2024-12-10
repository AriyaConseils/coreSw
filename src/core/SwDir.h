#ifndef SWDIR_H
#define SWDIR_H

#include <string>
#include <vector>
#include <stdexcept>
#include <windows.h>
#include <iostream>
#include "Sw.h"

class SwDir {
public:
    explicit SwDir(const std::string& path = ".") {
        setPath(path);
    }

    ~SwDir() = default;

    bool exists() const {
        DWORD attributes = GetFileAttributesA(m_path.c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    std::string path() const {
        return m_path;
    }

    static bool exists(const std::string& path) {
        if (path.empty()) {
            std::cerr << "Error: Path is empty!" << std::endl;
            return false;
        }

        // Normaliser le chemin
        std::wstring normalizedPath = normalizePath(path);

        // V�rifier les attributs du chemin
        DWORD attributes = GetFileAttributesW(normalizedPath.c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES);
    }


    static std::wstring normalizePath(const std::string& path) {
        if (path.empty()) {
            throw std::invalid_argument("Path cannot be empty.");
        }

        // Convertir le chemin en Unicode (UTF-16)
        std::wstring wPath(path.begin(), path.end());

        // Normaliser les s�parateurs en antislashs
        for (wchar_t& c : wPath) {
            if (c == '/') {
                c = '\\';
            }
        }

        // Nettoyer les antislashs multiples (\\) � un seul (\)
        std::wstring result;
        bool lastWasBackslash = false;
        for (size_t i = 0; i < wPath.size(); ++i) {
            // Pr�server le pr�fixe \\?\ intact
            if (i < 4 && wPath.substr(0, 4) == L"\\\\?\\") {
                result += wPath[i];
                continue;
            }

            // R�duire les antislashs multiples (\\) � un seul (\) apr�s le pr�fixe
            if (wPath[i] == '\\') {
                if (lastWasBackslash) {
                    continue; // Ignorer les antislashs cons�cutifs
                }
                lastWasBackslash = true;
            }
            else {
                lastWasBackslash = false;
            }
            result += wPath[i];
        }


        // V�rifier et �viter les pr�fixes \\?\ doubl�s
        if (!result.empty() && result.substr(0, 8) == L"\\\\?\\?\\") {
            result = result.substr(4); // Supprimer le pr�fixe doubl�
        }

        // Ajouter le pr�fixe \\?\ pour les chemins longs ou sp�ciaux
        if (!result.empty() && result.substr(0, 4) != L"\\\\?\\") {
            result = L"\\\\?\\" + result;
        }

        return result;
    }






    bool setPath(const std::string& path) {
        // V�rifiez que le chemin n'est pas vide
        if (path.empty()) {
            std::cerr << "Error: Path is empty!" << std::endl;
            return false;
        }

        std::wstring wPath = normalizePath(path);

        // V�rifiez si le chemin existe et est un r�pertoire
        DWORD attributes = GetFileAttributesW(wPath.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            DWORD error = GetLastError();
            std::wcerr << L"Error: " << error << L" when checking path: " << wPath << std::endl;

            if (error == ERROR_PATH_NOT_FOUND) {
                std::cerr << "Part of the path does not exist!" << std::endl;
            }
            else if (error == ERROR_INVALID_NAME) {
                std::cerr << "Invalid path name!" << std::endl;
            }
            else {
                std::cerr << "Unknown error!" << std::endl;
            }
            return false;
        }

        // V�rifiez que c'est un r�pertoire
        if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
            m_path = path; // Conservez le chemin original (non Unicode)
            if (m_path.back() != '\\') {
                m_path += '\\'; // Ajoutez un s�parateur final
            }
            return true;
        }

        std::cerr << "The path exists but is not a directory!" << std::endl;
        return false;
    }


    std::vector<std::string> entryList(EntryTypes flags) const {
        std::vector<std::string> entries;

        // Construire le chemin de recherche
        std::string searchPath = m_path + "*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open directory: " + m_path);
        }

        do {
            std::string name = findData.cFileName;

            // Ignorer "." et ".."
            if (name == "." || name == "..") {
                continue;
            }

            // D�terminer si c'est un r�pertoire
            bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            // Filtrer selon les flags
            if ((isDir && flags.testFlag(EntryType::Directories)) ||
                (!isDir && flags.testFlag(EntryType::Files))) {
                entries.push_back(name);
            }
        } while (FindNextFileA(hFind, &findData));

        FindClose(hFind);
        return entries;
    }


    bool cd(const std::string& directoryName) {
        std::string newPath = m_path + directoryName + "\\";
        DWORD attributes = GetFileAttributesA(newPath.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            m_path = newPath;
            return true;
        }
        return false;
    }

    static std::string currentPath() {
        char buffer[MAX_PATH];
        DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);
        if (length > 0 && length < MAX_PATH) {
            return std::string(buffer) + "\\";
        }
        throw std::runtime_error("Failed to get current directory");
    }

    std::string absoluteFilePath(const std::string& relativePath) const {
        if (relativePath.empty()) {
            throw std::invalid_argument("Relative path cannot be empty.");
        }

        // V�rifier si le chemin est d�j� absolu
        if (relativePath.size() > 2 && (relativePath[1] == ':' || relativePath.substr(0, 2) == "\\\\")) {
            return relativePath;
        }

        // Ajouter le chemin relatif au chemin courant
        std::string absPath = m_path + relativePath;

        // Normaliser le chemin pour �viter les erreurs
        std::wstring normalizedPath = normalizePath(absPath);

        // Retourner en string
        return std::string(normalizedPath.begin(), normalizedPath.end());
    }

    static bool mkdir(const std::string& path) {
        // Normaliser le chemin
        std::wstring normalizedPath = normalizePath(path);

        // V�rifier si le r�pertoire existe d�j�
        DWORD attributes = GetFileAttributesW(normalizedPath.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return true; // R�pertoire d�j� existant
        }

        // Parcourir chaque composante du chemin
        size_t pos = 0;
        std::wstring currentPath;
        while ((pos = normalizedPath.find(L'\\', pos + 1)) != std::wstring::npos) {
            currentPath = normalizedPath.substr(0, pos);

            // Ignorer les cas o� le chemin est \\?\, la racine (\), ou une lettre de disque suivie de :
            if (currentPath.empty() || currentPath == L"\\\\?" || currentPath == L"\\" || currentPath.back() == L':') {
                continue;
            }

            // V�rifier si le r�pertoire existe d�j�
            DWORD attributes = GetFileAttributesW(currentPath.c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                continue; // Le r�pertoire existe d�j�
            }

            // Cr�er le r�pertoire s'il n'existe pas
            if (CreateDirectoryW(currentPath.c_str(), nullptr) == 0) {
                DWORD error = GetLastError();
                if (error != ERROR_ALREADY_EXISTS) {
                    std::wcerr << L"Failed to create directory: " << currentPath
                        << L" (Error Code: " << error << L")" << std::endl;
                    return false;
                }
            }
        }


        // Cr�er le r�pertoire final
        if (CreateDirectoryW(normalizedPath.c_str(), nullptr) == 0) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                std::wcerr << L"Failed to create directory: " << normalizedPath
                    << L" (Error Code: " << error << L")" << std::endl;
                return false;
            }
        }

        return true;
    }

    static bool removeRecursively(const std::string& path) {
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
        std::wstring searchPath = normalizePath(m_path) + L"*";
        hFind = FindFirstFileW(searchPath.c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE) {
            std::wcerr << L"Failed to open directory: " << normalizePath(m_path) << std::endl;
            return false;
        }

        do {
            std::wstring name = findData.cFileName;

            // Ignorer "." et ".."
            if (name == L"." || name == L"..") {
                continue;
            }

            // Construire le chemin absolu
            std::wstring fullPath = normalizePath(m_path) + name;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Si c'est un répertoire, le supprimer récursivement
                std::string fullPathString(fullPath.begin(), fullPath.end());
                SwDir subDir(fullPathString + "\\");
                if (!subDir.removeRecursively()) {
                    FindClose(hFind);
                    return false;
                }
            } else {
                // Si c'est un fichier, le supprimer
                if (!DeleteFileW(fullPath.c_str())) {
                    std::wcerr << L"Failed to delete file: " << fullPath << std::endl;
                    FindClose(hFind);
                    return false;
                }
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);

        // Supprimer le répertoire lui-même
        if (!RemoveDirectoryW(normalizePath(m_path).c_str())) {
            std::wcerr << L"Failed to remove directory: " << normalizePath(m_path) << std::endl;
            return false;
        }

        return true;
    }


    static bool copy(const std::string& sourcePath, const std::string& destinationPath) {
        try {
            // Normaliser les chemins
            std::wstring sourceSearchPath = normalizePath(sourcePath) + L"\\*";
            std::wstring destinationDir = normalizePath(destinationPath);

            WIN32_FIND_DATAW findData;
            HANDLE hFind = FindFirstFileW(sourceSearchPath.c_str(), &findData);
            if (hFind == INVALID_HANDLE_VALUE) {
                std::wcerr << L"Failed to open directory: " << sourceSearchPath << std::endl;
                return false;
            }

            // Cr�er le r�pertoire de destination s'il n'existe pas
            if (CreateDirectoryW(destinationDir.c_str(), nullptr) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
                std::wcerr << L"Failed to create destination directory: " << destinationDir << std::endl;
                FindClose(hFind);
                return false;
            }

            do {
                std::wstring entryName = findData.cFileName;

                // Ignorer "." et ".."
                if (entryName == L"." || entryName == L"..") {
                    continue;
                }

                std::wstring sourceEntry = normalizePath(sourcePath) + L"\\" + entryName;
                std::wstring destinationEntry = normalizePath(destinationPath) + L"\\" + entryName;

                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // Cr�er le r�pertoire de destination s'il n'existe pas
                    if (CreateDirectoryW(destinationEntry.c_str(), nullptr) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
                        std::wcerr << L"Failed to create destination directory: " << destinationDir << std::endl;
                        FindClose(hFind);
                        return false;
                    }
                    // Copier le sous-r�pertoire r�cursivement
                    if (!copy(std::string(sourceEntry.begin(), sourceEntry.end()),
                        std::string(destinationEntry.begin(), destinationEntry.end()))) {
                        FindClose(hFind);
                        return false;
                    }
                }
                else {
                    // Copier le fichier
                    if (!CopyFileW(sourceEntry.c_str(), destinationEntry.c_str(), FALSE)) {
                        std::wcerr << L"Failed to copy file: " << sourceEntry << L" to " << destinationEntry << std::endl;
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

std::vector<std::string> findFiles(const std::string& filter) const {
    std::vector<std::string> foundFiles;
    std::string searchPath = m_path + "*";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open directory: " + m_path << std::endl;
    }

    do {
        std::string name = findData.cFileName;

        // Ignorer "." et ".."
        if (name == "." || name == "..") {
            continue;
        }

        // Construire le chemin absolu
        std::string absolutePath = m_path + name;

        // Vérifier si c'est un répertoire
        bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (isDir) {
            // Parcours récursif des sous-répertoires
            SwDir subDir(absolutePath);
            auto subDirFiles = subDir.findFiles(filter);
            foundFiles.insert(foundFiles.end(), subDirFiles.begin(), subDirFiles.end());
        } else {
            // Vérifier si le fichier correspond au filtre
            if (name.size() >= filter.size() &&
                name.compare(name.size() - filter.size(), filter.size(), filter) == 0) {
                foundFiles.push_back(absolutePath);
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
    return foundFiles;
}

private:
    std::string m_path;
};

#endif // SWDIR_H
