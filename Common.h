#pragma once

enum CharType {
    ANSI,
    WIDE,
    UNKNOWN
};

bool WToM(const wchar_t* wideStr, char* outputBuffer, int bufferSize);
bool MToW(const char* multiByteStr, wchar_t* outputBuffer, int bufferSize);

void InitRandomSeed();
int RandomInRange(int min, int max);

SIZE_T GetCurrentProcessMemoryUsage();
