#include "framework.h"

#include "zlib.h"
#include "minizip/unzip.h"
#include "Compression.h"
#include <io.h>
#include <direct.h>
#include "Common.h"
#include "Log.h"
#include "BaseFileOpt.h"


#define CHUNK_SIZE 4096  // You can adjust this based on your requirements.

void create_directory_recursive(const char* path) {
    char tmp[512];

    const char* lastSlash = strrchr(path, '/');
    if (lastSlash) {
        // The "path" is a pointer pointing to the start of the string, and "lastSlash" is a pointer pointing to the last forward slash '/' in the string.
        // When we perform "lastSlash - path," we are actually calculating the distance between the two pointers, or in other words, the offset of "lastSlash" relative to "path."
        // This offset, "dirLen," represents the number of characters from the beginning of the string to the last forward slash (excluding the slash).
        // So, "lastSlash - path" measures the distance between the addresses, which is equivalent to "dirLen" when subtracted from the base address.
        size_t dirLen = lastSlash - path;
        // Copy "dirLen" characters from the starting position in the "path" to the "tmp" character array.
        strncpy_s(tmp, path, dirLen);
        tmp[dirLen] = '\0';
    }
    else {
        return;  // No directories in path
    }

    // Check if the directory already exists.
    if (_access(tmp, 0) != 0) {
        // If it doesn't exist, recursively create the parent directory.
        create_directory_recursive(tmp);
        // Then create the current directory.
        _mkdir(tmp);
    }
}

void MoveFolderContents(const wchar_t* sourceFolder, const wchar_t* destFolder) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    wchar_t srcSearchPath[MAX_PATH];
    swprintf_s(srcSearchPath, L"%ls/*", sourceFolder);

    hFind = FindFirstFile(srcSearchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return; // No files found
    }

    do {
        const wchar_t* fileName = findFileData.cFileName;

        if (wcscmp(fileName, L".") != 0 && wcscmp(fileName, L"..") != 0) {
            wchar_t srcPath[MAX_PATH];
            wchar_t destPath[MAX_PATH];

            swprintf_s(srcPath, L"%ls/%ls", sourceFolder, fileName);
            swprintf_s(destPath, L"%ls/%ls", destFolder, fileName);

            MoveFile(srcPath, destPath);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

int extract_zip_file(const wchar_t* zip_filename, const wchar_t* serviceType, const wchar_t* version) {
    if (!DirectoryExists(DIRECTORY_SERVICE)) {
        CreateDirectory(DIRECTORY_SERVICE, NULL);
    }

    wchar_t serveiceTypeDirectory[512];
    swprintf_s(serveiceTypeDirectory, sizeof(serveiceTypeDirectory) / sizeof(wchar_t), L"%ls/%ls", DIRECTORY_SERVICE, serviceType);

    if (!DirectoryExists(serveiceTypeDirectory)) {
        CreateDirectory(serveiceTypeDirectory, NULL);
    }
    /////////////////
    wchar_t tempVersionDirectory[512];
    swprintf_s(tempVersionDirectory, L"%ls/temp_%ls", serveiceTypeDirectory, version);
    if (DirectoryExists(tempVersionDirectory)) {
        LogAndMsgBox(L"Temporary directory exists, please manual delete this directory %ls\r\n", tempVersionDirectory);
        return -3;
    }

    // 创建临时目录
    if (!CreateDirectory(tempVersionDirectory, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        LogAndMsgBox(L"Failed to create temporary directory %ls\r\n", tempVersionDirectory);
        return -3;
    }
    /////////////////

    char multipleByteZipFile[1024];
    WToM(zip_filename, multipleByteZipFile, sizeof(multipleByteZipFile));

    unzFile zfile = unzOpen(multipleByteZipFile);
    if (!zfile) {
        LogAndMsgBox(L"Cannot open ZIP file %ls\r\n", zip_filename);
        return -1;
    }

    // version directory
    wchar_t versionDirectory[1024];
    swprintf_s(versionDirectory, sizeof(versionDirectory) / sizeof(wchar_t), L"%ls/%ls/%ls", DIRECTORY_SERVICE, serviceType, version);
    if (DirectoryExists(versionDirectory)) {
        unzClose(zfile);

        LogAndMsgBox(L"The corresponding version folder already exists: %ls/%ls/%ls\r\n", DIRECTORY_SERVICE, serviceType, version);
        return -3;
    }

    char buffer[CHUNK_SIZE];

    unsigned int topLevelItemCount = 0;
    char firstTopLevelItem[256] = { '\0' };
    // First pass: Detect the structure of the ZIP
    if (unzGoToFirstFile(zfile) != UNZ_OK) {
        unzClose(zfile);

        LogAndMsgBox(L"Failed to find the first file in the ZIP %ls\r\n", zip_filename);
        return -1;
    }

    unzGoToFirstFile(zfile);  // Reset to the first file before extraction.

    do {
        if (unzOpenCurrentFile(zfile) != UNZ_OK) {
            unzClose(zfile);

            LogAndMsgBox("Failed to open the current file\r\n");
            return -1;
        }

        char filename_inzip[512];
        unz_file_info file_info;
        unzGetCurrentFileInfo(zfile, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);

        char fullPath[2048];
        char mTempVersionDirectoryPath[512];
        WToM(tempVersionDirectory, mTempVersionDirectoryPath, sizeof(mTempVersionDirectoryPath));
        sprintf_s(fullPath, "%s/%s", mTempVersionDirectoryPath, filename_inzip);

        // If it's a directory, create it.
        if (filename_inzip[strlen(filename_inzip) - 1] == '/') {
            _mkdir(fullPath);
        }
        else {
            create_directory_recursive(fullPath);

            FILE* outfile;
            fopen_s(&outfile, fullPath, "wb");
            if (!outfile) {
                unzCloseCurrentFile(zfile);
                unzClose(zfile);

                LogAndMsgBox("Cannot create unzip file %s\r\n", fullPath);
                return -1;
            }

            int error = UNZ_OK;
            do {
                error = unzReadCurrentFile(zfile, buffer, CHUNK_SIZE);
                if (error < 0) {
                    LogAndMsgBox("Error %d reading zipped file\r\n", error);

                    return -1;
                }

                if (error > 0) {
                    fwrite(buffer, error, 1, outfile);  // Write data to file.
                }
            } while (error > 0);

            fclose(outfile);
        }

        // Count the number of top-level entries
        char* secondLevel = strchr(filename_inzip, '/');
        if (secondLevel) {
            *secondLevel = '\0';
            if (topLevelItemCount == 0 || strcmp(firstTopLevelItem, filename_inzip) != 0) {
                strcpy_s(firstTopLevelItem, sizeof(firstTopLevelItem), filename_inzip);
                topLevelItemCount++;
            }
            *secondLevel = '/';
        }
        else {
            topLevelItemCount++;
        }

        unzCloseCurrentFile(zfile);
    } while (unzGoToNextFile(zfile) == UNZ_OK);

    unzClose(zfile);

    //wchar_t finalDirectory[512];
    //swprintf_s(finalDirectory, L"%ls/%ls/", serveiceTypeDirectory, version);
    if (topLevelItemCount == 1) {
        wchar_t wFirstTopLevelItem[512];
        MToW(firstTopLevelItem, wFirstTopLevelItem, _countof(wFirstTopLevelItem));

        // Rename topLevel directory
        wchar_t oldPath[1024];
        swprintf_s(oldPath, L"%ls/%ls", tempVersionDirectory, wFirstTopLevelItem);
        _wrename(oldPath, versionDirectory);
    }
    else {
        // Move all projects to new folder
        CreateDirectory(versionDirectory, NULL);
        MoveFolderContents(tempVersionDirectory, versionDirectory);
    }

    // Delete temporary directory
    RemoveDirectory(tempVersionDirectory);

    return 0;
}
