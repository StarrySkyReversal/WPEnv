#pragma once


#define MAX_FILES 1000

typedef struct {
    char* filename[MAX_FILES];
    char* fileFullName[MAX_FILES];
    int count;
} FileList;


bool CheckDownloadFileExists(const char* fileFullName, FileList** outList);
void FreeCheckDownloadFileExists(FileList* fileList);

BOOL DirectoryExists(const char* dirName);
const char* GetFileFullNameFromUrl(const char* filename, const char* url);
void MergeFiles(const char* destination, const char* parts[], int num_parts);

void GetDirectoryFromPath(const char* fullPath, char* directory, size_t directorySize, bool getParent = false);

int ReadFileContent(const char* filePath, char** buffer, DWORD* size);
bool CheckFileExists(const char* filePath);
bool WriteStringToFileAsUTF8(const char* filePath, const char* content);
bool CreateNewFile(const char* filePath);

char** FindPartFileChunksWithPrefix(const char* directory, const char* targetPrefix, int* count);
void freeParFileChunkList(char** fileList, int count);

bool GetFileSize(const char* filepath, unsigned long long* size);

char* get_current_program_directory();

char* get_current_program_directory_with_forward_slash();

int copyFile(const char* sourcePath, const char* destPath);
