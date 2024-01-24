#include "framework.h"

#include <tchar.h>
#include <stdio.h>

#include "Log.h"
#include "FileFindOpt.h"

PathList* initPathList() {
    PathList* list = (PathList*)malloc(sizeof(PathList));
    if (list) {
        list->paths = NULL;
        list->count = 0;
    }
    return list;
}

void addPath(PathList* list, const char* path) {
    char** newPaths = (char**)realloc(list->paths, sizeof(char*) * (list->count + 1));
    if (!newPaths) {
        // Handle memory allocation error if necessary
        return;
    }

    list->paths = newPaths;
    list->paths[list->count] = _strdup(path);
    list->count++;
}

void freePathList(PathList* list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->paths[i]);
        list->paths[i] = NULL;  // Set to NULL after freeing
    }
    free(list->paths);
    list->paths = NULL;  // Set to NULL after freeing
    free(list);
}

void findFilesInDirectory(const char* directory, const char* targetFile, PathList* foundPaths) {
    WIN32_FIND_DATAA findFileData;
    char searchPattern[MAX_PATH];
    _snprintf_s(searchPattern, MAX_PATH, _TRUNCATE, "%s/*.*", directory);
    HANDLE hFind = FindFirstFileA(searchPattern, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        OutputDebugString(L"fail file handle");
        return;
    }

    do {
        const char* fileOrDirName = findFileData.cFileName;

        if (strcmp(fileOrDirName, ".") != 0 && strcmp(fileOrDirName, "..") != 0) {
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                char subDirectory[MAX_PATH];
                _snprintf_s(subDirectory, MAX_PATH, _TRUNCATE, "%s/%s", directory, fileOrDirName);
                findFilesInDirectory(subDirectory, targetFile, foundPaths);
            }
            else if (strcmp(fileOrDirName, targetFile) == 0) {
                char fullPath[MAX_PATH];
                _snprintf_s(fullPath, MAX_PATH, _TRUNCATE, "%s/%s", directory, fileOrDirName);
                addPath(foundPaths, fullPath);
            }
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void ProcessDirectory(const char* directory)
{
    WIN32_FIND_DATAA findFileData;
    char searchPattern[MAX_PATH];
    _snprintf_s(searchPattern, MAX_PATH, "%s/*.*", directory);

    HANDLE hFind = FindFirstFileA(searchPattern, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        // Error handling
    }
    else
    {
        do
        {
            const char* fileOrDirName = findFileData.cFileName;

            // Skip the current directory "." and parent directory ".."
            if (strcmp(fileOrDirName, ".") != 0 && strcmp(fileOrDirName, "..") != 0)
            {
                Log("%ls/%ls\r\n", directory, fileOrDirName);

                // If the found file is a directory, search recursively inside
                if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    char subDirectory[MAX_PATH];
                    _snprintf_s(subDirectory, MAX_PATH, _TRUNCATE, "%s/%s", directory, fileOrDirName);
                    ProcessDirectory(subDirectory);
                }
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
}
