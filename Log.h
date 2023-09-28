#pragma once

#include <windows.h>
#include <cstdarg>
#include <cstdio>

extern HWND hWndMain;

template<typename CharType>
struct LoggerHelper;

template<>
struct LoggerHelper<char> {
    static const size_t BufferSize = 2048;
    using Char = char;

    static void FormatString(Char* buffer, size_t bufferSize, const Char* format, va_list args) {
        vsprintf_s(buffer, bufferSize, format, args);
    }

    static void Output(const Char* str) {
        OutputDebugStringA(str);
    }

    static void MessageBox(const Char* str) {
        ::MessageBoxA(hWndMain, str, "Log Message", MB_OK);
    }

    static void ToFile(const Char* str, const char* filename) {
        FILE* pFile;
        fopen_s(&pFile, filename, "a");
        if (pFile) {
            size_t len = strlen(str);
            if ((len >= 2 && strcmp(str + len - 2, "\r\n") == 0) || (len >= 1 && str[len - 1] == '\n'))
                fprintf(pFile, "%s", str);
            else
                fprintf(pFile, "%s\n", str);
            fclose(pFile);
        }
    }
};

template<>
struct LoggerHelper<wchar_t> {
    static const size_t BufferSize = 2048;
    using Char = wchar_t;

    static void FormatString(Char* buffer, size_t bufferSize, const Char* format, va_list args) {
        vswprintf_s(buffer, bufferSize, format, args);
    }

    static void Output(const Char* str) {
        OutputDebugStringW(str);
    }

    static void MessageBox(const Char* str) {
        ::MessageBoxW(hWndMain, str, L"Log Message", MB_OK);
    }

    static void ToFile(const Char* str, const wchar_t* filename) {
        FILE* pFile;
        _wfopen_s(&pFile, filename, L"a");
        if (pFile) {
            size_t len = wcslen(str);
            if ((len >= 2 && wcscmp(str + len - 2, L"\r\n") == 0) || (len >= 1 && str[len - 1] == L'\n'))
                fwprintf(pFile, L"%s", str);
            else
                fwprintf(pFile, L"%s\n", str);
            fclose(pFile);
        }
    }
};

template<typename CharType>
void Log(const CharType* format, ...) {
    CharType buffer[LoggerHelper<CharType>::BufferSize];
    va_list args;
    va_start(args, format);
    LoggerHelper<CharType>::FormatString(buffer, LoggerHelper<CharType>::BufferSize, format, args);
    LoggerHelper<CharType>::Output(buffer);

    va_end(args);
}

template<typename CharType>
CharType* GetFormatString(const CharType* format, ...) {
    // Thread Local Storage (TLS): If your application is multithreaded, static local variables might pose issues, 
    // as multiple threads might attempt to access it simultaneously. In such cases, you can consider using Thread Local Storage.
    thread_local CharType buffer[LoggerHelper<CharType>::BufferSize];
    va_list args;
    va_start(args, format);
    LoggerHelper<CharType>::FormatString(buffer, LoggerHelper<CharType>::BufferSize, format, args);
    va_end(args);

    return buffer;
}

template<typename CharType>
void LogAndMsgBox(const CharType* format, ...) {
    CharType buffer[LoggerHelper<CharType>::BufferSize];
    va_list args;
    va_start(args, format);
    LoggerHelper<CharType>::FormatString(buffer, LoggerHelper<CharType>::BufferSize, format, args);

    LoggerHelper<CharType>::Output(buffer);
    LoggerHelper<CharType>::MessageBox(buffer);

    va_end(args);
}

template<typename CharType>
void LogAndFile(const CharType* filename, const CharType* format, ...) {
    CharType buffer[LoggerHelper<CharType>::BufferSize];
    va_list args;
    va_start(args, format);
    LoggerHelper<CharType>::FormatString(buffer, LoggerHelper<CharType>::BufferSize, format, args);

    LoggerHelper<CharType>::Output(buffer);
    LoggerHelper<CharType>::ToFile(buffer, filename);

    va_end(args);
}

