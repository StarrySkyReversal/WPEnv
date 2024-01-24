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

void MoveFolderContents(const char* sourceFolder, const char* destFolder) {
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    char srcSearchPath[MAX_PATH];
    sprintf_s(srcSearchPath, "%s/*", sourceFolder);

    hFind = FindFirstFileA(srcSearchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return; // No files found
    }

    do {
        const char* fileName = findFileData.cFileName;

        if (strcmp(fileName, ".") != 0 && strcmp(fileName, "..") != 0) {
            char srcPath[MAX_PATH];
            char destPath[MAX_PATH];

            sprintf_s(srcPath, "%s/%s", sourceFolder, fileName);
            sprintf_s(destPath, "%s/%s", destFolder, fileName);

            MoveFileA(srcPath, destPath);
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}

int extract_zip_file(const char* zipFilename, const char* serviceType, const char* version) {
    if (!DirectoryExists(DIRECTORY_SERVICE)) {
        CreateDirectoryA(DIRECTORY_SERVICE, NULL);
    }

    char serveiceTypeDirectory[512];
    sprintf_s(serveiceTypeDirectory, sizeof(serveiceTypeDirectory), "%s/%s", DIRECTORY_SERVICE, serviceType);

    if (!DirectoryExists(serveiceTypeDirectory)) {
        CreateDirectoryA(serveiceTypeDirectory, NULL);
    }
    /////////////////
    char tempVersionDirectory[512];
    sprintf_s(tempVersionDirectory, "%s/temp_%s", serveiceTypeDirectory, version);
    if (DirectoryExists(tempVersionDirectory)) {
        LogAndMsgBox("Temporary directory exists, please manual delete this directory %s\r\n", tempVersionDirectory);
        return -3;
    }

    // Create temporary directory
    if (!CreateDirectoryA(tempVersionDirectory, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        LogAndMsgBox("Failed to create temporary directory %l\r\n", tempVersionDirectory);
        return -3;
    }
    /////////////////

    unzFile zfile = unzOpen(zipFilename);
    if (!zfile) {
        LogAndMsgBox("Cannot open ZIP file %s\r\n", zipFilename);
        return -1;
    }

    // version directory
    char versionDirectory[1024];
    sprintf_s(versionDirectory, sizeof(versionDirectory), "%s/%s/%s", DIRECTORY_SERVICE, serviceType, version);
    if (DirectoryExists(versionDirectory)) {
        unzClose(zfile);

        LogAndMsgBox("The corresponding version folder already exists: %s/%s/%s\r\n", DIRECTORY_SERVICE, serviceType, version);
        return -3;
    }

    char buffer[CHUNK_SIZE];

    unsigned int topLevelItemCount = 0;
    char firstTopLevelItem[256] = { '\0' };
    // First pass: Detect the structure of the ZIP
    if (unzGoToFirstFile(zfile) != UNZ_OK) {
        unzClose(zfile);

        LogAndMsgBox("Failed to find the first file in the ZIP %s\r\n", zipFilename);
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
        sprintf_s(fullPath, "%s/%s", tempVersionDirectory, filename_inzip);

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

    if (topLevelItemCount == 1) {
        // Rename topLevel directory
        char oldPath[1024];
        sprintf_s(oldPath, "%s/%s", tempVersionDirectory, firstTopLevelItem);
        rename(oldPath, versionDirectory);
    }
    else {
        // Move all projects to new folder
        CreateDirectoryA(versionDirectory, NULL);
        MoveFolderContents(tempVersionDirectory, versionDirectory);
    }

    // Delete temporary directory
    RemoveDirectoryA(tempVersionDirectory);

    return 0;
}
