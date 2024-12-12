#ifndef SWSTANDARDLOCATION_H
#define SWSTANDARDLOCATION_H

#include <windows.h>
#include <shlobj.h>
#include "SwString.h"
#include "SwList.h"
#include <iostream>
#include "SwRegularExpression.h"


class SwStandardLocation {
public:

    enum class PathType {
        Windows,         // Chemin Windows classique (ex. "C:\\exemple\\fichier.txt")
        WindowsLong,     // Chemin Windows long (ex. "\\\\?\\C:\\exemple\\fichier.txt")
        Unix,            // Chemin Unix (ex. "/c/exemple/fichier.txt")
        Mixed,           // Chemin mixte (ex. "C:/exemple/fichier.txt")
        Undefined
    };


    enum Location {
        Desktop,
        Documents,
        Downloads,
        Music,
        Pictures,
        Videos,
        Home,
        Temp,
        AppData,
        LocalAppData,
        RoamingAppData,
        Cache,
        Config,
        StartMenu,
        Startup,
        Recent,
        SendTo,
        Favorites,
        PublicDesktop,
        PublicDocuments,
        PublicDownloads,
        PublicPictures,
        PublicMusic,
        PublicVideos,
        ProgramFiles,
        ProgramFilesX86,
        ProgramFilesCommon,
        ProgramFilesCommonX86,
        System,
        SystemX86,
        Windows,
        AdminTools,
        CommonAdminTools,
        Network,
        Public,
        PublicLibraries,
        PublicRingtones,
        SavedGames,
        SavedPictures,
        SavedVideos,
        CameraRoll,
        Screenshots,
        Playlists,
        CommonStartup,
        CommonPrograms,
        CommonStartMenu,
        InternetCache,
        Cookies,
        History,
        ApplicationShortcuts
    };


    /**
     * @brief Retrieves the standard location path as a SwString for a given location type.
     *
     * @param type The location type as a value of the Location enum.
     * @return SwString The corresponding standard location path.
     */
    static SwString standardLocation(Location type) {
        if (type == Temp) {
            // Gestion spécifique pour Temp
            wchar_t tempPath[MAX_PATH];
            DWORD pathLen = GetTempPathW(MAX_PATH, tempPath);
            if (pathLen > 0 && pathLen < MAX_PATH) {
                return SwString::fromWCharArray(tempPath).replace("\\", "/");
            } else {
                std::cerr << "Failed to retrieve temporary path." << std::endl;
                return SwString(); // Chaîne vide en cas d'erreur
            }
        }

        // Pour les autres types, utilisation de getFolderId
        REFKNOWNFOLDERID folderId = getFolderId(type);

        wchar_t* path = nullptr;
        SwString result;

        if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, NULL, &path))) {
            result = SwString::fromWCharArray(path);
            CoTaskMemFree(path);
        } else {
            std::cerr << "Failed to retrieve standard location for type: " << type << std::endl;
        }

        return result.replace("\\", "/");
    }

    /**
     * @brief Retrieves a list of possible paths for a given standard location.
     *
     * @param type The location type as a value of the Location enum.
     * @return SwList<SwString> A list of possible paths for the specified location.
     */
    static SwList<SwString> standardLocations(Location type) {
        SwList<SwString> locations;
        SwString location = standardLocation(type);
        if (!location.isEmpty()) {
            locations.append(location.replace("\\", "/"));
        }
        return locations;
    }

    /**
     * @brief Converts a file path to a specified format.
     *
     * @param path The input path as a SwString.
     * @param type The desired path format as a value of the PathType enum.
     * @return SwString The converted path.
     */
    static SwString convertPath(const SwString &path, PathType type) {
        if (path.isEmpty()) {
            std::cerr << "Error: Path is empty!" << std::endl;
            return SwString();
        }

        // Nettoyage initial : suppression des espaces en début et fin
        SwString cleanedPath = path.trimmed();

        // Vérification si le chemin est absolu
        SwRegularExpression absolutePattern(R"(^([a-zA-Z]:[\\/]|[\\/]{2}|/))"); // C:\, \\ ou /
        if (!absolutePattern.match(cleanedPath).hasMatch()) {
            std::cerr << "Warning: Path is relative, ensure proper handling!" << std::endl;
        }

         // Détection du type initial de chemin
         PathType initialType = PathType::Undefined;

         // Détection des chemins Windows longs
         if (cleanedPath.startsWith("\\\\?\\")) {
             initialType = PathType::WindowsLong;
             cleanedPath = cleanedPath.mid(4); // Retirer le préfixe "\\?\".
         }
         // Détection des chemins Windows standards
         else if (SwRegularExpression(R"(^[a-zA-Z]:)").match(cleanedPath).hasMatch()) {
             initialType = PathType::Windows;
         }
         // Détection des chemins Unix
         else if (cleanedPath.startsWith("/") || cleanedPath.startsWith("\\")) {
             initialType = PathType::Unix;
             char disk = SwString(cleanedPath[1]).toUpper().toStdString()[0];
             cleanedPath = cleanedPath.mid(3);
             cleanedPath.prepend("\\").prepend(":").prepend(disk);
         }

         // Application des transformations en fonction du type cible
         SwString result = cleanedPath;

        // Application des transformations en fonction du type
        switch (type) {
            case PathType::Windows: {
                // Remplacer tous les '/' par '\'
                SwRegularExpression unixToWindows(R"(/)");
                SwRegularExpressionMatch match;
                while ((match = unixToWindows.match(result)).hasMatch()) {
                    SwString toReplace = match.captured(); // Substring trouvée
                    result = result.replace(toReplace, "\\");
                }
                break;
            }
            case PathType::WindowsLong: {
                // Ajouter le préfixe '\\?\' si nécessaire
                SwRegularExpression longPathPattern(R"(^(?!\\\\\\?\\))");
                if (longPathPattern.match(result).hasMatch()) {
                    result = SwString("\\\\?\\") + result;
                }

                // Remplacer tous les '/' par '\'
                SwRegularExpression unixToWindows(R"(/)");
                SwRegularExpressionMatch match;
                while ((match = unixToWindows.match(result)).hasMatch()) {
                    SwString toReplace = match.captured(); // Substring trouvée
                    result = result.replace(toReplace, "\\");
                }
                break;
            }
            case PathType::Unix: {
                // Remplacer tous les '\' par '/'
                SwRegularExpression windowsToUnix(R"(\\)");
                SwRegularExpressionMatch match;
                while ((match = windowsToUnix.match(result)).hasMatch()) {
                    SwString toReplace = match.captured(); // Substring trouvée
                    result = result.replace(toReplace, "/");
                }

                // Transformer "C:" en "/c" pour les chemins Windows
                SwRegularExpression driveLetterPattern(R"(^([a-zA-Z]):)");
                SwRegularExpressionMatch driveMatch = driveLetterPattern.match(result);
                if (driveMatch.hasMatch()) {
                    result = SwString("/") + driveMatch.captured(1).toLower() + result.mid(driveMatch.capturedEnd());
                }
                break;
            }
            case PathType::Mixed: {
                // Remplacer tous les '\' par '/'
                SwRegularExpression windowsToUnix(R"(\\)");
                SwRegularExpressionMatch match;
                while ((match = windowsToUnix.match(result)).hasMatch()) {
                    SwString toReplace = match.captured(); // Substring trouvée
                    result = result.replace(toReplace, "/");
                }
                break;
            }
        }

        return result;
    }


private:
    // Mapping des types vers REFKNOWNFOLDERID
    static REFKNOWNFOLDERID getFolderId(Location type) {
        switch (type) {
            case Desktop: return FOLDERID_Desktop;
            case Documents: return FOLDERID_Documents;
            case Downloads: return FOLDERID_Downloads;
            case Music: return FOLDERID_Music;
            case Pictures: return FOLDERID_Pictures;
            case Videos: return FOLDERID_Videos;
            case Home: return FOLDERID_Profile;
            case AppData: return FOLDERID_RoamingAppData;
            case LocalAppData: return FOLDERID_LocalAppData;
            case RoamingAppData: return FOLDERID_RoamingAppData;
            case Cache: return FOLDERID_LocalAppData; // Pas de cache spécifique
            case Config: return FOLDERID_LocalAppData; // Pas de config spécifique
            case StartMenu: return FOLDERID_StartMenu;
            case Startup: return FOLDERID_Startup;
            case Recent: return FOLDERID_Recent;
            case SendTo: return FOLDERID_SendTo;
            case Favorites: return FOLDERID_Favorites;
            case PublicDesktop: return FOLDERID_PublicDesktop;
            case PublicDocuments: return FOLDERID_PublicDocuments;
            case PublicDownloads: return FOLDERID_PublicDownloads;
            case PublicPictures: return FOLDERID_PublicPictures;
            case PublicMusic: return FOLDERID_PublicMusic;
            case PublicVideos: return FOLDERID_PublicVideos;
            case ProgramFiles: return FOLDERID_ProgramFiles;
            case ProgramFilesX86: return FOLDERID_ProgramFilesX86;
            case ProgramFilesCommon: return FOLDERID_ProgramFilesCommon;
            case ProgramFilesCommonX86: return FOLDERID_ProgramFilesCommonX86;
            case System: return FOLDERID_System;
            case SystemX86: return FOLDERID_SystemX86;
            case Windows: return FOLDERID_Windows;
            case AdminTools: return FOLDERID_AdminTools;
            case CommonAdminTools: return FOLDERID_CommonAdminTools;
            case Network: return FOLDERID_NetHood;
            case Public: return FOLDERID_Public;
            case PublicLibraries: return FOLDERID_PublicLibraries;
            case PublicRingtones: return FOLDERID_PublicRingtones;
            case SavedGames: return FOLDERID_SavedGames;
            case SavedPictures: return FOLDERID_SavedPictures;
            case SavedVideos: return FOLDERID_VideosLibrary;
            case CameraRoll: return FOLDERID_CameraRoll;
            case Screenshots: return FOLDERID_Screenshots;
            case Playlists: return FOLDERID_Playlists;
            case CommonStartup: return FOLDERID_CommonStartup;
            case CommonPrograms: return FOLDERID_CommonPrograms;
            case CommonStartMenu: return FOLDERID_CommonStartMenu;
            case InternetCache: return FOLDERID_InternetCache;
            case Cookies: return FOLDERID_Cookies;
            case History: return FOLDERID_History;
            case ApplicationShortcuts: return FOLDERID_ApplicationShortcuts;
            default:
                std::cerr << "Error: Invalid Location type provided to getFolderId." << std::endl;
                return FOLDERID_Desktop; // Valeur par défaut
        }
    }


    // Conversion de wchar_t* en SwString
    static SwString wideCharToSwString(const wchar_t* wideStr) {
        if (!wideStr) {
            return SwString();
        }
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
        if (bufferSize <= 0) {
            return SwString();
        }
        std::string utf8Str(bufferSize - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &utf8Str[0], bufferSize, nullptr, nullptr);
        return SwString(utf8Str);
    }
};

#endif // SWSTANDARDLOCATION_H
