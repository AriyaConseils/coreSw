#pragma once

#include "IODevice.h"
#include "IODescriptor.h"
#include <windows.h>
#include <vector>
#include <string>
#include <iostream>

enum class ProcessFlags : DWORD {
    NoFlag = 0,              // Pas de flag spécifique
    CreateNoWindow = CREATE_NO_WINDOW, // Flag pour ne pas afficher de fenêtre
    CreateNewConsole = CREATE_NEW_CONSOLE, // Créer une nouvelle console
    Detached = DETACHED_PROCESS, // Lancer le processus de manière détachée
    Suspended = CREATE_SUSPENDED // Lancer le processus en état suspendu
};


class Process : public IODevice {
public:
    Process(Object* parent = nullptr)
        : IODevice(parent), processRunning(false), hProcess(NULL), hStdOutRead(NULL), hStdErrRead(NULL), hStdInWrite(NULL) {
        stdoutDescriptor = nullptr;
        stderrDescriptor = nullptr;
        stdinDescriptor = nullptr;
        monitorTimer = new Timer(500);
        connect(monitorTimer, "timeout", this, &Process::checkProcessStatus);
    }

    virtual ~Process() {
        if (isOpen()) {
            close();
        }
    }

    bool start(const std::string& program, const std::vector<std::string>& arguments = {}, ProcessFlags flags = ProcessFlags::NoFlag) {
        if (isOpen()) {
            std::cout << "Process already running!" << std::endl;
            return false;
        }

        if (!createPipes()) {
            std::cout << "Failed to create pipes!" << std::endl;
            return false;
        }

        if (!startProcess(program, arguments, flags)) {
            std::cout << "Failed to start process!" << std::endl;
            return false;
        }

        processRunning = true;
        emit deviceOpened();
        monitorTimer->start();
        m_timerDercriptor->start();
        return true;
    }

    void close() override {
        if (!isOpen()) {
            std::cout << "Process not running!" << std::endl;
            return;
        }

        processRunning = false;
        m_timerDercriptor->stop();

        // Fermer les pipes
        CloseHandle(hStdOutRead);
        CloseHandle(hStdErrRead);
        CloseHandle(hStdInWrite);

        // Terminer le processus
        TerminateProcess(hProcess, 0);
        WaitForSingleObject(hProcess, INFINITE);
        CloseHandle(hProcess);

        emit deviceClosed();
        emit processFinished();
        
        monitorTimer->stop();
    }

    bool isOpen() const override {
        return processRunning;
    }

    std::string read(int64_t maxSize = 0) override {
        if (!stdoutDescriptor) return "";
        return stdoutDescriptor->read();
    }

    std::string readStdErr() {
        if (!stderrDescriptor) return "";
        return stderrDescriptor->read();
    }

    bool write(const std::string& data) override {
        if (!stdinDescriptor) return false;
        return stdinDescriptor->write(data);
    }


public slots:
    void checkProcessStatus() {
        if (!processRunning) return;

        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
                std::cout << "Process terminated with exit code: " << exitCode << std::endl;
                emit processTerminated(exitCode);
                close();
            }
        }
        else {
            std::cerr << "Failed to get process exit code: " << GetLastError() << std::endl;
        }
    }

    void kill() {
        if (!isOpen()) {
            std::cout << "Process not running!" << std::endl;
            return;
        }

        // Forcer l'arrêt immédiat du processus
        if (!TerminateProcess(hProcess, 1)) {
            std::cerr << "Failed to kill process: " << GetLastError() << std::endl;
        }
        else {
            WaitForSingleObject(hProcess, INFINITE); 
            std::cout << "Process killed." << std::endl;
            close();  
        }
    }

    void terminate() {
        if (!isOpen()) {
            std::cout << "Process not running!" << std::endl;
            return;
        }

        DWORD exitCode = 0;
        GetExitCodeProcess(hProcess, &exitCode);
        if (exitCode == STILL_ACTIVE) {
            std::cout << "Attempting to close process..." << std::endl;

            // Ici, on pourrait envoyer un message WM_CLOSE pour des applications GUI, mais ce n'est pas universel.
            // Sinon, utiliser TerminateProcess si la fermeture propre échoue

            if (!TerminateProcess(hProcess, 0)) {
                std::cerr << "Failed to terminate process: " << GetLastError() << std::endl;
            }
            else {
                WaitForSingleObject(hProcess, INFINITE);
                std::cout << "Process terminated." << std::endl;
                close();
            }
        }
        else {
            std::cout << "Process already terminated with exit code: " << exitCode << std::endl;
        }
    }


signals:
    DECLARE_SIGNAL(deviceOpened)
    DECLARE_SIGNAL(deviceClosed)
    DECLARE_SIGNAL(processFinished)
    DECLARE_SIGNAL(processTerminated)

    

private:
    Timer* monitorTimer;
    bool processRunning;
    HANDLE hProcess;
    HANDLE hThread;
    HANDLE hStdOutRead;
    HANDLE hStdOutWrite;
    HANDLE hStdErrRead;
    HANDLE hStdErrWrite;
    HANDLE hStdInWrite;
    HANDLE hStdInRead;

    IODescriptor* stdoutDescriptor;
    IODescriptor* stderrDescriptor;
    IODescriptor* stdinDescriptor;

    bool createPipes() {
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        // Créer les pipes pour stdout
        if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) {
            std::cout << "Failed to create stdout pipe!" << std::endl;
            return false;
        }
        if (!SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0)) {
            std::cout << "Failed to set handle information for stdout!" << std::endl;
            return false;
        }

        // Créer les pipes pour stderr
        if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0)) {
            std::cout << "Failed to create stderr pipe!" << std::endl;
            return false;
        }
        if (!SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0)) {
            std::cout << "Failed to set handle information for stderr!" << std::endl;
            return false;
        }

        // Créer les pipes pour stdin
        if (!CreatePipe(&hStdInRead, &hStdInWrite, &saAttr, 0)) {
            std::cout << "Failed to create stdin pipe!" << std::endl;
            return false;
        }
        if (!SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0)) {
            std::cout << "Failed to set handle information for stdin!" << std::endl;
            return false;
        }

        stdoutDescriptor = new IODescriptor(hStdOutRead, "StdOut");
        stderrDescriptor = new IODescriptor(hStdErrRead, "StdErr");
        stdinDescriptor = new IODescriptor(hStdInWrite, "StdIn");
        addDescriptor(stdoutDescriptor);
        addDescriptor(stderrDescriptor);

        return true;
    }


    bool startProcess(const std::string& program, const std::vector<std::string>& arguments, ProcessFlags creationFlags = ProcessFlags::NoFlag) {
        std::string command = program;
        for (const auto& arg : arguments) {
            command += " " + arg;
        }

        std::wstring wideCommand = std::wstring(command.begin(), command.end());

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.hStdError = hStdErrWrite;
        si.hStdOutput = hStdOutWrite;
        si.hStdInput = hStdInRead;
        si.dwFlags |= STARTF_USESTDHANDLES;

        ZeroMemory(&pi, sizeof(pi));

        // Utiliser 'creationFlags' fourni par l'appelant
        if (!CreateProcessW(NULL, &wideCommand[0], NULL, NULL, TRUE, static_cast<DWORD>(creationFlags), NULL, NULL, &si, &pi)) {
            DWORD error = GetLastError(); // Récupérer l'erreur
            std::cerr << "CreateProcess failed with error code: " << error << std::endl;
            return false;
        }

        hProcess = pi.hProcess;
        hThread = pi.hThread;
        return true;
    }


 
};
