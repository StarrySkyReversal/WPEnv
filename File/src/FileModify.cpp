#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <io.h>
#include "Log.h"

int modify_conf(const wchar_t* filename, const wchar_t* old_content, const wchar_t* new_content) {
    FILE* file;
    if (_wfopen_s(&file, filename, L"rb+") != 0) {
        wprintf(L"Error opening file: %ls\n", filename);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long long file_size = _ftelli64(file); // Change here to use long long for file_size
    fseek(file, 0, SEEK_SET);

    wchar_t* buffer = (wchar_t*)malloc((file_size + 1) * sizeof(wchar_t));
    if (buffer == NULL) {
        perror("Memory allocation error");
        fclose(file);
        return -1;
    }

    fread(buffer, sizeof(wchar_t), file_size / sizeof(wchar_t), file);
    buffer[file_size / sizeof(wchar_t)] = L'\0';

    wchar_t* found = wcsstr(buffer, old_content);

    if (found) {
        // If old_content is found, replace it with new_content
        long long offset = found - buffer;  // Change here to use long long for offset
        FILE* temp_file;
        if (tmpfile_s(&temp_file) != 0 || temp_file == NULL) {  // Use tmpfile_s
            perror("Error creating temp file");
            free(buffer);
            fclose(file);
            return -1;
        }

        fwrite(buffer, sizeof(wchar_t), offset, temp_file);
        fwrite(new_content, sizeof(wchar_t), wcslen(new_content), temp_file);
        fwrite(found + wcslen(old_content), sizeof(wchar_t), buffer + file_size / sizeof(wchar_t) - found - wcslen(old_content), temp_file);

        fseek(temp_file, 0, SEEK_SET);
        fseek(file, 0, SEEK_SET);
        wchar_t ch;
        while (fread(&ch, sizeof(wchar_t), 1, temp_file)) {
            fwrite(&ch, sizeof(wchar_t), 1, file);
        }

        fclose(temp_file);
    }
    else {
        // If old_content is not found, append new_content to the end of the file
        fseek(file, 0, SEEK_END);
        fwprintf(file, L"\n%ls", new_content);
    }

    free(buffer);
    fclose(file);
    return 0;
}

int modify_conf_utf8AndAscii(const char* filename, const char* old_content, const char* new_content, int replace_all = false) {
    FILE* file;
    if (fopen_s(&file, filename, "rb+") != 0 || file == NULL) {
        printf("Error opening file: %s\n", filename);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)_ftelli64(file);
    if (file_size < 0) {
        printf("Error getting file size.\n");
        fclose(file);
        return -1;
    }

    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Memory allocation error");
        fclose(file);
        return -1;
    }

    size_t bytesRead = fread_s(buffer, file_size, sizeof(char), file_size, file);
    if (bytesRead != file_size) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        return -1;
    }
    buffer[file_size] = '\0';

    FILE* temp_file;
    if (tmpfile_s(&temp_file) != 0 || temp_file == NULL) {
        perror("Error creating temp file");
        free(buffer);
        fclose(file);
        return -1;
    }

    char* found = buffer;
    char* start = buffer;
    while ((found = strstr(start, old_content)) != NULL) {
        fwrite(start, sizeof(char), found - start, temp_file);
        fwrite(new_content, sizeof(char), strlen(new_content), temp_file);
        start = found + strlen(old_content);

        if (!replace_all) {
            break;
        }
    }
    fwrite(start, sizeof(char), buffer + file_size - start, temp_file);

    fseek(temp_file, 0, SEEK_SET);
    fseek(file, 0, SEEK_SET);
    char ch;
    while (fread_s(&ch, sizeof(ch), 1, 1, temp_file) == 1) {
        fwrite(&ch, 1, 1, file);
    }

    fclose(temp_file);
    free(buffer);
    fclose(file);
    return 0;
}

int modify_conf_line_utf8AndAscii(const char* file_path, const char* search_str, const char* replace_str) {
    FILE* orig_file;
    FILE* temp_file;

    if (fopen_s(&orig_file, file_path, "rb") != 0) {
        return -1;  // Error opening the original file
    }

    if (tmpfile_s(&temp_file) != 0) {
        fclose(orig_file);
        return -2;  // Error creating a temporary file
    }

    char line[2048] = { '\0' };
    int found = 0;
    while (fgets(line, sizeof(line), orig_file)) {
        char* pos = strstr(line, search_str);
        if (pos) {
            fprintf_s(temp_file, "%s", replace_str);
            if (!strchr(replace_str, '\n')) {
                //fprintf_s(temp_file, "\n");
            }
            found = 1;
        }
        else {
            fprintf_s(temp_file, "%s", line);
        }
    }

    fclose(orig_file);

    if (!found) {
        fclose(temp_file);
        return 1;  // Search string not found
    }

    if (fopen_s(&orig_file, file_path, "wb") != 0) {
        fclose(temp_file);
        return -3;  // Error reopening the original file for writing
    }

    rewind(temp_file);
    while (fgets(line, sizeof(line), temp_file)) {
        fprintf_s(orig_file, "%s", line);
    }

    //fflush(orig_file);  // Flush the file buffer

    fclose(orig_file);
    fclose(temp_file);

    return 0;  // Success
}

int modify_conf_line_InsertOrUpdate_utf8AndAscii(const char* file_path, const char* search_str, const char* replace_str) {
    FILE* orig_file;
    FILE* temp_file;

    if (fopen_s(&orig_file, file_path, "rb") != 0) {
        return -1;  // Error opening the original file
    }

    if (tmpfile_s(&temp_file) != 0) {
        fclose(orig_file);
        return -2;  // Error creating a temporary file
    }

    char line[2048] = { '\0' };
    int found = 0;
    while (fgets(line, sizeof(line), orig_file)) {
        char* pos = strstr(line, search_str);
        if (pos) {
            fprintf_s(temp_file, "%s", replace_str);
            if (!strchr(replace_str, '\n')) {
                fprintf_s(temp_file, "\n");
            }
            found = 1;
        }
        else {
            fprintf_s(temp_file, "%s", line);
        }
    }

    if (!found) {
        fprintf_s(temp_file, "%s", replace_str);
        if (!strchr(replace_str, '\n')) {
            fprintf_s(temp_file, "\n");
        }
    }

    fclose(orig_file);

    if (fopen_s(&orig_file, file_path, "wb") != 0) {
        fclose(temp_file);
        return -3;  // Error reopening the original file for writing
    }

    rewind(temp_file);
    while (fgets(line, sizeof(line), temp_file)) {
        fprintf_s(orig_file, "%s", line);
    }

    fclose(orig_file);
    fclose(temp_file);

    return 0;  // Success
}

int modify_conf_line(const wchar_t* filename, const wchar_t* keyword, const wchar_t* new_content) {
    FILE* file;
    if (_wfopen_s(&file, filename, L"r+") != 0) {
        wprintf(L"Error opening file: %s\n", filename);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long long file_size = _ftelli64(file);
    fseek(file, 0, SEEK_SET);

    wchar_t* buffer = (wchar_t*)malloc((file_size + 1) * sizeof(wchar_t));
    if (buffer == NULL) {
        perror("Memory allocation error");
        fclose(file);
        return -1;
    }

    fread(buffer, sizeof(wchar_t), file_size / sizeof(wchar_t), file);
    buffer[file_size / sizeof(wchar_t)] = L'\0';

    wchar_t* found = wcsstr(buffer, keyword);

    if (found) {
        wchar_t* line_start = found;
        while (line_start > buffer && *(line_start - 1) != L'\n') {  // Look for the start of the line
            line_start--;
        }
        wchar_t* line_end = wcschr(found, L'\n');  // Find the end of the line
        if (!line_end) line_end = buffer + file_size / sizeof(wchar_t);  // If newline not found, point to the end of file

        long long offset_start = line_start - buffer;
        long long offset_end = line_end - buffer;

        FILE* temp_file;
        if (tmpfile_s(&temp_file) != 0 || temp_file == NULL) {
            perror("Error creating temp file");
            free(buffer);
            fclose(file);
            return -1;
        }

        fwrite(buffer, sizeof(wchar_t), offset_start, temp_file);
        fwrite(new_content, sizeof(wchar_t), wcslen(new_content), temp_file);
        fwrite(line_end, sizeof(wchar_t), (buffer + file_size / sizeof(wchar_t)) - line_end, temp_file);

        fseek(temp_file, 0, SEEK_SET);
        fseek(file, 0, SEEK_SET);
        wchar_t ch;
        while (fread(&ch, sizeof(wchar_t), 1, temp_file)) {
            fwrite(&ch, sizeof(wchar_t), 1, file);
        }

        fclose(temp_file);
    }
    else {
        wprintf(L"Keyword not found. No changes made.\n");
    }

    free(buffer);
    fclose(file);
    return 0;
}

// find target content add or new content
int modify_conf_InsertOrUpdate_utf8AndAscii(const char* filename, const char* old_content, const char* new_content) {
    FILE* file;
    fopen_s(&file, filename, "rb+");
    if (file == NULL) {

        perror("Error opening file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Memory allocation error");
        fclose(file);
        return -1;
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    char* found = strstr(buffer, old_content);

    if (found) {
        // If old_content is found, replace it with new_content
        ptrdiff_t offset = found - buffer;
        FILE* temp_file;
        tmpfile_s(&temp_file);
        if (temp_file == NULL) {
            perror("Error creating temp file");
            free(buffer);
            fclose(file);
            return -1;
        }

        fwrite(buffer, 1, offset, temp_file);                     // write content before old_content
        fwrite(new_content, 1, strlen(new_content), temp_file);   // write new_content
        fwrite(found + strlen(old_content), 1, buffer + file_size - found - strlen(old_content), temp_file); // write content after old_content

        fseek(temp_file, 0, SEEK_SET);
        fseek(file, 0, SEEK_SET);
        char ch;
        while ((ch = fgetc(temp_file)) != EOF) {
            fputc(ch, file);
        }

        fclose(temp_file);
    }
    else {
        // If old_content is not found, append new_content to the end of the file
        fseek(file, 0, SEEK_END);
        fprintf(file, "\n%s", new_content);
    }

    free(buffer);
    fclose(file);
    return 0;
}

int comment_block(const char* filename, const char* beginLine, const char* endLine) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "rb+");
    if (err != 0 || file == NULL) {
        Log("Error opening file: %s\n", filename);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long long file_size = _ftelli64(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        Log("Memory allocation error");
        fclose(file);
        return -1;
    }

    size_t count_read = fread_s(buffer, file_size, 1, file_size, file);
    buffer[count_read] = '\0';

    if (count_read != file_size) {
        Log("Error reading file");
        free(buffer);
        fclose(file);
        return -1;
    }

    long long additional_space = 0;
    char* start = strstr(buffer, beginLine);
    while (start) {
        char* end = strstr(start, endLine);
        if (!end) {
            break;
        }

        char* current = start;
        while (current <= end + strlen(endLine) - 1) {
            if (*current == '\n') {
                additional_space++; // for each newline
            }
            current++;
        }
        additional_space += (end - start); // for characters between beginLine and endLine
        start = strstr(current, beginLine);
    }

    char* new_buffer = (char*)malloc(file_size + additional_space + 1);
    if (!new_buffer) {
        Log("Memory allocation error for new buffer");
        free(buffer);
        fclose(file);
        return -1;
    }
    char* ptr_new_buffer = new_buffer;
    start = buffer;

    while (*start) {
        if (strstr(start, beginLine) == start) {
            char* end = strstr(start, endLine);
            if (!end) {
                strcpy_s(ptr_new_buffer, file_size - (start - buffer) + 1, start);
                break;
            }

            *ptr_new_buffer++ = '#';
            while (start <= end + strlen(endLine) - 1) {
                if (*start == '\n' && start < end) {
                    *ptr_new_buffer++ = *start++;
                    *ptr_new_buffer++ = '#';
                }
                else {
                    *ptr_new_buffer++ = *start++;
                }
            }
        }
        else {
            *ptr_new_buffer++ = *start++;
        }
    }
    *ptr_new_buffer = '\0';

    fseek(file, 0, SEEK_SET);
    fwrite(new_buffer, 1, strlen(new_buffer), file);
    //fflush(file);

    free(buffer);
    free(new_buffer);
    fclose(file);
    return 0;
}

#define BUFFER_SIZE 4096
void remove_comment(const char* file_path, const char* start_marker, const char* end_marker) {
    FILE* file;
    errno_t err = fopen_s(&file, file_path, "rb+");
    if (err != 0) {
        perror("Error opening file");
        return;
    }

    // Read the entire file into memory
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* string = (char*)malloc(fsize + 1);
    if (string == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return;
    }

    fread(string, 1, fsize, file);
    string[fsize] = '\0';

    char* output = (char*)malloc(fsize + 1);
    if (output == NULL) {
        perror("Memory allocation failed");
        free(string);
        fclose(file);
        return;
    }

    char* out_ptr = output;
    char* search_start = string;
    int in_comment_block = 0;
    size_t start_marker_len = strlen(start_marker);
    size_t end_marker_len = strlen(end_marker);

    while (*search_start != '\0') {
        // Check for start marker
        if (!in_comment_block && strncmp(search_start, start_marker, start_marker_len) == 0) {
            in_comment_block = 1;
            // Copy and remove '#' up to start marker to output
            while (search_start < string + fsize && *search_start != '\n') {
                if (*search_start == '#') {
                    search_start++; // Skip the '#' character
                }
                *out_ptr++ = *search_start ? *search_start++ : '\0';
            }
            continue;
        }

        // Check for end marker
        if (in_comment_block && strncmp(search_start, end_marker, end_marker_len) == 0) {
            in_comment_block = 0;
            // Copy and remove '#' up to end marker to output
            while (search_start < string + fsize && *search_start != '\n') {
                if (*search_start == '#') {
                    search_start++; // Skip the '#' character
                }
                *out_ptr++ = *search_start ? *search_start++ : '\0';
            }
            continue;
        }

        // Remove '#' at the start of lines in comment block
        if (in_comment_block && *search_start == '#') {
            search_start++; // Skip the '#' character
        }

        *out_ptr++ = *search_start ? *search_start++ : '\0';
    }
    *out_ptr = '\0'; // Null-terminate the output string

    // Write the modified content back to the file
    fseek(file, 0, SEEK_SET);
    fwrite(output, 1, strlen(output), file);
    fflush(file); // Flush the file stream
    _chsize_s(_fileno(file), strlen(output)); // Truncate the file to the new size
    fclose(file);

    free(string);
    free(output);
}

bool find_string_in_file_s(const char* filepath, const char* search_string) {
    char buffer[1024];
    FILE* file;
    errno_t err = fopen_s(&file, filepath, "r");
    if (err != 0) {
        perror("Failed to open file");
        return false;
    }

    while (fgets(buffer, sizeof(buffer), file)) {
        if (strstr(buffer, search_string)) {
            fclose(file);
            return true;
        }
    }
    fclose(file);
    return false;
}

bool find_wstring_in_file_s(const wchar_t* filepath, const wchar_t* search_string) {
    wchar_t buffer[1024];
    FILE* file;
    errno_t err = _wfopen_s(&file, filepath, L"r,ccs=UNICODE");
    if (err != 0) {
        _wperror(L"Failed to open file");
        return false;
    }

    while (fgetws(buffer, sizeof(buffer) / sizeof(wchar_t), file)) {
        if (wcsstr(buffer, search_string)) {
            fclose(file);
            return true;
        }
    }
    fclose(file);
    return false;
}

bool contains_pattern(const char* filepath, const char* pattern_begin, const char* pattern_end) {
    char buffer[1024];
    FILE* file;
    errno_t err = fopen_s(&file, filepath, "r");
    if (err != 0) {
        perror("Failed to open file");
        return false;
    }

    while (fgets(buffer, sizeof(buffer), file)) {
        char* begin_ptr = strstr(buffer, pattern_begin);
        if (begin_ptr) {
            char* end_ptr = strstr(begin_ptr + strlen(pattern_begin), pattern_end);
            if (end_ptr) {
                fclose(file);
                return true;
            }
        }
    }
    fclose(file);
    return false;
}

bool contains_pattern_w(const wchar_t* filepath, const wchar_t* pattern_begin, const wchar_t* pattern_end) {
    wchar_t buffer[1024];
    FILE* file;
    errno_t err = _wfopen_s(&file, filepath, L"r,ccs=UNICODE");
    if (err != 0) {
        _wperror(L"Failed to open file");
        return false;
    }

    while (fgetws(buffer, sizeof(buffer) / sizeof(wchar_t), file)) {
        wchar_t* begin_ptr = wcsstr(buffer, pattern_begin);
        if (begin_ptr) {
            wchar_t* end_ptr = wcsstr(begin_ptr + wcslen(pattern_begin), pattern_end);
            if (end_ptr) {
                fclose(file);
                return true;
            }
        }
    }
    fclose(file);
    return false;
}
