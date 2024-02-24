#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "Log.h"
#include "IniOpt.h"

#define MAX_LINE_LENGTH 512


int read_ini_file(const char* filename, const char* section, const char* key, char* value, size_t value_size) {
    char line[MAX_LINE_LENGTH];
    char current_section[MAX_LINE_LENGTH] = "";
    FILE* file;
    fopen_s(&file, filename, "r+N");

    if (!file) {
        return -1;
    }

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == ';' || line[0] == '#') {
            continue; // comment line
        }

        if (line[0] == '[') {
            // Beginning of a new section
            sscanf_s(line, "[%[^]]", current_section, (unsigned)_countof(current_section));
            continue;
        }

        if (strcmp(current_section, section) == 0) {
            char key_buffer[MAX_LINE_LENGTH], value_buffer[MAX_LINE_LENGTH];
            if (sscanf_s(line, "%[^=]=%s", key_buffer, (unsigned)_countof(key_buffer), value_buffer, (unsigned)_countof(value_buffer)) == 2) {
                if (strcmp(key_buffer, key) == 0) {
                    strcpy_s(value, value_size, value_buffer);
                    fclose(file);
                    return 0; // Successfully found key
                }
            }
        }
    }

    fclose(file);
    return -1; // Key not found
}

bool checkKeyValueExists(const char* filename, const char* section, const char* key, const char* value) {

    //read_ini_file(const char* filename, const char* section, const char* key, char* value, size_t value_size);
    char temp[256];
    read_ini_file(filename, section, key, temp, sizeof(temp));

    return strcmp(temp, value) == 0;
}

// Used to check if a string is a given section
bool is_section(const char* line, const char* section) {
    char expected_line[MAX_LINE_LENGTH];
    snprintf(expected_line, MAX_LINE_LENGTH, "[%s]", section);
    return strncmp(line, expected_line, strlen(expected_line)) == 0;
}

int write_ini_file(const char* filename, const char* section, const char* key, const char* value) {
    const int MAX_LINE = 1024;
    char tempFilename[] = "temp.ini";
    char line[MAX_LINE];
    int sectionFound = 0, keyFound = 0;
    FILE* file;
    fopen_s(&file, filename, "r");
    FILE* tempFile;
    fopen_s(&tempFile, tempFilename, "w");

    if (!file) {
        fopen_s(&file, filename, "w+");
        if (!file) {
            if (tempFile) fclose(tempFile);
            return -1;
        }

        // The file has just been created, so the section and key must not exist, so add them directly.
        fprintf(file, "[%s]\n%s=%s\n", section, key, value);
        fclose(file);
        if (tempFile) fclose(tempFile);
        return 0;
    }

    if (!tempFile) {
        fprintf(stderr, "Error creating temporary file.\n");
        fclose(file);
        return -1;
    }

    char sectionLine[MAX_LINE];
    snprintf(sectionLine, sizeof(sectionLine), "[%s]", section);
    char keyLine[MAX_LINE];
    snprintf(keyLine, sizeof(keyLine), "%s=", key);

    while (fgets(line, MAX_LINE, file) != NULL) {
        if (!sectionFound) {
            // find a section
            if (strncmp(line, sectionLine, strlen(sectionLine)) == 0) {
                sectionFound = 1;
                fprintf(tempFile, "%s", line); // Write to section
                continue;
            }
        }
        else if (!keyFound) {
            // Check if you have reached the next section or the end of the file
            if (line[0] == '[' || feof(file)) {
                if (!keyFound && value != NULL) {
                    // Add key=value at the end of the current section
                    fprintf(tempFile, "%s=%s\n", key, value);
                    keyFound = 1; // Prevent duplicate additions
                }
                if (line[0] == '[') {
                    fprintf(tempFile, "%s", line); // Write to the next section of the line
                }
                continue;
            }
            else if (strncmp(line, keyLine, strlen(keyLine)) == 0) {
                keyFound = 1; // Find the key.
                if (value) {
                    fprintf(tempFile, "%s=%s\n", key, value);
                    continue;
                }
            }
        }
        fprintf(tempFile, "%s", line);
    }

    // If it's at the end of the file and neither section nor key is found, add them.
    if (!sectionFound && value) {
        fprintf(tempFile, "[%s]\n%s=%s\n", section, key, value);
    }
    else if (sectionFound && !keyFound && value) {
        fprintf(tempFile, "%s=%s\n", key, value);
    }

    fclose(file);
    fclose(tempFile);

    remove(filename);
    rename(tempFilename, filename);

    return 0;
}
