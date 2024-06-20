#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>
#include <Xinput.h>
#include <locale.h>

#pragma comment(lib, "Xinput.lib")

BOOL controllerConnected = FALSE;
// Function to create a default JSON file if it doesn't exist
void createDefaultJSON(const char* filename) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "w");
    if (err != 0) {
        printf("Cannot create default JSON file.\n");
        exit(1);
    }

    const char* defaultContent = "{\n    \"ping_button\": \"l\",\n    \"status_button\": \"k\"\n}\n";
    fwrite(defaultContent, sizeof(char), strlen(defaultContent), file);
    fclose(file);
    printf("Default JSON file created.\n");
}

// Function to read the keys from JSON file manually
void readButtonsFromJSON(const char* filename, char* pingButton, char* statusButton) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "r");
    if (err != 0) {
        createDefaultJSON(filename);
        err = fopen_s(&file, filename, "r");
        if (err != 0) {
            printf("Cannot open file after creating default.\n");
            exit(1);
        }
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(fileSize + 1);
    fread(buffer, 1, fileSize, file);
    fclose(file);
    buffer[fileSize] = '\0';

    char* pingKey = "\"ping_button\": \"";
    char* statusKey = "\"status_button\": \"";

    char* pingStart = strstr(buffer, pingKey);
    char* statusStart = strstr(buffer, statusKey);

    if (pingStart == NULL || statusStart == NULL) {
        printf("Keys not found in JSON.\n");
        free(buffer);
        exit(1);
    }

    pingStart += strlen(pingKey);
    statusStart += strlen(statusKey);

    *pingButton = pingStart[0];
    *statusButton = statusStart[0];

    free(buffer);
}

// Function to send a key using SendInput
void SendKey(char key) {
    INPUT inputs[2] = { 0 };

    // Convert character to virtual-key code and then to scancode
    UINT vkCode = VkKeyScanA(key); // Get the virtual-key code for the character
    if (vkCode == -1) {
        printf("Invalid key: %c\n", key);
        return;
    }
    UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC); // Convert virtual-key code to scancode

    // First key down
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wScan = scanCode;
    inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE; // Key down with scancode

    // First key up
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wScan = scanCode;
    inputs[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // Key up with scancode

    // Send the key down
    SendInput(1, &inputs[0], sizeof(INPUT));
    Sleep(50); // Short delay to simulate a real key press
    // Send the key up
    SendInput(1, &inputs[1], sizeof(INPUT));
}

// Structure to hold process information
typedef struct {
    DWORD pid;
    wchar_t exeFile[MAX_PATH];
    DWORD_PTR baseAddress;
} ProcessInfo;

// Function to check if a specific process is running and gather its information
int isProcessRunning(const wchar_t* processName, ProcessInfo* pInfo) {
    int exists = 0;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                exists = 1;
                pInfo->pid = entry.th32ProcessID;
                wcscpy_s(pInfo->exeFile, MAX_PATH, entry.szExeFile);
                HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, entry.th32ProcessID);
                if (hModuleSnap != INVALID_HANDLE_VALUE) {
                    MODULEENTRY32 me32;
                    me32.dwSize = sizeof(MODULEENTRY32);
                    if (Module32First(hModuleSnap, &me32)) {
                        pInfo->baseAddress = (DWORD_PTR)me32.modBaseAddr;
                    }
                    CloseHandle(hModuleSnap);
                }
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return exists;
}

// Function to check if a game controller is connected
BOOL isControllerConnected() {
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    DWORD result = XInputGetState(0, &state);
    return (result == ERROR_SUCCESS);
}

// Function to get the type of game controller connected
const char* getControllerType() {
    XINPUT_CAPABILITIES caps;
    ZeroMemory(&caps, sizeof(XINPUT_CAPABILITIES));
    if (XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &caps) == ERROR_SUCCESS) {
        switch (caps.SubType) {
        case XINPUT_DEVSUBTYPE_GAMEPAD:
            return "Xbox Controller";
        case XINPUT_DEVSUBTYPE_WHEEL:
            return "Wheel";
        case XINPUT_DEVSUBTYPE_ARCADE_STICK:
            return "Arcade Stick";
        case XINPUT_DEVSUBTYPE_FLIGHT_STICK:
            return "Flight Stick";
        case XINPUT_DEVSUBTYPE_DANCE_PAD:
            return "Dance Pad";
        case XINPUT_DEVSUBTYPE_GUITAR:
            return "Guitar";
        case XINPUT_DEVSUBTYPE_DRUM_KIT:
            return "Drum Kit";
        default:
            return "Unknown";
        }
    }
    return "None";
}

void setConsoleColors(HANDLE hConsole, WORD color) {
    SetConsoleTextAttribute(hConsole, color);
}

void printDashboard(HANDLE hConsole, char pingButton, char statusButton, int running, const ProcessInfo* pInfo, const char* controllerType) {
    system("cls"); // Clear console screen

    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf("========================================\n");
    printf("              AutoPing Tool             \n");
    printf("========================================\n");

    setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); // Yellow
    printf("Developed by: Just#1337 - @0xHailStorm\nDiscord : @Win32Apis - https://discord.gg/kom\n");

    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf("========================================\n");

    printf("[+] Ping Button: ");
    setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
    printf("%c\n", pingButton);

    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf("[+] Status Button: ");
    setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
    printf("%c\n", statusButton);

    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf("========================================\n");

    printf("Press ");
    setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
    printf("%c", statusButton);
    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf(" to toggle running state.\n");

    printf("Press ");
    setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
    printf("left mouse button");
    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf(" to send ping.\n");

    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf("========================================\n");

    if (running) {
        setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY); // Green
        printf("[+] Program is running...\n");
    }
    else {
        setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
        printf("[-] Program is paused...\n");
    }

    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf("========================================\n");
    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf("[+] Controller: ");
    if (controllerType == "None") {
        setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
        printf("%s\n", controllerType);
    }
    else {
        setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY); // Green
        printf("%s\n", controllerType);
    }

    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    if (pInfo->pid != 0) {
        printf("[+] Process Name: ");
        setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
        wprintf(L"%s\n", pInfo->exeFile);

        setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
        printf("[+] Process ID: ");
        setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
        printf("%lu\n", pInfo->pid);

        setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
        printf("[+] Base Address: ");
        setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
        printf("0x%p\n", (void*)pInfo->baseAddress);
    }
    else {
        setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // Red
        printf("[-] Target process not running.\n");
    }

   
}

// Function to send key press when R2 or RT is pressed on controller
void checkControllerAndSendKey(char pingButton) {
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    if (XInputGetState(0, &state) == ERROR_SUCCESS) {
        if (state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
            SendKey(pingButton);
            Sleep(3);
            SendKey(pingButton);
        }
    }
}

void SetConsoleWindowSize(int width, int height) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD bufferSize = { width, height };
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    SMALL_RECT windowSize = { 0, 0, width - 1, height - 1 };
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
}

int main() {
    SetConsoleWindowSize(65, 20);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    char pingButton, statusButton;
    readButtonsFromJSON("config.json", &pingButton, &statusButton);


    int running = 0;
    ProcessInfo pInfo = { 0 };
    int processWasRunning = 0;

    const char* controllerType = isControllerConnected() ? getControllerType() : "None";
    printDashboard(hConsole, pingButton, statusButton, running, &pInfo, controllerType);

    while (1) {
        int processIsRunning = isProcessRunning(L"cod.exe", &pInfo);

        BOOL currentControllerState = isControllerConnected();
        if (currentControllerState != controllerConnected) {
            controllerConnected = currentControllerState;
            controllerType = controllerConnected ? getControllerType() : "None";
            printDashboard(hConsole, pingButton, statusButton, running, &pInfo, controllerType);
        }

        if (processIsRunning && !processWasRunning) {
            controllerType = controllerConnected ? getControllerType() : "None";
            printDashboard(hConsole, pingButton, statusButton, running, &pInfo, controllerType);
        }

        if (processIsRunning) {
            if (running && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
                SendKey(pingButton);
                Sleep(10); // Add a short delay to prevent rapid firing
                SendKey(pingButton);
            }
            if ((GetKeyState(VkKeyScanA(statusButton)) & 0x8000) != 0) {
                running = !running;
                controllerType = controllerConnected ? getControllerType() : "None";
                printDashboard(hConsole, pingButton, statusButton, running, &pInfo, controllerType);
                if (!running) {
                }
                else {
                    running = 1;
                    readButtonsFromJSON("config.json", &pingButton, &statusButton); // Reload settings
                }
                Sleep(300);
            }

            if (running) {
                checkControllerAndSendKey(pingButton);
            }
        }
        else {
            if (processWasRunning) {
                controllerType = controllerConnected ? getControllerType() : "None";
                printDashboard(hConsole, pingButton, statusButton, running, &pInfo, controllerType);
            }
        }

        processWasRunning = processIsRunning;
        Sleep(10);
    }

    return 0;
}


// Source for checking if a controller is connected: 
// https://docs.microsoft.com/en-us/windows/win32/api/xinput/nf-xinput-xinputgetstate

// Source for getting the type of the connected controller: 
// https://docs.microsoft.com/en-us/windows/win32/api/xinput/nf-xinput-xinputgetcapabilities

// Source for setting console text colors: 
// https://docs.microsoft.com/en-us/windows/console/setconsoletextattribute

// Source for printing the dashboard: 
// https://docs.microsoft.com/en-us/windows/console/cls

// Source for sending a key using SendInput: 
// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput

// Source for checking if a specific process is running and gathering its information: 
// https://docs.microsoft.com/en-us/windows/win32/toolhelp/taking-a-snapshot-and-viewing-processes

// Source for checking if R2 or RT button is pressed on the controller: 
// https://docs.microsoft.com/en-us/windows/win32/api/xinput/nf-xinput-xinputgetstate

// Source for setting console window size: 
// https://learn.microsoft.com/en-us/windows/console/setconsolescreenbuffersize
// https://learn.microsoft.com/en-us/windows/console/setconsolewindowinfo

// Source for reading JSON: 
// https://www.cplusplus.com/reference/cstdio/fopen/
// https://www.cplusplus.com/reference/cstdio/fseek/
// https://www.cplusplus.com/reference/cstdio/ftell/
// https://www.cplusplus.com/reference/cstring/strstr/

// Source for creating default JSON: 
// https://www.cplusplus.com/reference/cstdio/fopen/
// https://www.cplusplus.com/reference/cstdio/fwrite/

