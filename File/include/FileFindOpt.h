#pragma once

typedef struct {
    wchar_t** paths;
    size_t count;
} PathList;

PathList* initPathList();
void freePathList(PathList* list);
void findFilesInDirectory(const wchar_t* directory, const wchar_t* targetFile, PathList* foundPaths);
