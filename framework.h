// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define DIRECTORY_SERVICE L"service"
#define DIRECTORY_CONFIG L"config"
#define DIRECTORY_DOWNLOAD L"downloads"
#define DIRECTORY_WEB L"www"
#define DIRECTORY_WEB_DEFAULT L"www/default"

#define FILE_CONFIG_SERVICE_SOURCE L"config/service_source.txt"
#define FILE_CONFIG_SERVICE_USE L"config/service_use.txt"
