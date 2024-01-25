#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include "framework.h"
#include "Log.h"
#include "BaseFileOpt.h"
#include "FileModify.h"
#include "SyncServiceConfig.h"
#include "FileFindOpt.h"
#include  "ServiceUse.h"

DWORD phpApacheDll(const char* phpVersion, const char* serviceVersionDir, char* result, size_t bufferSize) {
	//char tempPHPVersion[256];
	//strncpy_s(tempPHPVersion, sizeof(tempPHPVersion), phpVersion, strlen(phpVersion));
	//tempPHPVersion[sizeof(tempPHPVersion) - 1] = '\0';

	//char* versionStr;
	//char* context = NULL;
	//versionStr = strtok_s(tempPHPVersion, "_", &context);
	char dllFilePath[512];

	if (
		strcmp(phpVersion, "php-8.3.0") == 0 ||
		strcmp(phpVersion, "php-8.2.0") == 0 ||
		strcmp(phpVersion, "php-8.1.0") == 0 ||
		strcmp(phpVersion, "php-8.0.0") == 0
		) {
		sprintf_s(dllFilePath, sizeof(dllFilePath), "%s/php8apache2_4.dll", serviceVersionDir);
		if (_access_s(dllFilePath, 0) != 0) {
			MessageBoxA(NULL, "Please select from the list and download the PHP version with the 'ts' (Thread Safe) flag to get the missing 'php8apache2_4.dll' file.", NULL, 0);
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
			MessageBoxA(NULL, "Please select from the list and download the PHP version with the 'ts' (Thread Safe) flag to get the missing 'php7apache2_4.dll' file.", NULL, 0);
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
			MessageBoxA(NULL, "Please select from the list and download the PHP version with the 'ts' (Thread Safe) flag to get the missing 'php5apache2_4.dll' file.", NULL, 0);
			return -1;
		}
		sprintf_s(result, bufferSize, "LoadModule php5_module \"%s\"", dllFilePath);
	}
	else {
		strncpy_s(result, bufferSize, "", _TRUNCATE);

		MessageBoxA(NULL, "The httpd service did not find the corresponding php version.", NULL, 0);
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
		MessageBoxA(NULL, "The repository txt file does not exist, please rebuild the project.", NULL, 0);
		return -1;
	}
	errno_t err;

	err = fopen_s(&sourceFile, sourceFilename, "rb");
	if (err != 0) {
		MessageBoxA(NULL, "The repository directory does not exist, please rebuild the project.", NULL, 0);
		return -1;
	}

	err = fopen_s(&newFile, targetFilepath, "wb");
	if (err != 0) {
		MessageBoxA(NULL, "Target file does not exist, please download it first", NULL, 0);
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

DWORD SyncConfigTemplate(SoftwareGroupInfo softwareGroupInfo) {
	char* wProgramDirectory = get_current_program_directory_with_forward_slash();

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
		fopen_s(&file, webDefaultIndexFile, "w");
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
		if (versionMatch("apache", softwareGroupInfo.apache.version, httpdConfPath, softwareGroupInfo) != 0) {
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
				MessageBoxA(NULL, "Nginx Folder create fail", NULL, 0);
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

