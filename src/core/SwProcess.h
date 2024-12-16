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

#include "SwIODevice.h"
#include "SwIODescriptor.h"
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

/**
 * @class SwProcess
 * @brief The SwProcess class provides an interface for managing system processes with I/O redirection.
 *
 * This class allows you to launch and manage external processes while providing access to their
 * standard input, output, and error streams. It inherits from SwIODevice to leverage I/O capabilities.
 *
 * Key Features:
 * - Launch processes and monitor their execution state.
 * - Redirect and interact with the process's standard I/O streams.
 * - Periodically monitor the process state using a timer.
 *
 * Example Usage:
 * @code
 * SwProcess* process = new SwProcess();
 * process->start("myExecutable", {"arg1", "arg2"});
 * process->write("Input data");
 * QString output = process->readAllStandardOutput();
 * delete process;
 * @endcode
 *
 * @see SwIODevice
 */
class SwProcess : public SwIODevice {
public:

    /**
     * @brief Constructor for the SwProcess class.
     *
     * Initializes a new SwProcess object with an optional parent.
     * - Sets the process and pipe handles to NULL.
     * - Initializes the I/O descriptors (stdout, stderr, stdin) to `nullptr`.
     * - Creates a timer to monitor the process state.
     * - Connects the timer to the `checkProcessStatus` method for periodic process monitoring.
     *
     * @param parent An optional parent object for hierarchical ownership.
     */
    SwProcess(SwObject* parent = nullptr)
        : SwIODevice(parent), processRunning(false), hProcess(NULL), hStdOutRead(NULL), hStdErrRead(NULL), hStdInWrite(NULL) {
        stdoutDescriptor = nullptr;
        stderrDescriptor = nullptr;
        stdinDescriptor = nullptr;
        monitorTimer = new SwTimer(500);
        connect(monitorTimer, SIGNAL(timeout), this, &SwProcess::checkProcessStatus);
    }

    /**
     * @brief Destructor for the SwProcess class.
     *
     * Ensures the proper cleanup of resources associated with the process.
     * - If the process is running, calls `close()` to terminate it and release associated resources.
     */
    virtual ~SwProcess() {
        if (isOpen()) {
            close();
        }
    }

    /**
     * @brief Starts a new process with the specified program and arguments.
     *
     * Initializes and launches a new process while setting up input/output pipes.
     * - Ensures no other process is currently running.
     * - Creates communication pipes for standard input, output, and error streams.
     * - Launches the process using the specified program, arguments, flags, and working directory.
     * - Starts monitoring the process state using a timer.
     *
     * @param program The path to the executable to start.
     * @param arguments A vector of arguments to pass to the executable (optional).
     * @param flags Flags to configure the process creation (default: ProcessFlags::NoFlag).
     * @param workingDirectory The working directory for the process (optional).
     *
     * @return `true` if the process started successfully, `false` otherwise.
     */
    bool start(const SwString& program, const SwStringList& arguments = {},
               ProcessFlags flags = ProcessFlags::NoFlag,
               const SwString& workingDirectory = "") {
        if (isOpen()) {
            std::cout << "Process already running!" << std::endl;
            return false;
        }

        if (!createPipes()) {
            std::cout << "Failed to create pipes!" << std::endl;
            return false;
        }

        if (!startProcess(program, arguments, flags, workingDirectory)) {
            std::cout << "Failed to start process!" << std::endl;
            return false;
        }

        processRunning = true;
        emit deviceOpened();
        monitorTimer->start();
        m_timerDercriptor->start();
        return true;
    }

    /**
     * @brief Starts the process using previously set program, arguments, and working directory.
     *
     * Uses the stored program path, arguments, and working directory to launch the process.
     * - Validates that the program path is set before starting.
     * - Delegates the actual process start to the overloaded `start` method.
     *
     * @param flags Flags to configure the process creation (default: ProcessFlags::NoFlag).
     *
     * @return `true` if the process started successfully, `false` otherwise.
     */
    bool start(ProcessFlags flags = ProcessFlags::NoFlag) {
        if (m_program.isEmpty()) {
            std::cerr << "Program is not set!" << std::endl;
            return false;
        }
        return start(m_program, m_arguments, flags, m_workingDirectory);
    }

    /**
     * @brief Sets the program path for the process.
     *
     * Specifies the executable file to be used when starting the process.
     *
     * @param program The path to the executable file.
     */
    void setProgram(const SwString& program) { m_program = program; }

    /**
     * @brief Retrieves the program path for the process.
     *
     * Returns the path to the executable file set for the process.
     *
     * @return A string containing the program path.
     */
    SwString program() const { return m_program; }

    /**
     * @brief Sets the arguments to be passed to the process.
     *
     * Specifies the command-line arguments that will be used when starting the process.
     *
     * @param arguments A vector of strings containing the arguments.
     */
    void setArguments(const SwStringList& arguments) { m_arguments = arguments; }

    /**
     * @brief Retrieves the arguments set for the process.
     *
     * Returns the command-line arguments that will be passed to the process when started.
     *
     * @return A vector of strings containing the arguments.
     */
    SwStringList arguments() const { return m_arguments; }

    /**
     * @brief Sets the working directory for the process.
     *
     * Specifies the directory in which the process will run.
     *
     * @param dir The path to the working directory.
     */
    void setWorkingDirectory(const SwString& dir) { m_workingDirectory = dir; }

    /**
     * @brief Retrieves the working directory set for the process.
     *
     * Returns the directory in which the process will execute.
     *
     * @return A string containing the path to the working directory.
     */
    SwString workingDirectory() const { return m_workingDirectory; }

    /**
     * @brief Closes the currently running process and releases associated resources.
     *
     * Ensures proper termination of the process and cleanup of allocated resources:
     * - Stops monitoring timers and descriptors.
     * - Removes standard input/output/error descriptors from SwIODevice.
     * - Terminates the process and waits for it to fully stop.
     * - Releases all process handles and emits relevant signals.
     *
     * @note If the process is not running, a message is displayed, and no action is taken.
     */
    void close() override {
        if (!isOpen()) {
            std::cout << "Process not running!" << std::endl;
            return;
        }

        processRunning = false;
        m_timerDercriptor->stop();



        // Remove descriptors from SwIODevice
        // This will delete the descriptors and automatically close the associated pipe handles
        removeDescriptor(stdoutDescriptor);
        removeDescriptor(stderrDescriptor);

        // Delete the stdin descriptor safely and set the pointer to nullptr
        safeDelete(stdinDescriptor);


        // Terminer le processus
        TerminateProcess(hProcess, 0);
        WaitForSingleObject(hProcess, INFINITE);
        CloseHandle(hProcess);


        emit deviceClosed();
        emit processFinished();
        
        monitorTimer->stop();
    }

    /**
     * @brief Checks if the process is currently running.
     *
     * Determines whether the process is open and actively running.
     *
     * @return `true` if the process is running, `false` otherwise.
     */
    bool isOpen() const override {
        return processRunning;
    }

    /**
     * @brief Reads data from the standard output of the process.
     *
     * Reads up to the specified maximum size of data from the process's standard output.
     *
     * @param maxSize The maximum number of bytes to read (default: 0, which means no limit).
     *
     * @return A string containing the data read from the process's standard output.
     *         Returns an empty string if the standard output descriptor is unavailable.
     */
    SwString read(int64_t maxSize = 0) {
        if (!stdoutDescriptor) return "";
        return stdoutDescriptor->read();
    }

    /**
     * @brief Reads data from the standard error stream of the process.
     *
     * Retrieves the output written to the process's standard error stream.
     *
     * @return A string containing the data read from the process's standard error stream.
     *         Returns an empty string if the standard error descriptor is unavailable.
     */
    std::string readStdErr() {
        if (!stderrDescriptor) return "";
        return stderrDescriptor->read();
    }

    /**
     * @brief Writes data to the standard input of the process.
     *
     * Sends the specified string to the process's standard input stream.
     *
     * @param data The string to write to the standard input.
     *
     * @return `true` if the data was successfully written, `false` if the standard input descriptor is unavailable.
     */
    bool write(const SwString& data) {
        if (!stdinDescriptor) return false;
        return stdinDescriptor->write(data);
    }


public slots:

    /**
     * @brief Slot to forcibly terminate the running process.
     *
     * Immediately stops the process by calling `TerminateProcess`.
     * - Waits for the process to fully terminate using `WaitForSingleObject`.
     * - Emits cleanup signals and calls `close()` to release resources.
     * - Logs an error message if the termination fails.
     *
     * @note This method should be used for immediate termination without waiting for the process to finish naturally.
     */
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

    /**
     * @brief Slot to gracefully terminate the running process.
     *
     * Attempts to stop the process by checking its status and sending termination signals:
     * - If the process is still active, tries to terminate it gracefully (e.g., using `WM_CLOSE` for GUI applications).
     * - If graceful termination fails, calls `TerminateProcess` as a fallback.
     * - Waits for the process to fully terminate using `WaitForSingleObject`.
     * - Emits cleanup signals and calls `close()` to release resources.
     * - Logs the exit code if the process is already terminated.
     *
     * @note This method prioritizes graceful termination before resorting to forced termination.
     */
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
    SwTimer* monitorTimer;
    bool processRunning;

    SwString m_program;
    SwStringList m_arguments;
    SwString m_workingDirectory;

    HANDLE hProcess;
    HANDLE hThread;
    HANDLE hStdOutRead;
    HANDLE hStdOutWrite;
    HANDLE hStdErrRead;
    HANDLE hStdErrWrite;
    HANDLE hStdInWrite;
    HANDLE hStdInRead;

    SwIODescriptor* stdoutDescriptor;
    SwIODescriptor* stderrDescriptor;
    SwIODescriptor* stdinDescriptor;

    /**
     * @brief Creates pipes for process communication (stdin, stdout, and stderr).
     *
     * Sets up the standard input, output, and error pipes for the process:
     * - Ensures the descriptors are not already initialized.
     * - Uses `CreatePipe` to create communication pipes.
     * - Configures the handles to prevent inheritance using `SetHandleInformation`.
     * - Wraps the handles in `SwIODescriptor` objects for easier management and adds them to the device.
     *
     * @return `true` if all pipes were successfully created and configured, `false` otherwise.
     *
     * @note This is a private helper function used during process initialization.
     */
    bool createPipes() {
        if (stdoutDescriptor || stderrDescriptor || stdinDescriptor) {
             return true;
         }

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

        stdoutDescriptor = new SwIODescriptor(hStdOutRead, "StdOut");
        stderrDescriptor = new SwIODescriptor(hStdErrRead, "StdErr");
        stdinDescriptor = new SwIODescriptor(hStdInWrite, "StdIn");
        addDescriptor(stdoutDescriptor);
        addDescriptor(stderrDescriptor);
        return true;
    }

    /**
     * @brief Launches a new process with the specified program and arguments.
     *
     * Constructs the command line from the program and arguments, sets up process startup information,
     * and creates the process using the Windows API `CreateProcessW`. Configures standard input, output,
     * and error streams to redirect to the respective pipes.
     *
     * @param program The path to the executable to start.
     * @param arguments A vector of strings representing the arguments to pass to the executable.
     * @param creationFlags Flags to customize the process creation behavior (default: ProcessFlags::NoFlag).
     * @param workingDirectory The directory in which the process should run (optional, defaults to the current directory).
     *
     * @return `true` if the process is successfully created, `false` otherwise.
     *
     * @note This is a private helper function used internally during process initialization.
     */
    bool startProcess(const SwString& program, const SwStringList& arguments,
                      ProcessFlags creationFlags = ProcessFlags::NoFlag,
                      const SwString& workingDirectory = "") {
        SwString command = program;
        for (const auto& argv : arguments) {
            command += SwString(" %1").arg(argv);
        }

        std::wstring wideCommand = command.toStdWString();
        std::wstring wideWorkingDirectory = workingDirectory.toStdWString();

        STARTUPINFOW si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.hStdError = hStdErrWrite;
        si.hStdOutput = hStdOutWrite;
        si.hStdInput = hStdInRead;
        si.dwFlags |= STARTF_USESTDHANDLES;

        ZeroMemory(&pi, sizeof(pi));

        LPCWSTR lpWorkingDir = workingDirectory.isEmpty() ? NULL : wideWorkingDirectory.c_str();

        // Utiliser 'creationFlags' fourni par l'appelant
        if (!CreateProcessW(NULL, &wideCommand[0], NULL, NULL, TRUE, static_cast<DWORD>(creationFlags), NULL, lpWorkingDir, &si, &pi)) {
            DWORD error = GetLastError(); // Récupérer l'erreur
            std::cerr << "CreateProcess failed with error code: " << error << std::endl;
            return false;
        }

        hProcess = pi.hProcess;
        hThread = pi.hThread;
        return true;
    }

private slots:

    /**
     * @brief Slot to check the current status of the process.
     *
     * Monitors the running process to determine if it has terminated.
     * - Retrieves the exit code of the process.
     * - If the process has terminated, emits the `processTerminated` signal with the exit code and calls `close()` to clean up resources.
     * - Logs an error message if unable to retrieve the process's exit code.
     *
     * @note This slot is typically connected to a timer for periodic status monitoring.
     */
    void checkProcessStatus() {
        if (!processRunning) return;
        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
                emit processTerminated(exitCode);
                close();
            }
        }
        else {
            std::cerr << "Failed to get process exit code: " << GetLastError() << std::endl;
        }
    }
};
