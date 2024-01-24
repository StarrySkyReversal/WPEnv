#pragma once

bool IsHttpdMainProcessRunning();

BOOL ProcessIsRunning(const char* processName);

bool IsHttpdParentRunning(const char* processName);

DWORD isSelfChildProcessOfCurrent(const char* processName);
