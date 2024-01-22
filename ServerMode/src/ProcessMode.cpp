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

BOOL bPHPRunning = false;
BOOL bMysqlRunning = false;
BOOL bApacheRunning = false;
BOOL bNginxRunning = false;

struct ProcessDetail {
    const wchar_t* cmd;
    const wchar_t* dir;
    const wchar_t* processName;
    const wchar_t* serviceName;
    PROCESS_INFORMATION pi;
};

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

struct ProcessPipe {
    HANDLE hRead;
    HANDLE hWrite;
    HANDLE hProcess;
    const wchar_t* processName;
    //HANDLE hThread;
};

DWORD ReadFromPipeThread(LPVOID lpParam) {
    ProcessPipe* processPipe = (ProcessPipe*)lpParam;

    while (true) {
        DWORD availableBytes = 0;
        if (!PeekNamedPipe(processPipe->hRead, NULL, 0, NULL, &availableBytes, NULL)) {
            Log("PeekNamedPipe failed with error: %d\r\n", GetLastError());
            break;
        }

        if (availableBytes == 0) {
            Sleep(100);  // Wait a bit before checking again
            continue;
        }

        DWORD bytesRead = 0;
        char buffer[65535] = { '\0' };
        if (ReadFile(processPipe->hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            buffer[bytesRead] = '\0';

            wchar_t wBuffer[65535] = { '\0' };
            MToW(buffer, wBuffer, sizeof(wBuffer) / sizeof(wchar_t));

            wchar_t wOutputBuffer[65525];
            swprintf_s(wOutputBuffer, _countof(wOutputBuffer), L"INFO: Create process %ls result: %ls \r\n", processPipe->processName, wBuffer);

            AppendEditInfo(wOutputBuffer);
            //AppendEditInfo(wBuffer);

            if (!EndsWithNewline(buffer)) {
                AppendEditInfo(L"\r\n");
            }
        }
    }

    CloseHandle(processPipe->hRead);  // Close the handle once you are done reading

    delete processPipe;
    return 0;
}

HANDLE hJob;
bool CalllCreateProcess(ProcessDetail* pProcessDetail, bool waitProcess = false) {
    STARTUPINFO si;

    size_t cmdStrLen = wcslen(pProcessDetail->cmd) + 1;
    wchar_t* wCmdStr = (wchar_t*)malloc(cmdStrLen * sizeof(wchar_t));
    if (!wCmdStr) {
        return false;
    }
    wcscpy_s(wCmdStr, cmdStrLen, pProcessDetail->cmd);

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

    if (CreateProcess(NULL, wCmdStr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, pProcessDetail->dir, &si, &(pProcessDetail->pi))) {
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

    ProcessPipe* processPipe = new ProcessPipe;
    processPipe->hRead = hRead;
    processPipe->hProcess = pProcessDetail->pi.hProcess;
    processPipe->processName = pProcessDetail->processName;

    HANDLE hThread = CreateThread(NULL, 0, ReadFromPipeThread, processPipe, 0, NULL);
    //if (hThread) {
    //    WaitForSingleObject(hThread, INFINITE);
    //}

    //if (hThread == NULL) {
    //    free(wCmdStr);
    //    Log("Failed to create thread.\r\n");
    //    CloseHandle(hRead);
    //}
    //else {
    //    CloseHandle(hThread);  // Close the thread handle if you don't need to reference it anymore
    //}

    free(wCmdStr);

    return true;
}

struct WebDaemonService {
    bool bRun;

    const wchar_t* phpExe;
    const wchar_t* phpExePath;
    const wchar_t* phpExeDirectory;

    const wchar_t* mysqldExe;
    const wchar_t* mysqldExePath;
    const wchar_t* mysqldExeDirectory;

    const wchar_t* webServiceExe;
    const wchar_t* webServiceExePath;
    const wchar_t* webServiceExeDirectory;
};

WebDaemonService webDaemonServiceInstance;

ProcessDetail* pPhpProcessDetail = nullptr;
ProcessDetail* pMysqlProcessDetail = nullptr;
ProcessDetail* pMysqlClientProcessDetail = nullptr;
ProcessDetail* pWebServiceProcessDetail = nullptr;

DWORD phpProcess(ServiceUseConfig* serviceUse) {
    PathList* pathsPHP = initPathList();

    wchar_t phpDirectoryPath[512];
    swprintf_s(phpDirectoryPath, _countof(phpDirectoryPath), L"%ls/php/%ls", DIRECTORY_SERVICE, serviceUse->php);

    findFilesInDirectory(phpDirectoryPath, L"php-cgi.exe", pathsPHP);
    if (pathsPHP->count != 1) {
        LogAndMsgBox("%ls program as not found\r\n", serviceUse->php);
        return 1;
    }

    wchar_t phpRunCmd[1024] = { '\0' };
    swprintf_s(phpRunCmd, _countof(phpRunCmd),
        L"%ls -b 127.0.0.1:9000", pathsPHP->paths[0]);

    // Start php
    pPhpProcessDetail->processName = L"php-cgi.exe";
    pPhpProcessDetail->cmd = phpRunCmd;
    pPhpProcessDetail->dir = phpDirectoryPath;
    if (CalllCreateProcess(pPhpProcessDetail) == true) {
        AppendEditInfo(L"INFO: PHP runing.\r\n");
    }
    else {
        AppendEditInfo(L"INFO: PHP start fail.\r\n");
    }

    wchar_t phpBinDirectory[256];
    GetDirectoryFromPath(pathsPHP->paths[0], phpBinDirectory, sizeof(phpBinDirectory) / sizeof(wchar_t));

    webDaemonServiceInstance.phpExe = L"php-cgi.exe";
    webDaemonServiceInstance.phpExePath = _wcsdup(pathsPHP->paths[0]);
    webDaemonServiceInstance.phpExeDirectory = _wcsdup(phpBinDirectory);

    freePathList(pathsPHP);
    bPHPRunning = true;

    return 0;
}

DWORD mysqlClientProcess(ServiceUseConfig* serviceUse, bool bMysqlInit) {
    // or before mysql5.6.7
    if (bMysqlInit == true) {
        PathList* pathsMysqlClient = initPathList();

        wchar_t mysqlClientDirectoryPath[512];
        swprintf_s(mysqlClientDirectoryPath, _countof(mysqlClientDirectoryPath), L"%ls/mysql/%ls", DIRECTORY_SERVICE, serviceUse->mysql);

        findFilesInDirectory(mysqlClientDirectoryPath, L"mysql.exe", pathsMysqlClient);
        if (pathsMysqlClient->count != 1) {
            LogAndMsgBox("%ls program mysqlclient as not found.\r\n", serviceUse->mysql);
            return 1;
        }

        // format get bin directory
        wchar_t mysqlClientBinDirectory[1024] = { '\0' };
        GetDirectoryFromPath(pathsMysqlClient->paths[0], mysqlClientBinDirectory, sizeof(mysqlClientBinDirectory) / sizeof(wchar_t));

        // Initialized after mysql set password
        // initialized root user password
        wchar_t mysqlClientCmdInitRootUser[1024] = { '\0' };
        swprintf_s(mysqlClientCmdInitRootUser, _countof(mysqlClientCmdInitRootUser),
            L"%ls -u root -e \"ALTER USER 'root'@'localhost' IDENTIFIED BY 'root'; FLUSH PRIVILEGES;\"", pathsMysqlClient->paths[0]);

        pMysqlClientProcessDetail->processName = L"mysql.exe";
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

    wchar_t mysqldDirectoryPath[512];
    swprintf_s(mysqldDirectoryPath, _countof(mysqldDirectoryPath), L"%ls/mysql/%ls", DIRECTORY_SERVICE, serviceUse->mysql);

    findFilesInDirectory(mysqldDirectoryPath, L"mysqld.exe", pathsMysql);
    if (pathsMysql->count != 1) {
        LogAndMsgBox("%ls program mysqld as not found.\r\n", serviceUse->mysql);

        freePathList(pathsMysql);
        return 1;
    }

    // mysql data directory
    wchar_t mysqlDataDirectory[256];
    swprintf_s(mysqlDataDirectory, _countof(mysqlDataDirectory), L"%ls/data", mysqldDirectoryPath);

    // format get bin directory
    wchar_t mysqldBinDirectory[1024] = { '\0' };
    GetDirectoryFromPath(pathsMysql->paths[0], mysqldBinDirectory, sizeof(mysqldBinDirectory) / sizeof(wchar_t));

    // mysql initialized
    errno_t mysqlDataDirectoryCheck = _waccess_s(mysqlDataDirectory, 0);

    *bMysqlInit = false;
    if (mysqlDataDirectoryCheck != 0) {
        *bMysqlInit = true;

        Log("Initialize mysql.\r\n");
        ////////////////////////////////////////////////////////////////////////
        // No need to set a password --initialize-insecure,and not use --initialize
        // If you encounter an error, you can add the --console argument at the end to allow the console to output in real-time. This is specific to MYSQL.
        wchar_t mysqldCmdInitDataDirectory[1024] = { '\0' };
        swprintf_s(mysqldCmdInitDataDirectory, _countof(mysqldCmdInitDataDirectory),
            L"%ls --initialize-insecure --explicit_defaults_for_timestamp --datadir=data", pathsMysql->paths[0]);
        ProcessDetail* pMysqlInitProcessDetail = new ProcessDetail;

        pMysqlInitProcessDetail->processName = L"mysqld.exe";
        pMysqlInitProcessDetail->cmd = mysqldCmdInitDataDirectory;
        pMysqlInitProcessDetail->dir = mysqldBinDirectory;
        if (CalllCreateProcess(pMysqlInitProcessDetail, true) == true) {
            AppendEditInfo(L"INFO: Mysql initialized.\r\n");
        }
        else {
            AppendEditInfo(L"INFO: Mysql initialize fail.\r\n");
        }
        delete pMysqlInitProcessDetail;
        ////////////////////////////////////////////////////////////////////////
    }

    // Start mysql
    pMysqlProcessDetail->processName = L"mysqld.exe";
    wchar_t mysqldCmd[256];
    //swprintf_s(mysqldCmd, _countof(mysqldCmd), L"%ls --explicit_defaults_for_timestamp --console", pathsMysql->paths[0]);
    swprintf_s(mysqldCmd, _countof(mysqldCmd), L"%ls", pathsMysql->paths[0]);
    pMysqlProcessDetail->cmd = mysqldCmd;
    pMysqlProcessDetail->dir = mysqldBinDirectory;

    if (CalllCreateProcess(pMysqlProcessDetail) == true) {
        AppendEditInfo(L"INFO: Mysql runing.\r\n");
    }
    else {
        AppendEditInfo(L"INFO: Mysql start fail.\r\n");
    }

    webDaemonServiceInstance.mysqldExe = L"mysqld.exe";
    webDaemonServiceInstance.mysqldExePath = _wcsdup(pathsMysql->paths[0]);
    webDaemonServiceInstance.mysqldExeDirectory = _wcsdup(mysqldBinDirectory);

    bMysqlRunning = true;

    freePathList(pathsMysql);

    return 0;
}

DWORD webServiceProcess(ServiceUseConfig* serviceUse) {
    PathList* pathsWebservice = initPathList();

    const wchar_t* webServiceType;
    const wchar_t* webServiceBinExe;
    if (wcsncmp(serviceUse->webService, L"httpd", 5) == 0) {
        webServiceType = L"apache";
        webServiceBinExe = L"httpd.exe";
    }
    else {
        webServiceType = L"nginx";
        webServiceBinExe = L"nginx.exe";
    }

    wchar_t webServiceDirectoryPath[512];
    swprintf_s(webServiceDirectoryPath, _countof(webServiceDirectoryPath), L"%ls/%ls/%ls", DIRECTORY_SERVICE, webServiceType, serviceUse->webService);

    findFilesInDirectory(webServiceDirectoryPath, webServiceBinExe, pathsWebservice);

    if (pathsWebservice->count != 1) {
        LogAndMsgBox("%ls program as not found. \r\n", serviceUse->webService);
        return 1;
    }

    // Start webservice apache || nginx
    wchar_t webServiceBinDirectory[256];
    GetDirectoryFromPath(pathsWebservice->paths[0], webServiceBinDirectory, sizeof(webServiceBinDirectory) / sizeof(wchar_t));

    pWebServiceProcessDetail->processName = webServiceBinExe;
    pWebServiceProcessDetail->cmd = pathsWebservice->paths[0];
    pWebServiceProcessDetail->dir = webServiceBinDirectory;

    wchar_t* tempServiceProcessName;
    tempServiceProcessName = _wcsdup(webServiceType);

    tempServiceProcessName[0] = toupper(tempServiceProcessName[0]);

    if (CalllCreateProcess(pWebServiceProcessDetail) == true) {
        wchar_t successMsg[256];
        swprintf_s(successMsg, _countof(successMsg), L"INFO: %ls is running\r\n", tempServiceProcessName);
        AppendEditInfo(successMsg);
    }
    else {
        wchar_t failMsg[256];
        swprintf_s(failMsg, _countof(failMsg), L"INFO: %ls start fail\r\n", tempServiceProcessName);
        AppendEditInfo(failMsg);
    }

    webDaemonServiceInstance.webServiceExe = webServiceBinExe;
    webDaemonServiceInstance.webServiceExePath = _wcsdup(pathsWebservice->paths[0]);
    webDaemonServiceInstance.webServiceExeDirectory = _wcsdup(webServiceBinDirectory);

    bApacheRunning = webServiceType == L"apache" ? true : false;
    bNginxRunning = webServiceType == L"nginx" ? true : false;

    free(tempServiceProcessName);
    freePathList(pathsWebservice);

    return 0;
}


DWORD WINAPI DaemonServiceThread(LPVOID lParam) {
    pPhpProcessDetail = new ProcessDetail;
    pMysqlProcessDetail = new ProcessDetail;
    pMysqlClientProcessDetail = new ProcessDetail;
    pWebServiceProcessDetail = new ProcessDetail;

    ServiceUseConfig* serviceUse = (ServiceUseConfig*)malloc(sizeof(ServiceUseConfig));

    if (!GetServiceUseItem(serviceUse)) {
        return 1;
    }

    // ÇÐ»»apache ºÍ phpµÄÅäÖÃÄ£¿é
    if (wcsncmp(serviceUse->webService, L"httpd", 5) == 0) {
        if (wcsstr(serviceUse->php, L"nts") == NULL) {
            syncConfigFilePHPAndApache(serviceUse->php, serviceUse->webService);
        }
        else {
            free(serviceUse);

            LogAndMsgBox("php apache is not ts version.\r\n");
            return 1;
        }
    }

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

    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_START_SERVICE), FALSE);
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_REMOVE_CONFIG), FALSE);

    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_STOP_SERVICE), TRUE);
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_RESTART_SERVICE), TRUE);
    EnableWindow(GetDlgItem(hWndMain, IDC_LISTBOX_CONFIG), FALSE);

    free(serviceUse);

    return 0;
}

void freeWebDaemonServiceInstance(WebDaemonService* webDaemonService) {
    free((void*)webDaemonService->phpExePath);
    free((void*)webDaemonService->phpExeDirectory);

    free((void*)webDaemonService->mysqldExePath);
    free((void*)webDaemonService->mysqldExeDirectory);

    free((void*)webDaemonService->webServiceExePath);
    free((void*)webDaemonService->webServiceExeDirectory);
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
        //const wchar_t* processes[] = { L"php-cgi.exe", L"mysqld.exe", L"httpd.exe", L"nginx.exe"};
        const wchar_t* phpProcesses[] = { L"php-cgi.exe"};
        const wchar_t* mysqlProcesses[] = { L"mysqld.exe" };
        const wchar_t* apacheProcesses[] = { L"httpd.exe" };
        const wchar_t* nginxProcesses[] = { L"nginx.exe" };

        DWORD currentPHPHash = getTargetProcessesHash(phpProcesses, sizeof(phpProcesses) / sizeof(phpProcesses[0]));
        DWORD currentMysqlHash = getTargetProcessesHash(mysqlProcesses, sizeof(mysqlProcesses) / sizeof(mysqlProcesses[0]));
        DWORD currentApacheHash = getTargetProcessesHash(apacheProcesses, sizeof(apacheProcesses) / sizeof(apacheProcesses[0]));
        DWORD currentNginxHash = getTargetProcessesHash(nginxProcesses, sizeof(nginxProcesses) / sizeof(nginxProcesses[0]));

        if ((currentPHPHash != previousPHPHash || isFirstRun == true) && isSelfChildProcessOfCurrent(L"php-cgi.exe") == 2 && IsHttpdParentRunning(L"php-cgi.exe")) {
            AppendEditInfo(L"WARNING: PHP is not started by this program.\r\n");
        }

        if ((previousMysqlHash != currentMysqlHash || isFirstRun == true) && isSelfChildProcessOfCurrent(L"mysqld.exe") == 2 && IsHttpdParentRunning(L"mysqld.exe")) {
            AppendEditInfo(L"WARNING: Mysql is not started by this program.\r\n");
        }

        if ((previousApacheHash != currentApacheHash || isFirstRun == true) && isSelfChildProcessOfCurrent(L"httpd.exe") == 2 && IsHttpdParentRunning(L"httpd.exe")) {
            AppendEditInfo(L"WARNING: Apache is not started by this program.\r\n");
        }

        if ((previousNginxHash != currentNginxHash || isFirstRun == true) && isSelfChildProcessOfCurrent(L"nginx.exe") == 2 && IsHttpdParentRunning(L"nginx.exe")) {
            AppendEditInfo(L"WARNING: Nginx is not started by this program.\r\n");
        }

        // While it's running, monitor to see if the process has been closed due to interference from other processes.
        if (bPHPRunning && !ProcessIsRunning(L"php-cgi.exe")) {
            AppendEditInfo(L"ERROR: php-cgi.exe Unexpected exit.\r\n");
            bPHPRunning = false;
        }

        if (bMysqlRunning && !ProcessIsRunning(L"mysqld.exe")) {
            AppendEditInfo(L"ERROR: mysqld.exe Unexpected exit.\r\n");
            bMysqlRunning = false;
        }

        if (bApacheRunning && !ProcessIsRunning(L"httpd.exe")) {
            AppendEditInfo(L"ERROR: httpd.exe Unexpected exit.\r\n");
            bApacheRunning = false;
        }

        if (bNginxRunning && !ProcessIsRunning(L"nginx.exe")) {
            AppendEditInfo(L"ERROR: nginx.exe Unexpected exit.\r\n");
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
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    //ProcessDetail* pProcessDetail = (ProcessDetail*)lParam;
    size_t wCmdStrLen = wcslen(pProcessDetail.cmd) + 1;
    wchar_t* wCmdStr = (wchar_t*)malloc(wCmdStrLen * sizeof(wchar_t));
    if (!wCmdStr) {
        return 0;
    }
    wcscpy_s(wCmdStr, wCmdStrLen, pProcessDetail.cmd);

    if (!CreateProcess(NULL, wCmdStr, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, pProcessDetail.dir, &si, &pi)) {
        free(wCmdStr);
        return 0;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    wchar_t publicMsgInfo[256] = { '\0' };

    if (exitCode != 0) {
        Log("code:%lu\r\n", exitCode);
        swprintf_s(publicMsgInfo, sizeof(publicMsgInfo) / sizeof(wchar_t), L"INFO: %ls stop fail\r\n", pProcessDetail.serviceName);
        AppendEditInfo(publicMsgInfo);
    }
    else {
        swprintf_s(publicMsgInfo, sizeof(publicMsgInfo) / sizeof(wchar_t), L"INFO: %ls stop\r\n", pProcessDetail.serviceName);
        AppendEditInfo(publicMsgInfo);
    }

    free(wCmdStr);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 1;
}

void CloseDaemonService() {
    if (webDaemonServiceInstance.bRun != true) {
        return;
    }

    EnterCriticalSection(&daemonMonitorServiceCs);

    ProcessDetail pProcessDetail;

    wchar_t cmdService[256] = { '\0' };
    if (webDaemonServiceInstance.webServiceExe == L"nginx.exe") {
        swprintf_s(cmdService, sizeof(cmdService) / sizeof(wchar_t), L"%ls -s quit", webDaemonServiceInstance.webServiceExePath);

        pProcessDetail.serviceName = L"Nginx";
        pProcessDetail.processName = webDaemonServiceInstance.webServiceExe;
        pProcessDetail.cmd = cmdService;
        pProcessDetail.dir = webDaemonServiceInstance.webServiceExeDirectory;
        CallDeleteProcess(pProcessDetail);

        bNginxRunning = false;
    }
    else {
        // Terminating the Apache process requires administrator privileges.
        swprintf_s(cmdService, sizeof(cmdService) / sizeof(wchar_t), L"taskkill.exe /F /IM %ls", webDaemonServiceInstance.webServiceExe);
        pProcessDetail.serviceName = L"Apache";
        pProcessDetail.processName = webDaemonServiceInstance.webServiceExe;
        pProcessDetail.cmd = cmdService;
        //pProcessDetail.dir = webDaemonServiceInstance.webServiceExeDirectory;
        pProcessDetail.dir = L"C:/Windows/system/";
        CallDeleteProcess(pProcessDetail);

        bApacheRunning = false;
    }

    pProcessDetail.serviceName = L"Mysql";
    pProcessDetail.processName = L"mysqld.exe";
    pProcessDetail.cmd = L"taskkill.exe /F /IM mysqld.exe";
    pProcessDetail.dir = L"C:/Windows/system/";
    CallDeleteProcess(pProcessDetail);

    bMysqlRunning = false;

    pProcessDetail.serviceName = L"PHP";
    pProcessDetail.processName = L"php-cgi.exe";
    pProcessDetail.cmd = L"taskkill.exe /F /IM php-cgi.exe";
    pProcessDetail.dir = L"C:/Windows/system/";
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

    delete pPhpProcessDetail;
    delete pMysqlProcessDetail;
    delete pMysqlClientProcessDetail;
    delete pWebServiceProcessDetail;

    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_STOP_SERVICE), FALSE);
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_RESTART_SERVICE), FALSE);
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_START_SERVICE), TRUE);
    EnableWindow(GetDlgItem(hWndMain, IDC_LISTBOX_CONFIG), TRUE);
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_REMOVE_CONFIG), TRUE);

    LeaveCriticalSection(&daemonMonitorServiceCs);
}

void RestartDaemonService() {
    if (webDaemonServiceInstance.bRun != true) {
        return;
    }

    CloseDaemonService();

    StartDaemonService();
}
