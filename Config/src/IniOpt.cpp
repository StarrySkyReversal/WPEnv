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

// Used to check if a string is a given section
bool is_section(const char* line, const char* section) {
    char expected_line[MAX_LINE_LENGTH];
    snprintf(expected_line, MAX_LINE_LENGTH, "[%s]", section);
    return strncmp(line, expected_line, strlen(expected_line)) == 0;
}

int write_ini_file(const char* filename, const char* section, const char* key, const char* value) {
    FILE* file;
    char line[MAX_LINE_LENGTH];
    char new_content[MAX_LINE_LENGTH * 256] = "";
    bool section_found = false, key_found = false;
    errno_t err;

    // Opening a file in read mode
    err = fopen_s(&file, filename, "r+N");
    if (err == 0) {
        // Read existing content
        while (fgets(line, sizeof(line), file)) {
            if (!section_found) {
                if (is_section(line, section)) {
                    section_found = true;
                }
            }
            else if (line[0] == '[') {
                section_found = false;
            }

            if (section_found && strncmp(line, key, strlen(key)) == 0 && line[strlen(key)] == '=') {
                key_found = true;
                snprintf(line, sizeof(line), "%s=%s\n", key, value); // Replacement of key values
            }

            strcat_s(new_content, sizeof(new_content), line);
        }
        fclose(file);
    }

    // Contents of the modification
    if (!section_found) {
        snprintf(new_content + strlen(new_content), sizeof(new_content) - strlen(new_content), "[%s]\n", section);
    }
    if (!key_found) {
        snprintf(new_content + strlen(new_content), sizeof(new_content) - strlen(new_content), "%s=%s\n", key, value);
    }

    // Open the file in write mode and write the modified content
    err = fopen_s(&file, filename, "w+N");
    if (err != 0) {
        Log("Error opening file for writing");
        return -1;
    }

    fwrite(new_content, 1, strlen(new_content), file);
    fclose(file);

    return 0;
}
