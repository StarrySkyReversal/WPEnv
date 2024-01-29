//#include "framework.h"
//#include <stdio.h>
//#include <stdbool.h>
//#include <string.h>
//#include <errno.h>
//
//
//typedef enum {
//    CHAR_NODE,
//    ANY,
//    STAR,
//    END
//} NodeType;
//
//typedef struct Node {
//    NodeType type;
//    char ch;
//    struct Node* next;
//} Node;
//
//Node* compile_pattern(const char* pattern) {
//    Node* start = NULL, * current = NULL;
//    while (*pattern) {
//        Node* newNode = (Node*)malloc(sizeof(Node));
//        newNode->next = NULL;
//
//        if (*pattern == '*') {
//            newNode->type = STAR;
//        }
//        else if (*pattern == '?') {
//            newNode->type = ANY;
//        }
//        else {
//            newNode->type = CHAR_NODE;
//            newNode->ch = *pattern;
//        }
//
//        if (!start) {
//            start = newNode;
//            current = newNode;
//        }
//        else {
//            current->next = newNode;
//            current = newNode;
//        }
//        pattern++;
//    }
//
//    Node* endNode = (Node*)malloc(sizeof(Node));
//    endNode->type = END;
//    endNode->next = NULL;
//    current->next = endNode;
//
//    return start;
//}
//
//void free_pattern(Node* pattern) {
//    Node* current = pattern;
//    while (current) {
//        Node* next = current->next;
//        free(current);
//        current = next;
//    }
//}
//
//bool is_match(Node* pattern, const char* str, const char** start, const char** end) {
//    if (pattern->type == END) {
//        *end = str;
//        return true;
//    }
//
//    if (pattern->type == CHAR_NODE) {
//        if (pattern->ch == *str) {
//            return is_match(pattern->next, str + 1, start, end);
//        }
//    }
//    else if (pattern->type == ANY) {
//        // Count consecutive ANY nodes
//        int countAny = 0;
//        Node* tempPattern = pattern;
//        while (tempPattern && tempPattern->type == ANY) {
//            countAny++;
//            tempPattern = tempPattern->next;
//        }
//
//        // Try to match up to countAny characters
//        for (int i = 0; i <= countAny; i++) {
//            if (is_match(tempPattern, str + i, start, end)) {
//                return true;
//            }
//        }
//    }
//    else if (pattern->type == STAR) {
//        do {
//            if (is_match(pattern->next, str, start, end)) {
//                return true;
//            }
//        } while (*str++);
//    }
//
//    return false;
//}
//
//bool search_file_for_pattern(const char* filepath, const char* pattern, char* out, size_t out_size) {
//    char buffer[1024];
//    FILE* file;
//    errno_t err = fopen_s(&file, filepath, "r+N");
//    if (err != 0) {
//        perror("Failed to open file");
//        return false;
//    }
//
//    Node* compiledPattern = compile_pattern(pattern);
//    while (fgets(buffer, sizeof(buffer), file)) {
//        const char* ptr = buffer;
//        while (*ptr) {
//            const char* start = ptr;
//            const char* end;
//            if (is_match(compiledPattern, ptr, &start, &end)) {
//                size_t len = end - start;
//                strncpy_s(out, out_size, start, len < out_size ? len : out_size - 1);
//                out[len] = '\0';  // Null-terminate the result
//                free_pattern(compiledPattern);
//                fclose(file);
//                return true;
//            }
//            ptr++;
//        }
//    }
//
//    free_pattern(compiledPattern);
//    fclose(file);
//    return false;
//}
