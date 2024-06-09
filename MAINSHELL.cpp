#include<stdio.h>
#include<string.h>
#include<windows.h>
#include <tlhelp32.h>
#include<tchar.h>
#include<unistd.h>
#include<signal.h>
#define MAX 1000

void run_single_command(char* input);
void printHelp(){
    printf("help:    provide information to shell commands\n");
    printf("exit:    exit shell\n"); 
    printf("dir:     display a list of files and subdirectories in current directory\n");
    printf("time:    display/change current time\n");
    printf("date:    display or change current date\n");
    printf("kill:    kill a process\n"); 
    printf("stop:    stop a process permanently\n");
    printf("resume:  resume a stopped process\n");
    printf("list:    list all background processes\n");
    printf("path:    get environment PATH variable\n");
    printf("addpath: add environment PATH variable\n");
    printf("clear:   clear all screen\n");
    printf("execute: run .bat file\n");
    printf("delete:  delete a file\n");
    printf("move:    move a file to other address\n");
}
void clear(){
    system("cls");
}
void exitShell() {
    printf("Exiting shell. Goodbye!\n");
    exit(0);
}
void list(){
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        printf("Error creating snapshot.\n");
        return ;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &processEntry)) {
        do {
            printf("Process ID: %d, Name: %s\n", processEntry.th32ProcessID, processEntry.szExeFile);
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
}
HANDLE hForegroundProcess = NULL;
void handle_sigint(int sig) {
    if (hForegroundProcess != NULL) {
        // terminate the foreground process
        TerminateProcess(hForegroundProcess, 0);
        CloseHandle(hForegroundProcess);
        hForegroundProcess = NULL;
        printf("\nForeground process terminated.\n");
    }
}
void create_process(char* input, int mode){
    STARTUPINFO si;    
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    //convert to wchar
    TCHAR command[MAX];
    #ifdef UNICODE
    MultiByteToWideChar(CP_ACP, 0, input, -1, command, MAX);
    #else
    strncpy(command, input, MAX);
    #endif

    //create process
    if( !CreateProcess( NULL,   // No module name (use command line).
        command, // Command line.
        NULL,             // Process handle not inheritable.
        NULL,             // Thread handle not inheritable.
        FALSE,            // Set handle inheritance to FALSE.
        CREATE_NEW_CONSOLE,                // No creation flags.
        NULL,             // Use parent's environment block.
        NULL,             // Use parent's starting directory.
        &si,              // Pointer to STARTUPINFO structure.
        &pi )             // Pointer to PROCESS_INFORMATION structure.
    ){ 
        printf("CreateProcess failed.\n");
        printf("Use: run <OPTION> <exeFile>\n");
        printf("   -f: foreground mode\n   -b: background mode\n");
        return;
    }
    
    // mode ==0 mean run in foreground mode
    if (mode == 0) {
        hForegroundProcess = pi.hProcess;
        // Wait until child process exits
        WaitForSingleObject(pi.hProcess, INFINITE);
        hForegroundProcess = NULL;
    }

   
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}
DWORD FindProcessId(const char *processName) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    DWORD processID = 0;

 
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return 0;
    }

    
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return 0;
    }


    do {
        if (strcmp((const char*)pe32.szExeFile, processName) == 0) {
            processID = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return processID;
}

//terminate a process by its process ID
BOOL kill(DWORD processID) {
    //open existing process to interact with it
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
    if (hProcess == NULL) {
        printf("Error: Unable to open process for termination!\n");
        return FALSE;
    }

    BOOL result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);

    return result;
}
void stop(DWORD processID){
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(hThreadSnap == INVALID_HANDLE_VALUE) return;
    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    if(Thread32First(hThreadSnap,&te32)){
        do{
            if(te32.th32OwnerProcessID == processID){
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                if (hThread != NULL) {
                    SuspendThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while(Thread32Next(hThreadSnap,&te32));
    }
    CloseHandle(hThreadSnap);
}
void resume(DWORD processID) {
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hThreadSnap, &te32)) {
        do {
            if (te32.th32OwnerProcessID == processID) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                if (hThread != NULL) {
                    ResumeThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hThreadSnap, &te32));
    }
    CloseHandle(hThreadSnap);
}

void dir(const char* directory){
    WIN32_FIND_DATA findFileData;
        // convert directory from const char to TCHAR
    TCHAR dirW[MAX];
    #ifdef UNICODE
    MultiByteToWideChar(CP_ACP, 0, directory, -1, dirW, MAX);
    #else
    strncpy(dirW, directory, MAX);
    #endif

    HANDLE hFind = FindFirstFile(dirW,&findFileData);
     if (hFind == INVALID_HANDLE_VALUE) {
        printf("Invalid file handle. Error is %u\n", GetLastError());
        return;
    } 

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
            printf("%s\n", findFileData.cFileName);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    
    if (GetLastError() != ERROR_NO_MORE_FILES) {
        printf("FindNextFile error. Error is %u\n", GetLastError());
    }

    FindClose(hFind);
}

void getPath() {
    char* path = getenv("PATH");
    if (path != NULL) {
        printf("Current PATH: %s\n", path);
    } else {
        printf("Unable to get PATH variable.\n");
    }
}
void addPath(const char* newPath) {
    // Open the registry key where the PATH environment variable is stored
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        fprintf(stderr, "Failed to open registry key.\n");
        return;
    }

    // Read the current PATH variable
    char currentPath[1024];
    DWORD bufferSize = sizeof(currentPath);
    if (RegQueryValueExA(hKey, "Path", NULL, NULL, (LPBYTE)currentPath, &bufferSize) != ERROR_SUCCESS) {
        fprintf(stderr, "Failed to read current PATH.\n");
        RegCloseKey(hKey);
        return;
    }

    // Concatenate the new path to the current PATH
    char updatedPath[2048];
    sprintf(updatedPath, "%s;%s", currentPath, newPath);

    // Write the updated PATH back to the registry
    if (RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, (const BYTE*)updatedPath, strlen(updatedPath) + 1) != ERROR_SUCCESS) {
        fprintf(stderr, "Failed to update PATH.\n");
    } else {
        printf("PATH updated successfully.\n");
    }

    RegCloseKey(hKey);

    // Notify other applications about the change in environment variables
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
}

void rundotBat(const char* input){
    FILE *fptr;
    fptr = fopen(input,"r");
    char line[150]; // Buffer to store the line
    while (fgets(line, sizeof(line), fptr) != NULL) {
        line[strcspn(line,"\n")]=0;
        printf("%s\n",line);
        run_single_command(line);
    }
    fclose(fptr);
}
void Date(){
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("Current date: %02d/%02d/%04d\n", st.wDay, st.wMonth, st.wYear);

    int day,month,year;
    printf("Enter new date (dd/mm/yyyy): ");
    if (scanf("%d/%d/%d", &day, &month, &year) == 3) {
        st.wDay = day;
        st.wMonth = month;
        st.wYear = year;

        if (SetLocalTime(&st)) {
            printf("Date successfully updated.\n");
        } else {
            printf("Failed to update date. Error code: %d\n", GetLastError());
        }
    } else {
        printf("Invalid date format. Please use dd/mm/yyyy.\n");
    }
}
void time(){
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("Current time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);

    int hour, minute, second;
    printf("Enter new time: ");
    if(scanf("%d:%d:%d", &hour,&minute,&second)==3){
        st.wHour = hour;
        st.wMinute = minute;
        st.wSecond = second;

        if(SetLocalTime(&st)){
            printf("Time successfully updated.\n");
        }
        else{
            printf("Failed to updated time. Error code: %d\n", GetLastError());
        }
    }
}

void deleteFile(const char* path) {
    //convert to wchar
    TCHAR wPath[MAX];
    #ifdef UNICODE
    MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX);
    #else
    strncpy(wPath, path, MAX);
    #endif
    
    //delete file
    if (DeleteFile(wPath)) {
        printf("File removed successfully.\n");
    } else {
        printf("Failed to remove file. Error code: %d\n", GetLastError());
    }
}

void moveFile(const char* source, const char* destination) {
    //convert to wchar
    TCHAR wSource[MAX], wDes[MAX];
    #ifdef UNICODE
    MultiByteToWideChar(CP_ACP, 0, source, -1, wSource, MAX);
    MultiByteToWideChar(CP_ACP, 0, destination, -1, wDes, MAX);
    #else
    strncpy(wSource, source, MAX);
    strncpy(wDes, destination, MAX);
    #endif

    //move file
    if (MoveFile(wSource, wDes)) {
        printf("File moved successfully.\n");
    } else {
        printf("Failed to move file. Error code: %d\n", GetLastError());
    }
}

void run_single_command(char* input){
    
        if(strcmp(input,"help")==0) printHelp();
        else if(strcmp(input,"list")==0) list();
        else if(strcmp(input,"exit")==0) exitShell();
        else if(strncmp(input,"run -f ", 7 )==0){
            //move strlen(input-7)+1 char (include NULL) start from input + strlen("run -f ") to input
            memmove(input,input+7,strlen(input)-7+1);
            create_process(input,0);
        }
         else if(strncmp(input,"run -b ", 7 )==0){
            memmove(input,input+7,strlen(input)-7+1);
            create_process(input,1); // 1 mean background mode
        }
        else if(strncmp(input,"kill ",5)==0){
            memmove(input,input+5,strlen(input)-5+1);
            DWORD processid = FindProcessId(input);
            kill(processid);

        }
        else if(strncmp(input,"stop ",5)==0){
            memmove(input,input+5,strlen(input)-5+1);
            DWORD processid = FindProcessId(input);
            stop(processid);

        }
        else if(strncmp(input,"resume ",7)==0){
            memmove(input,input+7,strlen(input)-7+1);
            DWORD processid = FindProcessId(input);
            resume(processid);
        }
        else if(strncmp(input,"clear",5)==0) clear();
        else if(strncmp(input,"dir ",4)==0){
            memmove(input,input+4,strlen(input)-4+1);
            strcat(input, "\\*.*");
            dir(input);
        }
        else if (strncmp(input, "path",4) == 0) {
            getPath();
        }
        else if (strncmp(input, "addpath ", 8) == 0) {
            memmove(input, input + 8, strlen(input) - 8 + 1);
            addPath(input);
        }
        else if(strncmp(input,"execute ",8)==0){
            memmove(input, input + 8, strlen(input) - 8 + 1);
            rundotBat(input);
        }
        else if(strcmp(input,"date")==0){
            Date();
        }
        else if(strcmp(input,"time")==0){
            time();
        }
        else if (strncmp(input, "delete ", 7) == 0) {
            memmove(input, input + 7, strlen(input) - 7 + 1);
            deleteFile(input);
        }
        else if(strncmp(input, "move ",5)==0){
            memmove(input,input+5,strlen(input)-5+1);
            char* sign = strchr(input,'>');
            if((sign!=NULL)){
                *(sign-1) ='\0';
                const char* source = input;
                const char* destination = sign+2;
                moveFile(source, destination);
            }
            else {
            printf("Invalid move command format. Use: move <source_path> > <destination_path>\n");
            }
        }
        else{
            printf("ERROR: Invalid command \n");
            printf("Please use 'help' command for more information\n");
        }
}


int main(){
    printf("Hello, welcome to my shell.\n");
    signal(SIGINT, handle_sigint);
    while(1){
        printf("\033[1;31m Tiny Shell> \033[0m ");
        char input[MAX];
        fgets(input,MAX,stdin);
        input[strcspn(input,"\n")] = 0;
        run_single_command(input);

    }


}