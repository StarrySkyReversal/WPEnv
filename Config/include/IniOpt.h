#pragma once

int read_ini_file(const char* filename, const char* section, const char* key, char* value, size_t value_size);

int write_ini_file(const char* filename, const char* section, const char* key, const char* value);
