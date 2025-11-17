#include "CommandExecutor.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <memory>      // std::unique_ptr
#include <stdexcept>   // std::runtime_error
#include <array>       // std::array for buffer

#ifdef _WIN32
#include <windows.h>    // Windows API
#endif

/**
 * @brief Helper function used to execute a shell command
 *        and return its output as a string.
 *
 * @param cmd The full shell command (e.g., "tasklist", "ps aux").
 * @return The console output produced by the executed command.
 */
std::string CommandExecutor::executeShellCommand(const char* cmd) {
    std::array<char, 128> buffer{};
    std::string result;

    // unique_ptr to ensure pclose() is always called (RAII)
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);

    if (!pipe)
        throw std::runtime_error("Error: _popen() failed!");
    
    // Read output in cmd line by line
    while (fgets(buffer.data(), buffer.size(), pipe.get())) 
        result += buffer.data();
    
    return result;
}

// --- Application Management ---
std::string CommandExecutor::ListApplications() {
    std::cout << "[Module 2] Executing: List Applications.\n";
    return executeShellCommand("dir \"C:\\Program Files\""); // "tasklist /APPS /FO CSV /NH"
}
void CommandExecutor::StartApplication(const std::string& applicationName) {
    std::cout << "[Module 2] Executing: Start Application (" << applicationName << ").\n";
    std::string cmd = "start \"\" \"" + applicationName + "\"";
    executeShellCommand(cmd.c_str());
}
void CommandExecutor::KillApplication(const std::string& applicationIdOrName) {
    std::cout << "[Module 2] Executing: Kill Application (" << applicationIdOrName << ").\n";
    // F: Force (to close)
    // T:Tree (kill all sub processes)
    // Check if input is PID or Name
    std::string cmd;
    try {
        std::stoul(applicationIdOrName);
        // If no error -> PID
        cmd = "taskkill /F /T /PID " + applicationIdOrName;
    } catch (const std::exception&) {
        // If fail -> Name
        cmd = "taskkill /F /T /IM \"" + applicationIdOrName + "\"";
    }
    executeShellCommand(cmd.c_str());
}

// --- Process Management ---
std::string CommandExecutor::ListProcess() {
     std::cout << "[Module 2] Executing: List Processes.\n";
    return executeShellCommand("tasklist");
}
void CommandExecutor::StartProcess(const std::string& processName) {
    std::cout << "[Module 2] Executing: Start Process (" << processName << ").\n";
    std::string cmd = "start \"\" \"" + processName + "\"";
    executeShellCommand(cmd.c_str());
}
void CommandExecutor::KillProcess(const std::string& processIdOrName) {
    std::cout << "[Module 2] Executing: Kill Process (" << processIdOrName << ").\n";

    std::string cmd;
    try {
        std::stoul(processIdOrName);
        // If no error -> PID
        cmd = "taskkill /F /PID" + processIdOrName;
    } catch (const std::exception&) {
        // If fail -> Name
        cmd = "taskkill /F /IM \"" + processIdOrName + "\"";
    }
    executeShellCommand(cmd.c_str());
}

// --- Screen Capture ---
std::vector<uint8_t> CommandExecutor::PrintScreen() {
    return std::vector<uint8_t>();
}

// --- Keylogger ---
void CommandExecutor::StartKeyLogging() {

}
std::string CommandExecutor::GetLoggedKeys() {
    return "";
}

// --- Webcam Capture / Video ---
std::vector<uint8_t> CommandExecutor::Capture() {
    return std::vector<uint8_t>();
}

// --- System Control ---
void CommandExecutor::Shutdown() {
    std::cout << "[Module 2] Executing: Shutdown.\n";
    executeShellCommand("shutdown /s /t 0"); // could set to shutdown after 'time'
}
void CommandExecutor::Restart() {
    std::cout << "[Module 2] Excuting: Restart.\n";
    executeShellCommand("shutdown /r /t 0");
}

