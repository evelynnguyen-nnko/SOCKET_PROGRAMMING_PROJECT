#pragma once

#include <string>
#include <vector>
#include <cstdint> // For using uint8_t (commonly used to represent raw bytes)

/**
 * @brief Interface (“contract”) for Module 2.
 * 
 * This interface defines all concrete operations that Module 1 (CommandServer)
 * is allowed to invoke. Module 1 parses incoming commands, and based on the
 * command logic, it calls these methods in Module 2.
 *
 * Each function represents one capability that the system supports
 * (process management, application management, keylogging, webcam capture, etc.).
 */
class ICommandExecutor {
public:
    /**
     * @brief Virtual destructor.
     * 
     * Required when using interfaces with polymorphism, ensuring derived classes
     * clean up correctly.
     */
    virtual ~ICommandExecutor() {} 

    // --- Application Management ---
    virtual std::string ListApplications() = 0;
    virtual void StartApplication(const std::string& applicationName) = 0;
    virtual void KillApplication(const std::string& applicationIdOrName) = 0;

    // --- Process Management ---
    virtual std::string ListProcess() = 0;
    virtual void StartProcess(const std::string& processName) = 0;
    virtual void KillProcess(const std::string& processIdOrName) = 0;

    // --- Screen Capture ---
    virtual std::vector<uint8_t> PrintScreen() = 0;

    // --- Keylogger ---
    virtual void StartKeyLogging() = 0;
    virtual std::string GetLoggedKeys() = 0;

    // --- Webcam Capture / Video ---
    virtual std::vector<uint8_t> Capture() = 0;

    // --- System Control ---
    virtual void Shutdown() = 0;
    virtual void Restart() = 0;
};