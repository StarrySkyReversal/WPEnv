#include "framework.h"
#include "BaseFileOpt.h"
#include <stdio.h>

FileList* ListFiles(const wchar_t* directory) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    wchar_t path[512];

    FileList* list = (FileList*)malloc(sizeof(FileList));
    if (!list) {
        return NULL;
    }
    memset(list, 0, sizeof(FileList));

    _snwprintf_s(path, sizeof(path) / sizeof(wchar_t), _TRUNCATE, L"%ls/*", directory);
    hFind = FindFirstFile(path, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        free(list);
        return NULL;
    }

    list->count = 0;

    do {
        wchar_t* fileFullNameData = findFileData.cFileName;

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Skip directories
            continue;
        }

        // Remove the file extension.
        //wchar_t* dot = wcsrchr(file, '.');
        //if (dot) {
        //    *dot = '\0';
        //}
        size_t len = wcslen(fileFullNameData);
        wchar_t* copyFile = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
        if (!copyFile) {
            free(list);
            FindClose(hFind);
            return NULL;
        }

        wcscpy_s(copyFile, len + 1, fileFullNameData);
        wchar_t* dot = wcsrchr(copyFile, L'.');
        if (dot) {
            *dot = L'\0';
        }

        wchar_t* fileFullName = _wcsdup(fileFullNameData);
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

    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    return list;
}

bool CheckDownloadFileExists(const wchar_t* fileFullName, FileList** outList) {
    *outList = ListFiles(DIRECTORY_DOWNLOAD);

    if (*outList) {
        for (int i = 0; i < (*outList)->count; i++) {
            if (wcscmp((*outList)->filename[i], fileFullName) == 0) {
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

BOOL DirectoryExists(const wchar_t* dirName) {
    DWORD fileAttributes = GetFileAttributes(dirName);

    // Check if the directory exists and it is a directory
    if (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;
    }

    return FALSE;
}

const wchar_t* GetFileFullNameFromUrl(const wchar_t* filename, const wchar_t* url) {
    const wchar_t* dot = wcsrchr(url, L'.');
    if (!dot || dot == url) return _wcsdup(L"");

    wchar_t bufferFileName[2048];
    swprintf_s(bufferFileName, sizeof(bufferFileName) / sizeof(wchar_t), L"%s.%s", filename, (dot + 1));

    wchar_t* fullFileName = (wchar_t*)malloc((wcslen(bufferFileName) + 1) * sizeof(wchar_t));
    if (fullFileName) {
        wcscpy_s(fullFileName, wcslen(bufferFileName) + 1, bufferFileName);
    }

    return fullFileName;
}

void MergeFiles(const wchar_t* destination, const wchar_t* parts[], int num_parts) {
    FILE* dest_file;
    _wfopen_s(&dest_file, destination, L"wb");
    if (!dest_file) {
        perror("Error opening destination file");
        return;
    }

    for (int i = 0; i < num_parts; i++) {
        if (CheckFileExists(parts[i])) {
            FILE* part_file;
            _wfopen_s(&part_file, parts[i], L"rb");

            if (!part_file) {
                perror("Error opening part file");
                fclose(dest_file);
                return;
            }

            char buffer[4096];
            size_t bytes_read;
            while ((bytes_read = fread_s(buffer, sizeof(buffer), 1, sizeof(buffer), part_file)) > 0) {
                fwrite(buffer, 1, bytes_read, dest_file);
            }

            fclose(part_file);
            // delete merged files
            DeleteFile(parts[i]);
        }
    }

    fclose(dest_file);
}

void GetDirectoryFromPath(const wchar_t* fullPath, wchar_t* directory, size_t directorySize, bool getParent) {
    wcsncpy_s(directory, directorySize, fullPath, _TRUNCATE);

    wchar_t* lastSlash = wcsrchr(directory, L'/');
    //#ifdef _WIN32
    //    // For Windows, also check for backslashes.
    //    wchar_t* lastBackslash = wcsrchr(directory, L'\\');
    //    if (lastBackslash && (!lastSlash || lastBackslash > lastSlash)) {
    //        lastSlash = lastBackslash;
    //    }
    //#endif

    // Truncate the content after the delimiter, leaving the directory path. 
    if (lastSlash) {
        if (getParent) {
            *lastSlash = L'\0';  // End the string at the last found slash or backslash.
            lastSlash = (lastSlash == directory) ? nullptr : wcsrchr(directory, L'/');
//#ifdef _WIN32
//            lastBackslash = (lastSlash == directory) ? nullptr : wcsrchr(directory, L'\\');
//            if (lastBackslash && (!lastSlash || lastBackslash > lastSlash)) {
//                lastSlash = lastBackslash;
//            }
//#endif
        }

        if (lastSlash) {
            //*(lastSlash + 1) = L'\0';  // +1 Retain the slash or backslash.
            *(lastSlash) = L'\0';
        }

    }

}

int ReadFileContent(const wchar_t* filePath, wchar_t** buffer, DWORD* size) {
    HANDLE hFile = CreateFileW(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fwprintf(stderr, L"Error opening file: %d\n", GetLastError());
        return 0;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        fwprintf(stderr, L"Error getting file size: %d\n", GetLastError());
        CloseHandle(hFile);
        return 0;
    }

    *size = fileSize / sizeof(wchar_t);
    *buffer = (wchar_t*)malloc((*size + 1) * sizeof(wchar_t)); // +1 for null termination
    if (!*buffer) {
        fwprintf(stderr, L"Memory allocation error\n");
        CloseHandle(hFile);
        return 0;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, *buffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        fwprintf(stderr, L"Error reading file: %d\n", GetLastError());
        CloseHandle(hFile);
        free(*buffer);
        *buffer = NULL;
        return 0;
    }

    (*buffer)[*size] = L'\0'; // null-terminate the string

    CloseHandle(hFile);
    return 1;
}

bool CheckFileExists(const wchar_t* filePath) {
    DWORD dwAttrib = GetFileAttributesW(filePath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool WriteStringToFileAsUTF8(const wchar_t* filePath, const wchar_t* content) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, content, -1, NULL, 0, NULL, NULL);
    if (bufferSize == 0) {
        fwprintf(stderr, L"Error converting to UTF-8: %d\n", GetLastError());
        return false;
    }

    char* utf8Buffer = (char*)malloc(bufferSize);
    if (!utf8Buffer) {
        fwprintf(stderr, L"Memory allocation error.\n");
        return false;
    }

    WideCharToMultiByte(CP_UTF8, 0, content, -1, utf8Buffer, bufferSize, NULL, NULL);

    HANDLE hFile = CreateFileW(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fwprintf(stderr, L"Error opening file for write: %d\n", GetLastError());
        free(utf8Buffer);
        return false;
    }

    DWORD bytesToWrite = bufferSize - 1;  // -1 to exclude the null terminator
    DWORD bytesWritten;

    if (!WriteFile(hFile, utf8Buffer, bytesToWrite, &bytesWritten, NULL) || bytesWritten != bytesToWrite) {
        fwprintf(stderr, L"Error writing to file: %d\n", GetLastError());
        CloseHandle(hFile);
        free(utf8Buffer);
        return false;
    }

    CloseHandle(hFile);
    free(utf8Buffer);
    return true;
}

bool CreateNewFile(const wchar_t* filePath) {
    HANDLE hFile = CreateFileW(filePath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_EXISTS) {
            fwprintf(stderr, L"File already exists.\n");
        }
        else {
            fwprintf(stderr, L"Error creating file: %d\n", GetLastError());
        }
        return false;
    }

    CloseHandle(hFile);
    return true;
}

wchar_t** FindPartFileChunksWithPrefix(const wchar_t* directory, const wchar_t* targetPrefix, int* count) {
//wchar_t** FindPartFileAndDeleteEmptyFile(const wchar_t* directory, const wchar_t* targetPrefix, int* count) {
    WIN32_FIND_DATA findFileData;
    wchar_t searchPattern[MAX_PATH];
    wchar_t** fileList = (wchar_t**)malloc(MAX_FILES * sizeof(wchar_t*));
    *count = 0;

    swprintf_s(searchPattern, _countof(searchPattern), L"%ls/%ls_part_*", directory, targetPrefix);

    HANDLE hFind = FindFirstFile(searchPattern, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        free(fileList);
        return NULL;
    }

    do {
        if (*count < MAX_FILES && wcsstr(findFileData.cFileName, targetPrefix)) {
            fileList[*count] = _wcsdup(findFileData.cFileName);
            (*count)++;
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    return fileList;
}

void freeParFileChunkList(wchar_t** fileList, int count) {
    for (int i = 0; i < count; i++) {
        free(fileList[i]);
    }
    free(fileList);
}

bool GetFileSize(const wchar_t* filepath, unsigned long long* size) {
    HANDLE hFile = CreateFile(
        filepath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        wprintf(L"Error opening file %ls. Error code: %lu\n", filepath, GetLastError());
        return false;
    }

    ULARGE_INTEGER fileSize;
    fileSize.LowPart = GetFileSize(hFile, &fileSize.HighPart);

    if (fileSize.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        wprintf(L"Error getting size of file %ls. Error code: %lu\n", filepath, GetLastError());
        CloseHandle(hFile);
        return false;
    }

    *size = fileSize.QuadPart;

    CloseHandle(hFile);
    return true;
}

wchar_t* get_current_program_directory() {
    static wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);

    wchar_t* last_slash = wcsrchr(buffer, L'\\');
    if (last_slash) {
        *last_slash = L'\0';  // Truncate the string to remove the filename
    }

    return buffer;
}

wchar_t* get_current_program_directory_with_forward_slash() {
    static wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);

    wchar_t* last_slash = wcsrchr(buffer, L'\\');
    if (last_slash) {
        *last_slash = L'\0';  // Truncate the string to remove the filename
    }

    // Replace all backslashes with forward slashes
    for (wchar_t* p = buffer; *p; ++p) {
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
