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

#define DIRECTORY_SERVICE "service"
#define DIRECTORY_CONFIG "config"
#define DIRECTORY_DOWNLOAD "downloads"
#define DIRECTORY_WEB "www"
#define DIRECTORY_WEB_DEFAULT "www/default"

#define FILE_CONFIG_SERVICE_SOURCE "config/service_source.txt"
#define FILE_CONFIG_SERVICE_USE "config/service_use.txt"
