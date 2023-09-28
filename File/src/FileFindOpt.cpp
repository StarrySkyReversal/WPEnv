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

void addPath(PathList* list, const wchar_t* path) {
    wchar_t** newPaths = (wchar_t**)realloc(list->paths, sizeof(wchar_t*) * (list->count + 1));
    if (!newPaths) {
        // Handle memory allocation error if necessary
        return;
    }

    list->paths = newPaths;
    list->paths[list->count] = _wcsdup(path);
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

void findFilesInDirectory(const wchar_t* directory, const wchar_t* targetFile, PathList* foundPaths) {
    WIN32_FIND_DATA findFileData;
    wchar_t searchPattern[MAX_PATH];
    _snwprintf_s(searchPattern, MAX_PATH, _TRUNCATE, L"%ls/*.*", directory);

    HANDLE hFind = FindFirstFile(searchPattern, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        OutputDebugString(L"fail file handle");
        return;
    }

    do {
        const wchar_t* fileOrDirName = findFileData.cFileName;

        if (wcscmp(fileOrDirName, L".") != 0 && wcscmp(fileOrDirName, L"..") != 0) {
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                wchar_t subDirectory[MAX_PATH];
                _snwprintf_s(subDirectory, MAX_PATH, _TRUNCATE, L"%ls/%ls", directory, fileOrDirName);
                findFilesInDirectory(subDirectory, targetFile, foundPaths);
            }
            else if (wcscmp(fileOrDirName, targetFile) == 0) {
                wchar_t fullPath[MAX_PATH];
                _snwprintf_s(fullPath, MAX_PATH, _TRUNCATE, L"%ls/%ls", directory, fileOrDirName);
                addPath(foundPaths, fullPath);
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void ProcessDirectory(const wchar_t* directory)
{
    WIN32_FIND_DATA findFileData;
    wchar_t searchPattern[MAX_PATH];
    _snwprintf_s(searchPattern, MAX_PATH, L"%ls/*.*", directory);

    HANDLE hFind = FindFirstFile(searchPattern, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        // Error handling
    }
    else
    {
        do
        {
            const wchar_t* fileOrDirName = findFileData.cFileName;

            // Skip the current directory "." and parent directory ".."
            if (wcscmp(fileOrDirName, L".") != 0 && wcscmp(fileOrDirName, L"..") != 0)
            {
                Log("%ls/%ls\r\n", directory, fileOrDirName);

                // If the found file is a directory, search recursively inside
                if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    wchar_t subDirectory[MAX_PATH];
                    _snwprintf_s(subDirectory, MAX_PATH, _TRUNCATE, L"%ls/%ls", directory, fileOrDirName);
                    ProcessDirectory(subDirectory);
                }
            }
        } while (FindNextFile(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
}
