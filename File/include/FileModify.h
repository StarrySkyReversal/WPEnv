#pragma once

int modify_conf(const wchar_t* filename, const wchar_t* old_content, const wchar_t* new_content);

int modify_conf_utf8AndAscii(const char* filename, const char* old_content, const char* new_content);

int modify_conf_InsertOrUpdate_utf8AndAscii(const char* filename, const char* old_content, const char* new_content);

int modify_conf_line(const wchar_t* filename, const wchar_t* keyword, const wchar_t* new_content);

int modify_conf_line_utf8AndAscii(const char* file_path, const char* search_str, const char* replace_str);

int modify_conf_line_InsertOrUpdate_utf8AndAscii(const char* file_path, const char* search_str, const char* replace_str);

int comment_block(const char* filename, const char* beginLine, const char* endLine);

bool find_string_in_file_s(const char* filepath, const char* search_string);

bool find_wstring_in_file_s(const wchar_t* filepath, const wchar_t* search_string);

bool contains_pattern(const char* filepath, const char* pattern_begin, const char* pattern_end);

bool contains_pattern_w(const wchar_t* filepath, const wchar_t* pattern_begin, const wchar_t* pattern_end);

