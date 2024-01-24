#pragma once

typedef struct {
    char** paths;
    size_t count;
} PathList;

PathList* initPathList();
void freePathList(PathList* list);
void findFilesInDirectory(const char* directory, const char* targetFile, PathList* foundPaths);
