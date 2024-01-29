#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <io.h>
#include "Log.h"
#include <sys/stat.h>
#include <iostream>

int replaceSubstring_s(char* source, size_t sourceSize, const char* target, const char* replacement) {
    char buffer[1024];
    char* insert_point = buffer;
    const char* tmp = source;
    size_t targetLen = strlen(target);
    size_t replacementLen = strlen(replacement);

    while (1) {
        const char* p = strstr(tmp, target);

        if (p == NULL) {
            strcpy_s(insert_point, sourceSize - (insert_point - buffer), tmp);
            break;
        }

        memcpy_s(insert_point, sourceSize - (insert_point - buffer), tmp, p - tmp);
        insert_point += p - tmp;

        memcpy_s(insert_point, sourceSize - (insert_point - buffer), replacement, replacementLen);
        insert_point += replacementLen;

        tmp = p + targetLen;
    }

    strcpy_s(source, sourceSize, buffer);
    return 0;
}

void replaceStringInFile(const char* filename, const char* old_content, const char* new_content) {
    char linedata[512] = { '\0' };
    FILE* fp;
    fopen_s(&fp, filename, "rb+N");

    FILE* tempFile;
    fopen_s(&tempFile, "tmp.txt", "wb+N");
    while (fgets(linedata, sizeof(linedata) - 1, fp))
    {
        char* ptr = strstr(linedata, old_content);
        if (ptr != NULL) {
            replaceSubstring_s(linedata, sizeof(linedata), old_content, new_content);
        }
        fputs(linedata, tempFile);
    }

    fclose(fp);
    fclose(tempFile);

    remove(filename);
    rename("tmp.txt", filename);
}
