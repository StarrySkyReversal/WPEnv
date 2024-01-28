#include "framework.h"
#include "BaseFileOpt.h"
#include <stdio.h>
#include "Log.h"
#include <errno.h>

FileList* ListFiles(const char* directory) {
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char path[512];

    FileList* list = (FileList*)malloc(sizeof(FileList));
    if (!list) {
        return NULL;
    }
    memset(list, 0, sizeof(FileList));

    _snprintf_s(path, sizeof(path), _TRUNCATE, "%s/*", directory);
    hFind = FindFirstFileA(path, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        free(list);
        return NULL;
    }

    list->count = 0;

    do {
        char* fileFullNameData = findFileData.cFileName;

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Skip directories
            continue;
        }

        size_t len = strlen(fileFullNameData);
        char* copyFile = (char*)malloc((len + 1) * sizeof(char));
        if (!copyFile) {
            free(list);
            FindClose(hFind);
            return NULL;
        }

        strcpy_s(copyFile, len + 1, fileFullNameData);
        char* dot = strrchr(copyFile, '.');
        if (dot) {
            *dot = L'\0';
        }

        char* fileFullName = _strdup(fileFullNameData);
        if (!fileFullName) {
            free(list);
            FindClose(hFind);
            return NULL;
        }

        if (fileFullName && list->count < MAX_FILES) {
            list->filename[list->count] = copyFile;
            list->fileFullName[list->count] = fileFullName;
            list->count += 1;
        }

    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);

    return list;
}

bool CheckDownloadFileExists(const char* fileFullName, FileList** outList) {
    *outList = ListFiles(DIRECTORY_DOWNLOAD);

    if (*outList) {
        for (int i = 0; i < (*outList)->count; i++) {
            if (strcmp((*outList)->filename[i], fileFullName) == 0) {
                return true;
            }
        }
    }

    return false;
}

void FreeCheckDownloadFileExists(FileList* fileList) {
    if (!fileList) return;

    for (int i = 0; i < fileList->count; i++) {
        free(fileList->filename[i]);
        free(fileList->fileFullName[i]);
    }

    free(fileList);
}

BOOL DirectoryExists(const char* dirName) {
    DWORD fileAttributes = GetFileAttributesA(dirName);

    // Check if the directory exists and it is a directory
    if (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;
    }

    return FALSE;
}

const char* GetFileFullNameFromUrl(const char* filename, const char* url) {
    const char* dot = strrchr(url, '.');
    if (!dot || dot == url) return _strdup("");

    char bufferFileName[2048];
    sprintf_s(bufferFileName, sizeof(bufferFileName), "%s.%s", filename, (dot + 1));

    char* fullFileName = (char*)malloc((strlen(bufferFileName) + 1) * sizeof(char));
    if (fullFileName) {
        strcpy_s(fullFileName, strlen(bufferFileName) + 1, bufferFileName);
    }

    return fullFileName;
}

DWORD MergeFiles(const char* destination, const char* parts[], int num_parts) {
    FILE* dest_file;
    errno_t err;
    char err_buffer[512];
    fopen_s(&dest_file, destination, "wb");
    if (!dest_file) {
        Log("Error opening destination file");
        return -1;
    }

    int attempts = 0;
    int max_attempts = 100;

    for (int i = 0; i < num_parts; i++) {
        if (CheckFileExists(parts[i])) {
            FILE* part_file;
            while ((err = fopen_s(&part_file, parts[i], "rb")) != 0 && attempts < max_attempts) {
                strerror_s(err_buffer, sizeof(err_buffer), err);
                Log("Attempt %d ,Error opening part file, msg: %s\r\n", attempts, err_buffer);
                attempts++;

                Sleep(500);
            }

            if (!part_file) {
                Log("Error opening part file unknown error\r\n");
                fclose(dest_file);

                // Delete zip files that failed to execute
                remove(destination);
                return -1;
            }

            char buffer[4096];
            size_t bytes_read;
            while ((bytes_read = fread_s(buffer, sizeof(buffer), 1, sizeof(buffer), part_file)) > 0) {
                fwrite(buffer, 1, bytes_read, dest_file);
            }

            fclose(part_file);
            // delete merged files
            DeleteFileA(parts[i]);
        }
    }

    fclose(dest_file);

    return 0;
}

void GetDirectoryFromPath(const char* fullPath, char* directory, size_t directorySize, bool getParent) {
    strncpy_s(directory, directorySize, fullPath, _TRUNCATE);

    char* lastSlash = strrchr(directory, '/');
    // Truncate the content after the delimiter, leaving the directory path. 
    if (lastSlash) {
        if (getParent) {
            *lastSlash = L'\0';  // End the string at the last found slash or backslash.
            lastSlash = (lastSlash == directory) ? nullptr : strrchr(directory, '/');
        }

        if (lastSlash) {
            //*(lastSlash + 1) = L'\0';  // +1 Retain the slash or backslash.
            *(lastSlash) = '\0';
        }
    }
}

int ReadFileContent(const char* filePath, char** buffer, DWORD* size) {
    HANDLE hFile = CreateFileA(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening file: %d\n", GetLastError());
        return 0;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        fwprintf(stderr, L"Error getting file size: %d\n", GetLastError());
        CloseHandle(hFile);
        return 0;
    }

    *size = fileSize;
    *buffer = (char*)malloc((*size + 1) * sizeof(char)); // +1 for null termination
    if (!*buffer) {
        fprintf(stderr, "Memory allocation error\n");
        CloseHandle(hFile);
        return 0;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, *buffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        fprintf(stderr, "Error reading file: %d\n", GetLastError());
        CloseHandle(hFile);
        free(*buffer);
        *buffer = NULL;
        return 0;
    }

    (*buffer)[*size] = L'\0'; // null-terminate the string

    CloseHandle(hFile);
    return 1;
}

bool CheckFileExists(const char* filePath) {
    DWORD dwAttrib = GetFileAttributesA(filePath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool GetFileSize(const char* filepath, unsigned long long* size) {
    HANDLE hFile = CreateFileA(
        filepath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error opening file %s. Error code: %lu\n", filepath, GetLastError());
        return false;
    }

    ULARGE_INTEGER fileSize;
    fileSize.LowPart = GetFileSize(hFile, &fileSize.HighPart);

    if (fileSize.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        printf("Error getting size of file %s. Error code: %lu\n", filepath, GetLastError());
        CloseHandle(hFile);
        return false;
    }

    *size = fileSize.QuadPart;

    CloseHandle(hFile);
    return true;
}

char* get_current_program_directory() {
    static char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);

    char* last_slash = strrchr(buffer, L'\\');
    if (last_slash) {
        *last_slash = L'\0';  // Truncate the string to remove the filename
    }

    return buffer;
}

char* get_current_program_directory_with_forward_slash() {
    static char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);

    char* last_slash = strrchr(buffer, L'\\');
    if (last_slash) {
        *last_slash = L'\0';  // Truncate the string to remove the filename
    }

    // Replace all backslashes with forward slashes
    for (char* p = buffer; *p; ++p) {
        if (*p == L'\\') {
            *p = L'/';
        }
    }

    return buffer;
}

int copyFile(const char* sourcePath, const char* destPath) {
    FILE* sourceFile, * destFile;
    char ch;

    if (fopen_s(&sourceFile, sourcePath, "r") != 0) {
        perror("Unable to open source file");
        return -1;
    }

    if (fopen_s(&destFile, destPath, "w") != 0) {
        perror("Unable to open destination file");
        fclose(sourceFile);
        return -1;
    }

    while ((ch = fgetc(sourceFile)) != EOF) {
        fputc(ch, destFile);
    }

    fclose(sourceFile);
    fclose(destFile);

    return 0;
}
