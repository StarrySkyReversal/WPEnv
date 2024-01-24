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

void phpApacheDll(const char* phpVersion, const char* serviceVersionDir, char* result, size_t bufferSize) {
	if (strcmp(phpVersion, "php-8.3.0") == 0) {
		sprintf_s(result, bufferSize, "LoadModule php_module \"%s/php8apache2_4.dll\"", serviceVersionDir);
	}
	else if (strcmp(phpVersion, "php-7.2.6") == 0) {
		sprintf_s(result, bufferSize, "LoadModule php7_module \"%s/php7apache2_4.dll\"", serviceVersionDir);
	}
	else if (strcmp(phpVersion, "php-5.6.10") == 0) {
		sprintf_s(result, bufferSize, "LoadModule php5_module \"%s/php5apache2_4.dll\"", serviceVersionDir);
	}
	else {
		strncpy_s(result, bufferSize, "", _TRUNCATE);
	}
}

void versionMatch(const char* serviceType, const char* fileTagVersion, const char* targetFilepath, SoftwareGroupInfo softwareGroupInfo) {
	char* wProgramDirectory = get_current_program_directory_with_forward_slash();

	// php.ini
	// nginx.conf vhosts/default.conf
	// httpd.conf vhosts/default.conf
	// mysql my.ini current is not exists

	FILE *sourceFile, *newFile;

	char sourceFilename[256];
	sprintf_s(sourceFilename, sizeof(sourceFilename), "%s/Repository/%s/%s.txt", wProgramDirectory, serviceType, fileTagVersion);
	if (_access_s(sourceFilename, 0) != 0) {
		return;
	}
	errno_t err;

	err = fopen_s(&sourceFile, sourceFilename, "rb");
	if (err != 0) {
		MessageBoxA(NULL, "sourceFilename Error", NULL, 0);
		return;
	}

	err = fopen_s(&newFile, targetFilepath, "wb");
	if (err != 0) {
		MessageBoxA(NULL, "targetFilepath Error", NULL, 0);
		return;
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

	if (strcmp(serviceType, "apache") == 0) {
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

			replaceStringInFile(targetFilepath, "{{TPL_VAR:addType}}", "AddType application/x-httpd-php .php");

			char phpIniDir[512];
			sprintf_s(phpIniDir, sizeof(phpIniDir), "phpIniDir %s", phpDir);

			replaceStringInFile(targetFilepath, "{{TPL_VAR:phpIniDir}}", phpIniDir);

			char dllStr[512];
			// phpIniDir

			phpApacheDll(softwareGroupInfo.php.version, phpDir, dllStr, sizeof(dllStr));

			replaceStringInFile(targetFilepath, "{{TPL_VAR:phpLoadModule}}", dllStr);
		}
	}
	else if (strcmp(serviceType, "nginx") == 0) {
		replaceStringInFile(targetFilepath, "{{TPL_VAR:DocumentRoot}}", wwwDefaultDir);
		replaceStringInFile(targetFilepath, "{{TPL_VAR:ServerName}}", "localhost");
		replaceStringInFile(targetFilepath, "{{TPL_VAR:wwwLogs}}", wwwLogsDir);
	}
}

void sync(SoftwareGroupInfo softwareGroupInfo) {
	char* wProgramDirectory = get_current_program_directory_with_forward_slash();

	char webDir[256];
	sprintf_s(webDir, sizeof(webDir), "%s/www", wProgramDirectory);
	if (!DirectoryExists(webDir)) {
		if (!CreateDirectoryA(webDir, NULL)) {
			return;
		}
	}

	char webDefaultDir[256];
	sprintf_s(webDefaultDir, sizeof(webDefaultDir), "%s/www/default", wProgramDirectory);
	if (!DirectoryExists(webDefaultDir)) {
		if (!CreateDirectoryA(webDefaultDir, NULL)) {
			return;
		}
	}

	char webLogsDir[256];
	sprintf_s(webLogsDir, sizeof(webLogsDir), "%s/www_logs", wProgramDirectory);
	if (!DirectoryExists(webLogsDir)) {
		if (!CreateDirectoryA(webLogsDir, NULL)) {
			return;
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

	versionMatch("php", softwareGroupInfo.php.version, phpIniPath, softwareGroupInfo);

	if (softwareGroupInfo.apache.version != NULL) {
		// apache
		char httpdConfPath[256];
		sprintf_s(httpdConfPath, sizeof(httpdConfPath), "%s/service/apache/%s/Apache24/conf/httpd.conf", wProgramDirectory, softwareGroupInfo.apache.version);
		versionMatch("apache", softwareGroupInfo.apache.version, httpdConfPath, softwareGroupInfo);

		char httpdVhostsConf[256];
		sprintf_s(httpdVhostsConf, sizeof(httpdVhostsConf), "%s/service/apache/%s/Apache24/conf/extra/httpd-vhosts.conf", wProgramDirectory, softwareGroupInfo.apache.version);

		versionMatch("apache", "vhosts", httpdVhostsConf, softwareGroupInfo);
	}
	else {
		// nginx
		char nginxConfPath[256];
		sprintf_s(nginxConfPath, sizeof(nginxConfPath), "%s/service/nginx/%s/conf/nginx.conf", wProgramDirectory, softwareGroupInfo.nginx.version);
		versionMatch("nginx", softwareGroupInfo.nginx.version, nginxConfPath, softwareGroupInfo);

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
			}
		}

		char nginxVhostsConf[256];
		sprintf_s(nginxVhostsConf, sizeof(nginxVhostsConf), "%s/service/nginx/%s/conf/vhosts/default.conf", wProgramDirectory, softwareGroupInfo.nginx.version);
		versionMatch("nginx", "vhosts", nginxVhostsConf, softwareGroupInfo);
	}
}

