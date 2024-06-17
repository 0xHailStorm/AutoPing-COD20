#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>
#include <locale.h>

// Function to create a default JSON file if it doesn't exist
void createDefaultJSON(const char* filename) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "w");
    if (err != 0) {
        printf("Cannot create default JSON file.\n");
        exit(1);
    }

    const char* defaultContent = "{\n    \"ping_button\": \"x\",\n    \"status_button\": \"R\"\n}\n";
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

// Source: https://www.cplusplus.com/reference/cstdio/fopen/
//         https://www.cplusplus.com/reference/cstdio/fseek/
//         https://www.cplusplus.com/reference/cstdio/ftell/
//         https://www.cplusplus.com/reference/cstring/strstr/

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


// Source: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput

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

// Source: https://docs.microsoft.com/en-us/windows/win32/toolhelp/taking-a-snapshot-and-viewing-processes

void setConsoleColors(HANDLE hConsole, WORD color) {
    SetConsoleTextAttribute(hConsole, color);
}

void printDashboard(HANDLE hConsole, char pingButton, char statusButton, int running, const ProcessInfo* pInfo) {
    system("cls"); // Clear console screen

    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Light blue
    printf("========================================\n");
    printf("              AutoPing Tool             \n");
    printf("========================================\n");

    setConsoleColors(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); // Yellow
    printf("Developed by: Just#1337 - @0xHailStorm\n");

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

    setConsoleColors(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Reset to light blue
}

// Source for console text color: https://docs.microsoft.com/en-us/windows/console/setconsoletextattribute

void SetConsoleWindowSize(int width, int height) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);


    COORD bufferSize = { width, height };
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    
    SMALL_RECT windowSize = { 0, 0, width - 1, height - 1 };
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
}
/* Source:
    https://learn.microsoft.com/en-us/windows/console/setconsolescreenbuffersize
    https://learn.microsoft.com/en-us/windows/console/setconsolewindowinfo
*/

int main() {
    SetConsoleWindowSize(65, 18);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    char pingButton, statusButton;
    readButtonsFromJSON("config.json", &pingButton, &statusButton);

    int running = 0;
    ProcessInfo pInfo = { 0 };
    int processWasRunning = 0;
    printDashboard(hConsole, pingButton, statusButton, running, &pInfo);
    while (1) {
        int processIsRunning = isProcessRunning(L"cod.exe", &pInfo);

        if (processIsRunning && !processWasRunning) {
            printDashboard(hConsole, pingButton, statusButton, running, &pInfo);
        }

        if (processIsRunning) {
            if (running && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
                SendKey(pingButton);
                Sleep(10); // Add a short delay to prevent rapid firing
                SendKey(pingButton);
            }

            if (GetAsyncKeyState(VkKeyScanA(statusButton)) & 0x8000) {
                running = !running;
                printDashboard(hConsole, pingButton, statusButton, running, &pInfo);
                if (!running) {
                    printf("[-] Program paused...\n");
                }
                else {
                    running = 1;
                    readButtonsFromJSON("config.json", &pingButton, &statusButton); // Reload settings
                    printf("[+] Program resumed with new settings...\n");
                }
                Sleep(300);
            }
        }
        else {
            if (processWasRunning) {
                printDashboard(hConsole, pingButton, statusButton, running, &pInfo);
            }
        }

        processWasRunning = processIsRunning;
        Sleep(10);
    }

    return 0;
}


// Source for reading JSON: https://www.cplusplus.com/reference/cstdio/fopen/
//                          https://www.cplusplus.com/reference/cstdio/fseek/
//                          https://www.cplusplus.com/reference/cstdio/ftell/
//                          https://www.cplusplus.com/reference/cstring/strstr/



// Source for creating default JSON: https://www.cplusplus.com/reference/cstdio/fopen/
//                                   https://www.cplusplus.com/reference/cstdio/fwrite/



// Source for SendInput: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput


// Source for checking process: https://docs.microsoft.com/en-us/windows/win32/toolhelp/taking-a-snapshot-and-viewing-processes


// Source for console text color: https://docs.microsoft.com/en-us/windows/console/setconsoletextattribute


// Source for console WindowSize: https://learn.microsoft.com/en-us/windows/console/setconsolescreenbuffersize
//                                https://learn.microsoft.com/en-us/windows/console/setconsolewindowinfo
