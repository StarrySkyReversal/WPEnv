#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include "framework.h"
#include "Log.h"
#include "BaseFileOpt.h"
#include "FileModify.h"
#include "ServiceUse.h"
#include "ServiceSource.h"
#include "SyncServiceConfig.h"
#include "FileFindOpt.h"
#include "Compression.h"
#include <errno.h>
#include "DownloadThread.h"


DWORD phpApacheDll(const char* phpVersion, const char* serviceVersionDir, char* result, size_t bufferSize) {
	char dllFilePath[512];

	if (
		strcmp(phpVersion, "php-8.3.0") == 0 ||
		strcmp(phpVersion, "php-8.2.0") == 0 ||
		strcmp(phpVersion, "php-8.1.0") == 0 ||
		strcmp(phpVersion, "php-8.0.0") == 0
		) {
		sprintf_s(dllFilePath, sizeof(dllFilePath), "%s/php8apache2_4.dll", serviceVersionDir);
		if (_access_s(dllFilePath, 0) != 0) {
			MessageBoxA(hWndMain, "Please select from the list and download the PHP version with the 'ts' (Thread Safe) flag to get the missing 'php8apache2_4.dll' file.", "WPEnv", MB_ICONINFORMATION);
			return -1;
		}
		sprintf_s(result, bufferSize, "LoadModule php_module \"%s\"", dllFilePath);
	}
	else if (
		strcmp(phpVersion, "php-7.3.0") == 0 ||
		strcmp(phpVersion, "php-7.2.0") == 0 ||
		strcmp(phpVersion, "php-7.1.0") == 0 ||
		strcmp(phpVersion, "php-7.0.0") == 0
		) {
		sprintf_s(dllFilePath, sizeof(dllFilePath), "%s/php7apache2_4.dll", serviceVersionDir);
		if (_access_s(dllFilePath, 0) != 0) {
			MessageBoxA(hWndMain, "Please select from the list and download the PHP version with the 'ts' (Thread Safe) flag to get the missing 'php7apache2_4.dll' file.", "WPEnv", MB_ICONINFORMATION);
			return -1;
		}
		sprintf_s(result, bufferSize, "LoadModule php7_module \"%s\"", dllFilePath);
	}
	else if (
		strcmp(phpVersion, "php-5.6.0") == 0 ||
		strcmp(phpVersion, "php-5.5.0") == 0
		) {
		sprintf_s(dllFilePath, sizeof(dllFilePath), "%s/php5apache2_4.dll", serviceVersionDir);
		if (_access_s(dllFilePath, 0) != 0) {
			MessageBoxA(hWndMain, "Please select from the list and download the PHP version with the 'ts' (Thread Safe) flag to get the missing 'php5apache2_4.dll' file.", "WPEnv", MB_ICONINFORMATION);
			return -1;
		}
		sprintf_s(result, bufferSize, "LoadModule php5_module \"%s\"", dllFilePath);
	}
	else {
		strncpy_s(result, bufferSize, "", _TRUNCATE);

		MessageBoxA(hWndMain, "The httpd service did not find the corresponding php version.", "WPEnv", MB_ICONINFORMATION);
		return -1;
	}

	return 0;
}

DWORD versionMatch(const char* serviceType, const char* fileTagVersion, const char* targetFilepath, SoftwareGroupInfo softwareGroupInfo) {
	char* wProgramDirectory = get_current_program_directory_with_forward_slash();

	// php.ini
	// nginx.conf vhosts/default.conf
	// httpd.conf vhosts/default.conf
	// mysql my.ini current is not exists

	FILE *sourceFile, *newFile;

	char sourceFilename[256];
	sprintf_s(sourceFilename, sizeof(sourceFilename), "%s/repository/%s/%s.txt", wProgramDirectory, serviceType, fileTagVersion);
	if (_access_s(sourceFilename, 0) != 0) {
		MessageBoxA(hWndMain, "The repository txt file does not exist, please rebuild the project.", "WPEnv", MB_ICONINFORMATION);
		return -1;
	}
	errno_t err;

	char msgInfo[512];

	err = fopen_s(&sourceFile, sourceFilename, "rb+N");
	if (err != 0) {
		sprintf_s(msgInfo, sizeof(msgInfo), "Failed to open repository file, please check if you have appropriate permissions. path:%s", sourceFilename);
		MessageBoxA(hWndMain, msgInfo, "WPEnv", MB_ICONINFORMATION);
		return -1;
	}

	err = fopen_s(&newFile, targetFilepath, "wb+N");
	if (err != 0) {
		fclose(sourceFile);

		sprintf_s(msgInfo, sizeof(msgInfo), "Target file does not exist, please download it first. path:%s", fileTagVersion);
		MessageBoxA(hWndMain, msgInfo, "WPEnv", MB_ICONINFORMATION);
		return -1;
	}

	char buffer[1024];
	size_t bytesRead = 0;
	while ((bytesRead = fread(buffer, 1, 1024, sourceFile)) > 0) {
		fwrite(buffer, 1, bytesRead, newFile);
	}
	
	fclose(sourceFile);
	fclose(newFile);

	char wwwDir[256];
	sprintf_s(wwwDir, sizeof(wwwDir), "%s/www", wProgramDirectory);

	char wwwDefaultDir[256];
	sprintf_s(wwwDefaultDir, sizeof(wwwDefaultDir), "%s/www/default", wProgramDirectory);

	char wwwLogsDir[256];
	sprintf_s(wwwLogsDir, sizeof(wwwLogsDir), "%s/www_logs", wProgramDirectory);

	if (strcmp(serviceType, "php") == 0) {
		char phpExtensionDir[512];
		sprintf_s(phpExtensionDir, sizeof(phpExtensionDir), "%s/service/php/%s/ext", wProgramDirectory, softwareGroupInfo.php.version);

		replaceStringInFile(targetFilepath, "{{TPL_VAR:PHPExtensionDir}}", phpExtensionDir);
	} else if (strcmp(serviceType, "apache") == 0) {
		replaceStringInFile(targetFilepath, "{{TPL_VAR:DocumentRoot}}", wwwDefaultDir);
		replaceStringInFile(targetFilepath, "{{TPL_VAR:ServerName}}", "localhost:80");
		replaceStringInFile(targetFilepath, "{{TPL_VAR:wwwLogs}}", wwwLogsDir);

		if (fileTagVersion == "vhosts") {
			replaceStringInFile(targetFilepath, "{{TPL_VAR:wwwRoot}}", wwwDir);
		}
		else {
			char apacheBaseDir[256];
			
			getApacheVersionAbsBaseDir(softwareGroupInfo.apache.version, apacheBaseDir, sizeof(apacheBaseDir));

			char phpDir[512];
			sprintf_s(phpDir, sizeof(phpDir), "%s/service/php/%s", wProgramDirectory, softwareGroupInfo.php.version);

			replaceStringInFile(targetFilepath, "{{TPL_VAR:SRVROOT}}", apacheBaseDir);

			replaceStringInFile(targetFilepath, "{{TPL_VAR:addType}}", "AddType application/x-httpd-php .php .html");

			char phpIniDir[512];
			sprintf_s(phpIniDir, sizeof(phpIniDir), "phpIniDir \"%s\"", phpDir);

			replaceStringInFile(targetFilepath, "{{TPL_VAR:phpIniDir}}", phpIniDir);

			char dllStr[512];
			if (phpApacheDll(softwareGroupInfo.php.versionNumber, phpDir, dllStr, sizeof(dllStr)) != 0) {
				return -1;
			}

			// The curl dependency dll for php 7.3 cannot be actively loaded
			if (strcmp(softwareGroupInfo.php.versionNumber, "php-7.3.0") == 0) {
				char libcryptox64_File[512];
				char libsslx64_File[512];
				char libssh2_File[512];
				char loadFileStr[2048];
				sprintf_s(libcryptox64_File, sizeof(libcryptox64_File), "%s/libcrypto-1_1-x64.dll", phpDir);
				sprintf_s(libsslx64_File, sizeof(libsslx64_File), "%s/libssl-1_1-x64.dll", phpDir);
				sprintf_s(libssh2_File, sizeof(libssh2_File), "%s/libssh2.dll", phpDir);
				sprintf_s(loadFileStr, sizeof(loadFileStr), "LoadFile \"%s\" \"%s\" \"%s\"", libcryptox64_File, libsslx64_File, libssh2_File);

				replaceStringInFile(targetFilepath, "{{TPL_VAR:LoadFile}}", loadFileStr);
			} else if (strcmp(softwareGroupInfo.php.versionNumber, "php-5.5.0") == 0 ||
				strcmp(softwareGroupInfo.php.versionNumber, "php-5.6.0") == 0) {
				char libeay32_File[512];
				char ssleay32_File[512];
				char loadFileStr[2048];
				sprintf_s(libeay32_File, sizeof(libeay32_File), "%s/libeay32.dll", phpDir);
				sprintf_s(ssleay32_File, sizeof(ssleay32_File), "%s/ssleay32.dll", phpDir);
				sprintf_s(loadFileStr, sizeof(loadFileStr), "LoadFile \"%s\" \"%s\"", libeay32_File, ssleay32_File);

				replaceStringInFile(targetFilepath, "{{TPL_VAR:LoadFile}}", loadFileStr);
			}
			else {
				replaceStringInFile(targetFilepath, "{{TPL_VAR:LoadFile}}", "");
			}

			replaceStringInFile(targetFilepath, "{{TPL_VAR:phpLoadModule}}", dllStr);
		}
	}
	else if (strcmp(serviceType, "nginx") == 0) {
		replaceStringInFile(targetFilepath, "{{TPL_VAR:DocumentRoot}}", wwwDefaultDir);
		replaceStringInFile(targetFilepath, "{{TPL_VAR:ServerName}}", "localhost");
		replaceStringInFile(targetFilepath, "{{TPL_VAR:wwwLogs}}", wwwLogsDir);
	}

	return 0;
}

void InstallDefaultService(char* wProgramDirectory) {
	// Determine if the default package is unpacked
	char apacheDir[512];
	sprintf_s(apacheDir, sizeof(apacheDir), "%s/service/apache", wProgramDirectory);
	char nginxDir[512];
	sprintf_s(nginxDir, sizeof(nginxDir), "%s/service/nginx", wProgramDirectory);

	char defaultApacheVersionDir_x64[512];
	char defaultApacheVersionDir_x86[512];
	char defaultNginxVersionDir[512];
	sprintf_s(defaultApacheVersionDir_x64, sizeof(defaultApacheVersionDir_x64), "%s/httpd-2.4.58_vs17-x64", apacheDir);
	sprintf_s(defaultApacheVersionDir_x86, sizeof(defaultApacheVersionDir_x86), "%s/httpd-2.4.58_vs17-x86", apacheDir);
	sprintf_s(defaultNginxVersionDir, sizeof(defaultNginxVersionDir), "%s/nginx-1.24.0", nginxDir);

	char defaultApacheFile_x64[512];
	char defaultApacheFile_x86[512];
	char defaultNginxFile[512];
	sprintf_s(defaultApacheFile_x64, sizeof(defaultApacheFile_x64), "%s/downloads/httpd-2.4.58_vs17-x64.zip", wProgramDirectory);
	sprintf_s(defaultApacheFile_x86, sizeof(defaultApacheFile_x86), "%s/downloads/httpd-2.4.58_vs17-x86.zip", wProgramDirectory);
	sprintf_s(defaultNginxFile, sizeof(defaultNginxFile), "%s/downloads/nginx-1.24.0.zip", wProgramDirectory);

	if (_access(defaultApacheFile_x64, 0) == 0) {
		if (!DirectoryExists(defaultApacheVersionDir_x64)) {
			SoftwareInfo pSoftwareInfo;
			pSoftwareInfo.fileFullName = "httpd-2.4.58_vs17-x64.zip";
			pSoftwareInfo.serviceType = "apache";
			pSoftwareInfo.version = "httpd-2.4.58_vs17-x64";
			pSoftwareInfo.versionNumber = "httpd-2.4.58";
			pSoftwareInfo.link = "";
			UnzipFile(&pSoftwareInfo);
		}
	}

	if (_access(defaultApacheFile_x86, 0) == 0) {
		if (!DirectoryExists(defaultApacheVersionDir_x86)) {
			SoftwareInfo pSoftwareInfo;
			pSoftwareInfo.fileFullName = "httpd-2.4.58_vs17-x86.zip";
			pSoftwareInfo.serviceType = "apache";
			pSoftwareInfo.version = "httpd-2.4.58_vs17-x86";
			pSoftwareInfo.versionNumber = "httpd-2.4.58";
			pSoftwareInfo.link = "";
			UnzipFile(&pSoftwareInfo);
		}
	}

	if (_access(defaultNginxFile, 0) == 0) {
		if (!DirectoryExists(defaultNginxVersionDir)) {
			SoftwareInfo pSoftwareInfo;
			pSoftwareInfo.fileFullName = "nginx-1.24.0.zip";
			pSoftwareInfo.serviceType = "nginx";
			pSoftwareInfo.version = "nginx-1.24.0";
			pSoftwareInfo.versionNumber = "nginx-1.24.0";
			pSoftwareInfo.link = "";
			UnzipFile(&pSoftwareInfo);
		}
	}
}

DWORD SyncConfigTemplate(SoftwareGroupInfo softwareGroupInfo) {
	if (softwareGroupInfo.php.version != NULL && softwareGroupInfo.apache.version != NULL) {
		char phpLastThree[4], apacheLastThree[4];

		size_t phpLen = strlen(softwareGroupInfo.php.version);
		strncpy_s(phpLastThree, softwareGroupInfo.php.version + phpLen - 3, 3);
		phpLastThree[3] = '\0';

		size_t apacheLen = strlen(softwareGroupInfo.apache.version);
		strncpy_s(apacheLastThree, softwareGroupInfo.apache.version + apacheLen - 3, 3);
		apacheLastThree[3] = '\0';

		if (strcmp(phpLastThree, apacheLastThree) != 0) {
			MessageBoxA(hWndMain, "The architecture types of PHP and Apache must be either both x64 or both x86.", NULL, 0);

			return -1;
		}
	}


	if (
		!(
			(softwareGroupInfo.php.version != NULL && softwareGroupInfo.mysql.version != NULL && softwareGroupInfo.apache.version != NULL) ||
			(softwareGroupInfo.php.version != NULL && softwareGroupInfo.mysql.version != NULL && softwareGroupInfo.nginx.version != NULL)
			)
		) {
		MessageBoxA(hWndMain, "Please select the correct configuration item", "WPEnv", MB_ICONINFORMATION);
		return -1;
	}

	char* wProgramDirectory = get_current_program_directory_with_forward_slash();

	InstallDefaultService(wProgramDirectory);

	char webDir[256];
	sprintf_s(webDir, sizeof(webDir), "%s/www", wProgramDirectory);
	if (!DirectoryExists(webDir)) {
		if (!CreateDirectoryA(webDir, NULL)) {
			return -1;
		}
	}

	char webDefaultDir[256];
	sprintf_s(webDefaultDir, sizeof(webDefaultDir), "%s/www/default", wProgramDirectory);
	if (!DirectoryExists(webDefaultDir)) {
		if (!CreateDirectoryA(webDefaultDir, NULL)) {
			return -1;
		}
	}

	char webLogsDir[256];
	sprintf_s(webLogsDir, sizeof(webLogsDir), "%s/www_logs", wProgramDirectory);
	if (!DirectoryExists(webLogsDir)) {
		if (!CreateDirectoryA(webLogsDir, NULL)) {
			return -1;
		}
	}

	char webDefaultIndexFile[256];
	sprintf_s(webDefaultIndexFile, sizeof(webDefaultIndexFile), "%s/index.php", webDefaultDir);
	if (!CheckFileExists(webDefaultIndexFile)) {
		FILE* file;
		fopen_s(&file, webDefaultIndexFile, "w+N");
		if (file != NULL) {
			fprintf_s(file, "<?php  echo \"Hello World!\";?>\n");
			fclose(file);
		}
	}

	// php
	char phpIniPath[256];
	sprintf_s(phpIniPath, sizeof(phpIniPath), "%s/service/php/%s/php.ini", wProgramDirectory, softwareGroupInfo.php.version);

	if (versionMatch("php", softwareGroupInfo.php.versionNumber, phpIniPath, softwareGroupInfo) != 0) {
		return -1;
	}

	if (softwareGroupInfo.apache.version != NULL) {
		// apache
		char httpdConfPath[256];
		sprintf_s(httpdConfPath, sizeof(httpdConfPath), "%s/service/apache/%s/Apache24/conf/httpd.conf", wProgramDirectory, softwareGroupInfo.apache.version);
		if (versionMatch("apache", softwareGroupInfo.apache.versionNumber, httpdConfPath, softwareGroupInfo) != 0) {
			return -1;
		}

		char httpdVhostsConf[256];
		sprintf_s(httpdVhostsConf, sizeof(httpdVhostsConf), "%s/service/apache/%s/Apache24/conf/extra/httpd-vhosts.conf", wProgramDirectory, softwareGroupInfo.apache.version);

		if (versionMatch("apache", "vhosts", httpdVhostsConf, softwareGroupInfo) != 0) {
			return -1;
		}
	}
	else {
		// nginx
		char nginxConfPath[256];
		sprintf_s(nginxConfPath, sizeof(nginxConfPath), "%s/service/nginx/%s/conf/nginx.conf", wProgramDirectory, softwareGroupInfo.nginx.version);
		if (versionMatch("nginx", softwareGroupInfo.nginx.version, nginxConfPath, softwareGroupInfo) != 0) {
			return -1;
		}

		char nginxVhostsDir[256];
		sprintf_s(nginxVhostsDir, sizeof(nginxVhostsDir), "%s/service/nginx/%s/conf/vhosts", wProgramDirectory, softwareGroupInfo.nginx.version);
		if (_access_s(nginxVhostsDir, 0) == 0) {
			// Folder exists
		}
		else {
			if (_mkdir(nginxVhostsDir) == 0) {
				// Folder create success
			}
			else {
				Log("nginxVhostsDir create fail.");
				// Folder create fail
				MessageBoxA(hWndMain, "Nginx Folder create fail", "WPEnv", MB_ICONINFORMATION);
				return -1;
			}
		}

		char nginxVhostsConf[256];
		sprintf_s(nginxVhostsConf, sizeof(nginxVhostsConf), "%s/service/nginx/%s/conf/vhosts/default.conf", wProgramDirectory, softwareGroupInfo.nginx.version);
		if (versionMatch("nginx", "vhosts", nginxVhostsConf, softwareGroupInfo) != 0) {
			return -1;
		}
	}

	return 0;
}

DWORD SyncPHPAndApacheConf(SoftwareGroupInfo softwareGroupInfo, ServiceUseConfig serviceUse) {
	if (strstr(serviceUse.webService, "httpd") == NULL) {
		return -1;
	}

	char apacheBaseDir[256];
	getApacheVersionAbsBaseDir(softwareGroupInfo.apache.version, apacheBaseDir, sizeof(apacheBaseDir));

	char sourcePath[512];
	sprintf_s(sourcePath, sizeof(sourcePath), "%s/conf/httpd.conf", apacheBaseDir);

	char tempPath[512];
	sprintf_s(tempPath, sizeof(tempPath), "%s/conf/httpd.conf.temp", apacheBaseDir);

	FILE* sourceFile, *newFile;

	errno_t err;

	err = fopen_s(&sourceFile, sourcePath, "rb+N");
	if (err != 0) {
		MessageBoxA(hWndMain, "Switching php version apache configuration did not synchronize successfully, please exit the apache configuration file being edited.", "WPEnv", MB_ICONINFORMATION);
		return -1;
	}

	err = fopen_s(&newFile, tempPath, "wb+N");
	if (err != 0) {
		fclose(sourceFile);
		MessageBoxA(hWndMain, "Switching the php version of apache configuration did not synchronize successfully, create file failed.", "WPEnv", MB_ICONINFORMATION);
		return -1;
	}

	char line[1024];
	bool IsBlockArea = false;
	while (fgets(line, sizeof(line), sourceFile)) {
		if (strstr(line, "#TPL_VAR_Block_PHPAndApache_Start") != NULL) {
			IsBlockArea = true;
			fputs("#TPL_VAR_Block_PHPAndApache_Start\r\n", newFile);
			fputs("{{TPL_VAR:phpLoadModule}}\r\n", newFile);
			fputs("{{TPL_VAR:addType}}\r\n", newFile);
			fputs("{{TPL_VAR:phpIniDir}}\r\n", newFile);
			fputs("{{TPL_VAR:LoadFile}}\r\n", newFile);

			continue;
		}

		if (IsBlockArea == true) {
			if (strstr(line, "#TPL_VAR_Block_PHPAndApache_End") != NULL) {
				IsBlockArea = false;
				fputs("#TPL_VAR_Block_PHPAndApache_End\r\n", newFile);
			}
			else {
				continue;
			}
		}
		else {
			fputs(line, newFile);
		}
	}

	fclose(sourceFile);
	fclose(newFile);

	remove(sourcePath);
	int result = rename(tempPath, sourcePath);
	if (result != 0) {
		char errorMsg[256];
		strerror_s(errorMsg, sizeof(errorMsg), errno);

		MessageBoxA(hWndMain, "Switching the php version of apache configuration did not synchronize successfully, temp file create failed.", "WPEnv", MB_ICONINFORMATION);
		return -1;
	}

	if (versionMatch("apache", softwareGroupInfo.apache.versionNumber, sourcePath, softwareGroupInfo) != 0) {
		MessageBoxA(hWndMain, "Switching the php version of apache configuration did not synchronize successfully, sync new file content failed.", "WPEnv", MB_ICONINFORMATION);
		return -1;
	}

	return 0;
}
