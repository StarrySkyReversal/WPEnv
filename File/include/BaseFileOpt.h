#pragma once


#define MAX_FILES 1000

typedef struct {
    wchar_t* filename[MAX_FILES];
    wchar_t* fileFullName[MAX_FILES];
    int count;
} FileList;


bool CheckDownloadFileExists(const wchar_t* fileFullName, FileList** outList);
void FreeCheckDownloadFileExists(FileList* fileList);

BOOL DirectoryExists(const wchar_t* dirName);
const wchar_t* GetFileFullNameFromUrl(const wchar_t* filename, const wchar_t* url);
void MergeFiles(const wchar_t* destination, const wchar_t* parts[], int num_parts);

void GetDirectoryFromPath(const wchar_t* fullPath, wchar_t* directory, size_t directorySize, bool getParent = false);

int ReadFileContent(const wchar_t* filePath, wchar_t** buffer, DWORD* size);
bool CheckFileExists(const wchar_t* filePath);
bool WriteStringToFileAsUTF8(const wchar_t* filePath, const wchar_t* content);
bool CreateNewFile(const wchar_t* filePath);

wchar_t** FindPartFileChunksWithPrefix(const wchar_t* directory, const wchar_t* targetPrefix, int* count);
void freeParFileChunkList(wchar_t** fileList, int count);

bool GetFileSize(const wchar_t* filepath, unsigned long long* size);

wchar_t* get_current_program_directory();

wchar_t* get_current_program_directory_with_forward_slash();

int copyFile(const char* sourcePath, const char* destPath);
