#pragma once

bool IsHttpdMainProcessRunning(const wchar_t* processName);

BOOL ProcessIsRunning(LPCWSTR processName);

bool IsHttpdParentRunning(const wchar_t* processName);

DWORD isSelfChildProcessOfCurrent(const wchar_t* processName);

DWORD* GetAllChildProcesses(DWORD parentPid, int* count);

