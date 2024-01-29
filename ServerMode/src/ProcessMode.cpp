#include "framework.h"
#include "ProcessMode.h"
#include <stdio.h>
#include "Common.h"
#include "Log.h"
#include "BaseFileOpt.h"
#include "RichEditControls.h"
#include "ServiceUse.h"
#include "FileFindOpt.h"
#include "BaseFileOpt.h"
#include "ModeMonitor.h"
#include "ProcessOpt.h"
#include <io.h>
#include "SyncServiceConfig.h"
#include "IniOpt.h"


bool bIsStart = false;
bool bPHPRunning = false;
bool bMysqlRunning = false;
bool bApacheRunning = false;
bool bNginxRunning = false;

#define PIPE_STATUS_RUNNING 1
#define PIPE_STATUS_EXIT 0

typedef struct {
    HANDLE hRead;
    HANDLE hWrite;
    int status;
    const char* processName;
} ProcessPipe;

typedef struct {
    const char* cmd;
    const char* dir;
    const char* processName;
    const char* serviceName;
    ProcessPipe* pipe;
    PROCESS_INFORMATION pi;
} ProcessDetail;

WebDaemonService webDaemonServiceInstance;

ProcessDetail* pPhpProcessDetail = nullptr;
ProcessDetail* pMysqlProcessDetail = nullptr;
ProcessDetail* pMysqlClientProcessDetail = nullptr;
ProcessDetail* pMysqlInitProcessDetail = nullptr;
ProcessDetail* pWebServiceProcessDetail = nullptr;

int EndsWithNewline(const char* str) {
    size_t len = strlen(str);
    if (len == 0) {
        return 0;
    }
    if (str[len - 1] == '\n') {
        return 1;
    }
    if (len > 1 && str[len - 2] == '\r' && str[len - 1] == '\n') {
        return 1;
    }
    return 0;
}

DWORD WINAPI ReadFromPipeThread(LPVOID lpParam) {
    ProcessPipe* processPipe = (ProcessPipe*)lpParam;

    DWORD availableBytes = 0;
    while (true) {
        PeekNamedPipe(processPipe->hRead, NULL, 0, NULL, &availableBytes, NULL);
        if (processPipe->status == PIPE_STATUS_EXIT) {
            Log("Pipe %s exit.\r\n", processPipe->processName);
            break;
        }

        // It is essential to add judgment behavior here, otherwise it will lead to read-end blocking,
        // causing the execution of this line of code to end without any warning. Moreover, if a breakpoint is set here,
        // the 'step over' operation will not be able to proceed.
        if (availableBytes > 0) {
            DWORD bytesRead = 0;
            char buffer[65535] = { '\0' };
            ReadFile(processPipe->hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
            buffer[bytesRead] = '\0';

            char wOutputBuffer[65525];
            sprintf_s(wOutputBuffer, sizeof(wOutputBuffer), "Create process %s result: %s", processPipe->processName, buffer);

            if (strstr(buffer, "error")) {
                ErrOutput(wOutputBuffer);
            } else if(strstr(buffer, "warning")) {
                WarnOutput(wOutputBuffer);
            }
            else {
                InfoOutput(wOutputBuffer);
            }

            if (!EndsWithNewline(buffer)) {
                InfoOutput("\r\n");
            }
        }

        Sleep(100);
    }

    CloseHandle(processPipe->hRead);
    CloseHandle(processPipe->hWrite);

    delete processPipe;

    return 0;
}

HANDLE hJob;
bool CalllCreateProcess(ProcessDetail* pProcessDetail, bool waitProcess = false) {
    STARTUPINFOA si;

    size_t cmdStrLen = strlen(pProcessDetail->cmd) + 1;
    char* wCmdStr = (char*)malloc(cmdStrLen * sizeof(char));
    if (!wCmdStr) {
        return false;
    }
    strcpy_s(wCmdStr, cmdStrLen, pProcessDetail->cmd);

    hJob = CreateJobObject(NULL, NULL);
    if (hJob == NULL) {
        free(wCmdStr);
        LogAndMsgBox("CreateJobObject fail.\r\n");
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = { 0 };
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if (!SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo))) {
        CloseHandle(hJob);
        free(wCmdStr);

        LogAndMsgBox(L"SetInformationJobObject failed with error code: %d\r\n", GetLastError());
        return false;
    }

    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    // Create an anonymous pipe
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        Log("Pipe creation failed\r\n");
        free(wCmdStr);
        return false;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;

    ZeroMemory(&(pProcessDetail->pi), sizeof(pProcessDetail->pi));

    if (CreateProcessA(NULL, wCmdStr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, pProcessDetail->dir, &si, &(pProcessDetail->pi))) {
        if (!AssignProcessToJobObject(hJob, pProcessDetail->pi.hProcess)) {
            Log("AssignProcessToJobObject error.\r\n");
            TerminateProcess(pProcessDetail->pi.hProcess, 1);
            CloseHandle(pProcessDetail->pi.hProcess);
            CloseHandle(pProcessDetail->pi.hThread);
            CloseHandle(hJob);
            free(wCmdStr);
            return false;
        }

        if (waitProcess == true) {
            WaitForSingleObject(pProcessDetail->pi.hProcess, INFINITE);
        }

        CloseHandle(pProcessDetail->pi.hProcess);
        CloseHandle(pProcessDetail->pi.hThread);
    }
    else {
        free(wCmdStr);
        Log("CreateProcess error.\r\n");
        CloseHandle(hJob);
    }

    pProcessDetail->pipe = new ProcessPipe();
    pProcessDetail->pipe->hRead = hRead;
    pProcessDetail->pipe->hWrite = hWrite;
    pProcessDetail->pipe->status = PIPE_STATUS_RUNNING;
    pProcessDetail->pipe->processName = pProcessDetail->processName;

    HANDLE hThread = CreateThread(NULL, 0, ReadFromPipeThread, pProcessDetail->pipe, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    }

    free(wCmdStr);

    return true;
}

DWORD phpProcess(ServiceUseConfig* serviceUse) {
    PathList* pathsPHP = initPathList();

    char phpDirectoryPath[512];
    sprintf_s(phpDirectoryPath, sizeof(phpDirectoryPath), "%s/php/%s", DIRECTORY_SERVICE, serviceUse->php);

    findFilesInDirectory(phpDirectoryPath, "php-cgi.exe", pathsPHP);
    if (pathsPHP->count != 1) {
        LogAndMsgBox("This program %s does not exist, please download and unzip it first, and add the configuration again.\r\n", serviceUse->php);
        return 1;
    }

    char phpRunCmd[1024] = { '\0' };
    sprintf_s(phpRunCmd, sizeof(phpRunCmd),
        "%s -b 127.0.0.1:9000", pathsPHP->paths[0]);

    // Start php
    pPhpProcessDetail->processName = "php-cgi.exe";
    pPhpProcessDetail->cmd = phpRunCmd;
    pPhpProcessDetail->dir = phpDirectoryPath;
    if (CalllCreateProcess(pPhpProcessDetail) == true) {
        InfoOutput("PHP runing.\r\n");
    }
    else {
        ErrOutput("PHP start fail.\r\n");
    }

    char phpBinDirectory[256];
    GetDirectoryFromPath(pathsPHP->paths[0], phpBinDirectory, sizeof(phpBinDirectory));

    webDaemonServiceInstance.phpExe = "php-cgi.exe";
    webDaemonServiceInstance.phpExePath = _strdup(pathsPHP->paths[0]);
    webDaemonServiceInstance.phpExeDirectory = _strdup(phpBinDirectory);

    freePathList(pathsPHP);
    bPHPRunning = true;

    return 0;
}

DWORD mysqlClientProcess(ServiceUseConfig* serviceUse, bool bMysqlInit) {
    // or before mysql5.6.7
    if (bMysqlInit == true) {
        PathList* pathsMysqlClient = initPathList();

        char mysqlClientDirectoryPath[512];
        sprintf_s(mysqlClientDirectoryPath, sizeof(mysqlClientDirectoryPath), "%s/mysql/%s", DIRECTORY_SERVICE, serviceUse->mysql);

        findFilesInDirectory(mysqlClientDirectoryPath, "mysql.exe", pathsMysqlClient);
        if (pathsMysqlClient->count != 1) {
            LogAndMsgBox("This program %s mysqlclient does not exist, please download and unzip it first, and add the configuration again.\r\n", serviceUse->mysql);
            return 1;
        }

        // format get bin directory
        char mysqlClientBinDirectory[1024] = { '\0' };
        GetDirectoryFromPath(pathsMysqlClient->paths[0], mysqlClientBinDirectory, sizeof(mysqlClientBinDirectory));

        // Initialized after mysql set password
        // initialized root user password
        char mysqlClientCmdInitRootUser[1024] = { '\0' };
        sprintf_s(mysqlClientCmdInitRootUser, sizeof(mysqlClientCmdInitRootUser),
            "%s -u root -e \"ALTER USER 'root'@'localhost' IDENTIFIED BY 'root'; FLUSH PRIVILEGES;\"", pathsMysqlClient->paths[0]);

        pMysqlClientProcessDetail->processName = "mysql.exe";
        pMysqlClientProcessDetail->cmd = mysqlClientCmdInitRootUser;
        pMysqlClientProcessDetail->dir = mysqlClientBinDirectory;
        CalllCreateProcess(pMysqlClientProcessDetail);
        ////////////////////////////////////////////////////////////////////////

        freePathList(pathsMysqlClient);
    }

    return 0;
}

DWORD mysqlServiceProcess(ServiceUseConfig* serviceUse, bool* bMysqlInit) {
    PathList* pathsMysql = initPathList();

    char mysqldDirectoryPath[512];
    sprintf_s(mysqldDirectoryPath, sizeof(mysqldDirectoryPath), "%s/mysql/%s", DIRECTORY_SERVICE, serviceUse->mysql);

    findFilesInDirectory(mysqldDirectoryPath, "mysqld.exe", pathsMysql);
    if (pathsMysql->count != 1) {
        LogAndMsgBox("This program %s mysqld does not exist, please download and unzip it first, and add the configuration again.\r\n", serviceUse->mysql);

        freePathList(pathsMysql);
        return 1;
    }

    // mysql data directory
    char mysqlDataDirectory[256];
    sprintf_s(mysqlDataDirectory, sizeof(mysqlDataDirectory), "%s/data", mysqldDirectoryPath);

    // format get bin directory
    char mysqldBinDirectory[1024] = { '\0' };
    GetDirectoryFromPath(pathsMysql->paths[0], mysqldBinDirectory, sizeof(mysqldBinDirectory));

    // mysql initialized
    errno_t mysqlDataDirectoryCheck = _access(mysqlDataDirectory, 0);

    *bMysqlInit = false;
    if (mysqlDataDirectoryCheck != 0) {
        *bMysqlInit = true;

        Log("Initialize mysql.\r\n");
        InfoOutput("MySQL is about to start initialization.\r\n");

        ////////////////////////////////////////////////////////////////////////
        // No need to set a password --initialize-insecure,and not use --initialize
        // If you encounter an error, you can add the --console argument at the end to allow the console to output in real-time. This is specific to MYSQL.
        char mysqldCmdInitDataDirectory[1024] = { '\0' };
        sprintf_s(mysqldCmdInitDataDirectory, sizeof(mysqldCmdInitDataDirectory),
            "%s --initialize-insecure --explicit_defaults_for_timestamp --datadir=data", pathsMysql->paths[0]);

        pMysqlInitProcessDetail->processName = "mysqld.exe";
        pMysqlInitProcessDetail->cmd = mysqldCmdInitDataDirectory;
        pMysqlInitProcessDetail->dir = mysqldBinDirectory;
        if (CalllCreateProcess(pMysqlInitProcessDetail, true) == true) {
            InfoOutput("Mysql initialized.\r\n");
        }
        else {
            ErrOutput("Mysql initialize fail.\r\n");
        }
        ////////////////////////////////////////////////////////////////////////
    }

    // Start mysql
    pMysqlProcessDetail->processName = "mysqld.exe";
    char mysqldCmd[256];
    //sprintf_s(mysqldCmd, sizeof(mysqldCmd), "%s --explicit_defaults_for_timestamp --console", pathsMysql->paths[0]);
    sprintf_s(mysqldCmd, sizeof(mysqldCmd), "%s", pathsMysql->paths[0]);
    pMysqlProcessDetail->cmd = mysqldCmd;
    pMysqlProcessDetail->dir = mysqldBinDirectory;

    if (CalllCreateProcess(pMysqlProcessDetail) == true) {
        InfoOutput("Mysql runing.\r\n");
    }
    else {
        ErrOutput("Mysql start fail.\r\n");
    }

    webDaemonServiceInstance.mysqldExe = "mysqld.exe";
    webDaemonServiceInstance.mysqldExePath = _strdup(pathsMysql->paths[0]);
    webDaemonServiceInstance.mysqldExeDirectory = _strdup(mysqldBinDirectory);

    bMysqlRunning = true;

    freePathList(pathsMysql);

    return 0;
}

DWORD webServiceProcess(ServiceUseConfig* serviceUse) {
    PathList* pathsWebservice = initPathList();

    const char* webServiceType;
    const char* webServiceBinExe;
    if (strncmp(serviceUse->webService, "httpd", 5) == 0) {
        webServiceType = "apache";
        webServiceBinExe = "httpd.exe";
    }
    else {
        webServiceType = "nginx";
        webServiceBinExe = "nginx.exe";
    }

    char webServiceDirectoryPath[512];
    sprintf_s(webServiceDirectoryPath, sizeof(webServiceDirectoryPath), "%s/%s/%s", DIRECTORY_SERVICE, webServiceType, serviceUse->webService);

    findFilesInDirectory(webServiceDirectoryPath, webServiceBinExe, pathsWebservice);

    if (pathsWebservice->count != 1) {
        LogAndMsgBox("This program %s does not exist, please download and unzip it first, and add the configuration again.\r\n", serviceUse->webService);
        return 1;
    }

    // Start webservice apache || nginx
    char webServiceBinDirectory[256];
    GetDirectoryFromPath(pathsWebservice->paths[0], webServiceBinDirectory, sizeof(webServiceBinDirectory));

    pWebServiceProcessDetail->processName = webServiceBinExe;
    pWebServiceProcessDetail->cmd = pathsWebservice->paths[0];
    pWebServiceProcessDetail->dir = webServiceBinDirectory;

    char* tempServiceProcessName;
    tempServiceProcessName = _strdup(webServiceType);

    tempServiceProcessName[0] = toupper(tempServiceProcessName[0]);

    if (CalllCreateProcess(pWebServiceProcessDetail) == true) {
        char successMsg[256];
        sprintf_s(successMsg, sizeof(successMsg), "%s is running\r\n", tempServiceProcessName);

        InfoOutput(successMsg);
    }
    else {
        char failMsg[256];
        sprintf_s(failMsg, sizeof(failMsg), "%s start fail\r\n", tempServiceProcessName);

        ErrOutput(failMsg);
    }

    char serviceConfDir[512];
    char serviceVhostsFile[512];
    char* wProgramDirectory = get_current_program_directory_with_forward_slash();
    if (strncmp(serviceUse->webService, "httpd", 5) == 0) {
        sprintf_s(serviceConfDir, sizeof(serviceConfDir), "%s/%s/Apache24/conf/extra", wProgramDirectory, webServiceDirectoryPath);
        sprintf_s(serviceVhostsFile, sizeof(serviceVhostsFile), "%s/httpd-vhosts.conf", serviceConfDir);
    }
    else {
        sprintf_s(serviceConfDir, sizeof(serviceConfDir), "%s/%s/conf", wProgramDirectory, webServiceDirectoryPath);
        sprintf_s(serviceVhostsFile, sizeof(serviceVhostsFile), "%s/vhosts/default.conf", serviceConfDir);
    }

    webDaemonServiceInstance.webServiceExe = webServiceBinExe;
    webDaemonServiceInstance.webServiceExePath = _strdup(pathsWebservice->paths[0]);
    webDaemonServiceInstance.webServiceExeDirectory = _strdup(webServiceBinDirectory);
    webDaemonServiceInstance.webServiceConfDirectory = _strdup(serviceConfDir);
    webDaemonServiceInstance.webServiceVhostsFile = _strdup(serviceVhostsFile);
    webDaemonServiceInstance.webServiceVersion = _strdup(serviceUse->webService);

    bApacheRunning = strcmp(webServiceType, "apache") == 0 ? true : false;
    bNginxRunning = strcmp(webServiceType, "nginx") == 0 ? true : false;

    free(tempServiceProcessName);
    freePathList(pathsWebservice);

    write_ini_file("config/base.ini", "Service", "Version", serviceUse->webService);
    write_ini_file("config/base.ini", "Service", "LastConfDir", serviceConfDir);
    write_ini_file("config/base.ini", "Service", "LastVhostDir", serviceVhostsFile);

    return 0;
}

DWORD WINAPI DaemonServiceThread(LPVOID lParam) {
    if (bIsStart == true) {
        return 0;
    }

    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_START_SERVICE), FALSE);

    bIsStart = true;

    pPhpProcessDetail = new ProcessDetail;
    pPhpProcessDetail->cmd = NULL;

    pMysqlProcessDetail = new ProcessDetail;
    pMysqlProcessDetail->cmd = NULL;

    pMysqlClientProcessDetail = new ProcessDetail;
    pMysqlClientProcessDetail->cmd = NULL;

    pMysqlInitProcessDetail = new ProcessDetail;
    pMysqlInitProcessDetail->cmd = NULL;

    pWebServiceProcessDetail = new ProcessDetail;
    pWebServiceProcessDetail->cmd = NULL;

    webDaemonServiceInstance.phpExe = NULL;
    webDaemonServiceInstance.mysqldExe = NULL;
    webDaemonServiceInstance.webServiceExe = NULL;

    ServiceUseConfig* serviceUse = (ServiceUseConfig*)malloc(sizeof(ServiceUseConfig));

    if (!GetServiceUseItem(serviceUse)) {
        bIsStart = false;
        EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_START_SERVICE), TRUE);
        return 1;
    }

    SoftwareGroupInfo softwareGroupInfo;
    GetConfigViewVersionInfo(&softwareGroupInfo, serviceUse);
    SyncPHPAndApacheConf(softwareGroupInfo, *serviceUse);

    FreeSoftwareInfo(&softwareGroupInfo.php);
    FreeSoftwareInfo(&softwareGroupInfo.mysql);
    FreeSoftwareInfo(&softwareGroupInfo.apache);
    FreeSoftwareInfo(&softwareGroupInfo.nginx);

    ClearRichEdit();

    ///////////////////////PHP/////////////////////////
    phpProcess(serviceUse);
    ///////////////////MYSQL1//////////////////////////
    bool bMysqlInit = false;
    mysqlServiceProcess(serviceUse, &bMysqlInit);
    ///////////////////MYSQL2//////////////////////////
    mysqlClientProcess(serviceUse, bMysqlInit);
    /////////////////WebService////////////////////////
    webServiceProcess(serviceUse);

    webDaemonServiceInstance.bRun = true;

    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_REMOVE_CONFIG), FALSE);

    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_STOP_SERVICE), TRUE);
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_RESTART_SERVICE), TRUE);
    EnableWindow(GetDlgItem(hWndMain, IDC_LISTBOX_CONFIG), FALSE);

    free(serviceUse);

    bIsStart = false;

    return 0;
}

void freeWebDaemonServiceInstance(WebDaemonService* webDaemonService) {
    if (webDaemonService->phpExe != NULL) {
        free((void*)webDaemonService->phpExePath);
        free((void*)webDaemonService->phpExeDirectory);
    }

    if (webDaemonService->mysqldExe != NULL) {
        free((void*)webDaemonService->mysqldExePath);
        free((void*)webDaemonService->mysqldExeDirectory);
    }

    if (webDaemonService->webServiceExe != NULL) {
        free((void*)webDaemonService->webServiceExePath);
        free((void*)webDaemonService->webServiceExeDirectory);
        free((void*)webDaemonService->webServiceConfDirectory);
        free((void*)webDaemonService->webServiceVhostsFile);
    }
}

DWORD WINAPI DaemonMonitorService(LPVOID lParam) {
    //EnableDebugPrivilege();
    DWORD previousPHPHash = 0;
    DWORD previousMysqlHash = 0;
    DWORD previousApacheHash = 0;
    DWORD previousNginxHash = 0;

    bool isFirstRun = true;

    while (true) {
        EnterCriticalSection(&daemonMonitorServiceCs);
        //const char* processes[] = { L"php-cgi.exe", L"mysqld.exe", L"httpd.exe", L"nginx.exe"};
        const char* phpProcesses[] = { "php-cgi.exe"};
        const char* mysqlProcesses[] = { "mysqld.exe" };
        const char* apacheProcesses[] = { "httpd.exe" };
        const char* nginxProcesses[] = { "nginx.exe" };

        DWORD currentPHPHash = getTargetProcessesHash(phpProcesses, sizeof(phpProcesses) / sizeof(phpProcesses[0]));
        DWORD currentMysqlHash = getTargetProcessesHash(mysqlProcesses, sizeof(mysqlProcesses) / sizeof(mysqlProcesses[0]));
        DWORD currentApacheHash = getTargetProcessesHash(apacheProcesses, sizeof(apacheProcesses) / sizeof(apacheProcesses[0]));
        DWORD currentNginxHash = getTargetProcessesHash(nginxProcesses, sizeof(nginxProcesses) / sizeof(nginxProcesses[0]));

        if ((currentPHPHash != previousPHPHash || isFirstRun == true) && isSelfChildProcessOfCurrent("php-cgi.exe") == 2 && IsHttpdParentRunning("php-cgi.exe")) {
            WarnOutput("PHP is not started by this program.\r\n");
        }

        if ((previousMysqlHash != currentMysqlHash || isFirstRun == true) && isSelfChildProcessOfCurrent("mysqld.exe") == 2 && IsHttpdParentRunning("mysqld.exe")) {
            WarnOutput("Mysql is not started by this program.\r\n");
        }

        if ((previousApacheHash != currentApacheHash || isFirstRun == true) && isSelfChildProcessOfCurrent("httpd.exe") == 2 && IsHttpdParentRunning("httpd.exe")) {
            WarnOutput("Apache is not started by this program.\r\n");
        }

        if ((previousNginxHash != currentNginxHash || isFirstRun == true) && isSelfChildProcessOfCurrent("nginx.exe") == 2 && IsHttpdParentRunning("nginx.exe")) {
            WarnOutput("Nginx is not started by this program.\r\n");
        }

        // While it's running, monitor to see if the process has been closed due to interference from other processes.
        if (bPHPRunning && !ProcessIsRunning("php-cgi.exe")) {
            ErrOutput("php-cgi.exe Unexpected exit.\r\n");
            bPHPRunning = false;
        }

        if (bMysqlRunning && !ProcessIsRunning("mysqld.exe")) {
            ErrOutput("mysqld.exe Unexpected exit.\r\n");
            bMysqlRunning = false;
        }

        if (bApacheRunning && !ProcessIsRunning("httpd.exe")) {
            ErrOutput("httpd.exe Unexpected exit.\r\n");
            bApacheRunning = false;
        }

        if (bNginxRunning && !ProcessIsRunning("nginx.exe")) {
            ErrOutput("nginx.exe Unexpected exit.\r\n");
            bNginxRunning = false;
        }

        previousPHPHash = currentPHPHash;
        previousMysqlHash = currentMysqlHash;
        previousApacheHash = currentApacheHash;
        previousNginxHash = currentNginxHash;

        isFirstRun = false;

        LeaveCriticalSection(&daemonMonitorServiceCs);

        Sleep(500);
    }

    return 0;
}

DWORD StartDaemonService() {
    HANDLE daemonService = CreateThread(NULL, 0, DaemonServiceThread, NULL, 0, NULL);
    if (daemonService) {
        CloseHandle(daemonService);
    }

    return 0;
}

DWORD CallDeleteProcess(ProcessDetail pProcessDetail) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    //ProcessDetail* pProcessDetail = (ProcessDetail*)lParam;
    size_t wCmdStrLen = strlen(pProcessDetail.cmd) + 1;
    char* wCmdStr = (char*)malloc(wCmdStrLen * sizeof(char));
    if (!wCmdStr) {
        return 0;
    }
    strcpy_s(wCmdStr, wCmdStrLen, pProcessDetail.cmd);

    if (!CreateProcessA(NULL, wCmdStr, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, pProcessDetail.dir, &si, &pi)) {
        free(wCmdStr);
        return 0;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    char publicMsgInfo[256] = { '\0' };

    if (exitCode != 0) {
        Log("code:%lu\r\n", exitCode);
        sprintf_s(publicMsgInfo, sizeof(publicMsgInfo), "%s stop fail\r\n", pProcessDetail.serviceName);
        ErrOutput(publicMsgInfo);
    }
    else {
        sprintf_s(publicMsgInfo, sizeof(publicMsgInfo), "%s stop\r\n", pProcessDetail.serviceName);
        InfoOutput(publicMsgInfo);
    }

    free(wCmdStr);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 1;
}

void CloseDaemonService() {
    if (webDaemonServiceInstance.bRun == false) {
        return;
    }

    EnterCriticalSection(&daemonMonitorServiceCs);

    ProcessDetail pProcessDetail;

    char cmdService[256] = { '\0' };
    if (webDaemonServiceInstance.webServiceExe == "nginx.exe") {
        sprintf_s(cmdService, sizeof(cmdService), "%s -s quit", webDaemonServiceInstance.webServiceExePath);

        pProcessDetail.serviceName = "Nginx";
        pProcessDetail.processName = webDaemonServiceInstance.webServiceExe;
        pProcessDetail.cmd = cmdService;
        pProcessDetail.dir = webDaemonServiceInstance.webServiceExeDirectory;
        CallDeleteProcess(pProcessDetail);

        bNginxRunning = false;
    }
    else {
        // Terminating the Apache process requires administrator privileges.
        sprintf_s(cmdService, sizeof(cmdService), "taskkill.exe /F /IM %s", webDaemonServiceInstance.webServiceExe);
        pProcessDetail.serviceName = "Apache";
        pProcessDetail.processName = webDaemonServiceInstance.webServiceExe;
        pProcessDetail.cmd = cmdService;
        //pProcessDetail.dir = webDaemonServiceInstance.webServiceExeDirectory;
        pProcessDetail.dir = "C:/Windows/system/";
        CallDeleteProcess(pProcessDetail);

        bApacheRunning = false;
    }

    pProcessDetail.serviceName = "Mysql";
    pProcessDetail.processName = "mysqld.exe";
    pProcessDetail.cmd = "taskkill.exe /F /IM mysqld.exe";
    pProcessDetail.dir = "C:/Windows/system/";
    CallDeleteProcess(pProcessDetail);

    bMysqlRunning = false;

    pProcessDetail.serviceName = "PHP";
    pProcessDetail.processName = "php-cgi.exe";
    pProcessDetail.cmd = "taskkill.exe /F /IM php-cgi.exe";
    pProcessDetail.dir = "C:/Windows/system/";
    CallDeleteProcess(pProcessDetail);

    bPHPRunning = false;

    if (hJob) {
        // This function terminates all processes within the job.
        //TerminateJobObject(hJob, 0);

        // For this exit handle function, after testing, it was found that it can close Apache, but cannot close PHP and MySQL.
        CloseHandle(hJob);
        //TerminateJobObject(hJob, 0);
        //bool tRes = TerminateJobObject(hJob, 0);
    }

    freeWebDaemonServiceInstance(&webDaemonServiceInstance);

    if (pPhpProcessDetail->cmd != NULL) {
        pPhpProcessDetail->pipe->status = PIPE_STATUS_EXIT;
    }

    if (pMysqlProcessDetail->cmd != NULL) {
        pMysqlProcessDetail->pipe->status = PIPE_STATUS_EXIT;
    }

    if (pMysqlClientProcessDetail->cmd != NULL) {
        pMysqlClientProcessDetail->pipe->status = PIPE_STATUS_EXIT;
    }

    if (pMysqlInitProcessDetail->cmd != NULL) {
        pMysqlInitProcessDetail->pipe->status = PIPE_STATUS_EXIT;
    }

    if (pWebServiceProcessDetail->cmd != NULL) {
        pWebServiceProcessDetail->pipe->status = PIPE_STATUS_EXIT;
    }

    delete pPhpProcessDetail;
    delete pMysqlProcessDetail;
    delete pMysqlClientProcessDetail;
    delete pMysqlInitProcessDetail;
    delete pWebServiceProcessDetail;

    LeaveCriticalSection(&daemonMonitorServiceCs);

    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_STOP_SERVICE), FALSE);
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_RESTART_SERVICE), FALSE);
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_START_SERVICE), TRUE);
    EnableWindow(GetDlgItem(hWndMain, IDC_LISTBOX_CONFIG), TRUE);
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_REMOVE_CONFIG), TRUE);

    webDaemonServiceInstance.bRun = false;
}

void RestartDaemonService() {
    if (webDaemonServiceInstance.bRun != true) {
        return;
    }

    CloseDaemonService();

    StartDaemonService();
}
