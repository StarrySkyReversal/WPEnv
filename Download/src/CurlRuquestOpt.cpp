#include "framework.h"
#include <wininet.h>
#include "BaseFileOpt.h"
#include "Log.h"
#include "CurlRequestOpt.h"
#include "curl/curl.h"
#include "DownloadThread.h"
#include "Common.h"
#include <math.h>
#include "CircularQueue.h"
#include <io.h>

char* my_strndup(const char* s, size_t n) {
	size_t len = strnlen(s, n);  // 计算字符串的长度，但不超过n
	char* new_str = (char*)malloc(len + 1);  // 分配足够的空间（加1是为了null terminator）
	if (new_str == NULL) {
		return NULL;  // 内存分配失败
	}
	strncpy_s(new_str, len + 1, s, len);  // 复制字符串
	new_str[len] = '\0';  // 添加null terminator
	return new_str;
}

size_t subpart_write_data(void* buffer, size_t size, size_t nmemb, void* userp) {
	DownloadPart* part = (DownloadPart*)userp;

	if (part->statusCode != 206) {
		Log("file:%ls; nmemb:%llu; currentBytes:%llu;totalBytes:%llu; statusCode:%d\r\n",
			part->filename, nmemb, part->currentStartByte, part->totalBytesLength, part->statusCode);
		return 0;
	}

	size_t writeSize = fwrite(buffer, size, nmemb, part->file);

	part->readBytes += writeSize;
	part->currentStartByte += writeSize;

	EnterCriticalSection(&progressCriticalSection);
	downlodedTotalSize += writeSize;
	LeaveCriticalSection(&progressCriticalSection);

	return writeSize;
}

size_t header_status_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
	DownloadPart* part = (DownloadPart*)userdata;
	char* response_line = my_strndup(buffer, size * nitems);  // 使用自定义的my_strndup函数

	if (response_line != NULL) {
		// 如果这一行包含 HTTP 状态码
		if (strstr(response_line, "HTTP/") == response_line) {
			// 获取状态码
			char* status_code_str = strchr(response_line, ' ') + 1; // 跳过"HTTP/1.1" 
			int status_code = atoi(status_code_str);  // 转换状态码为整数

			// 检查状态码是否在 200-299 的范围内
			part->statusCode = status_code;
		}

		free(response_line);  // 不要忘记释放之前分配的内存
	}

	return size * nitems;
}

void SetCurlComponent(const char* url, DownloadPart* part, curl_slist* headers) {
	part->easy_handle = curl_easy_init();

	char range[64];
	snprintf(range, sizeof(range), "%llu-%llu", part->currentStartByte, part->endByte);

	curl_easy_setopt(part->easy_handle, CURLOPT_URL, url);
	
	curl_easy_setopt(part->easy_handle, CURLOPT_RANGE, range);
	curl_easy_setopt(part->easy_handle, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(part->easy_handle, CURLOPT_HEADERFUNCTION, header_status_callback);
	curl_easy_setopt(part->easy_handle, CURLOPT_HEADERDATA, part);

	curl_easy_setopt(part->easy_handle, CURLOPT_WRITEDATA, part);
	curl_easy_setopt(part->easy_handle, CURLOPT_WRITEFUNCTION, subpart_write_data);

	curl_easy_setopt(part->easy_handle, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(part->easy_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(part->easy_handle, CURLOPT_MAXREDIRS, 3L);
	curl_easy_setopt(part->easy_handle, CURLOPT_AUTOREFERER, 1L);
	curl_easy_setopt(part->easy_handle, CURLOPT_FORBID_REUSE, 1L);
	curl_easy_setopt(part->easy_handle, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(part->easy_handle, CURLOPT_CONNECTTIMEOUT, 3L);
	curl_easy_setopt(part->easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(part->easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);
}

DWORD CurlMultipleDownloadThread(LPVOID param, const int numSubPartSize) {
	DownloadPart** partGroup = (DownloadPart**)param;
	//ULONGLONG uTmp = GetTickCount64();

	char url[256];
	WToM(partGroup[0]->url, url, sizeof(url));

	//FILE* debugFile;
	//char tempMsg[256];
	//sprintf_s(tempMsg, sizeof(tempMsg), "downloads/a_debug_%llu.txt", part->startByte);
	//fopen_s(&debugFile, tempMsg, "wt");

	curl_slist* headers = NULL;
	// Append headers to the list
	headers = curl_slist_append(headers, "Connection: keep-alive");
	headers = curl_slist_append(headers, "Cache-Control: no-cache");
	headers = curl_slist_append(headers, "Accept: */*");
	//headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
	headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36");

	CURLM* multi_handle = curl_multi_init();

	for (int i = 0; i < numSubPartSize; i++) {
		//CURL* easy_handle;
		SetCurlComponent(url, partGroup[i], headers);

		Log("timestamp: %llu;count_filename:%ls\r\n", partGroup[i]->timestamp, partGroup[i]->filepath);
		if (_wfopen_s(&partGroup[i]->file, partGroup[i]->filepath, L"ab")) {  // ab
			Log("timestamp: %llu;Failed to open file: %ls errorCode:%d\r\n", partGroup[i]->timestamp, partGroup[i]->filepath, GetLastError());
			break;
		}

		curl_multi_add_handle(multi_handle, partGroup[i]->easy_handle);
	}

	int still_running = 1;
	do {
		CURLMcode mc = curl_multi_perform(multi_handle, &still_running);

		int num_messages;
		CURLMsg* m;
		while ((m = curl_multi_info_read(multi_handle, &num_messages))) {
			if (m->msg == CURLMSG_DONE) {
				for (int i = 0; i < numSubPartSize; i++) {
					if (partGroup[i]->easy_handle == m->easy_handle) {

						if (m->data.result != CURLE_OK) {
							Log("CURLE_ERROR:%s\r\n", curl_easy_strerror(m->data.result));
							EnterCriticalSection(&progressCriticalSection);
							// mark next retry
							partGroup[i]->status = -1;
							LeaveCriticalSection(&progressCriticalSection);
						}

						curl_multi_remove_handle(multi_handle, partGroup[i]->easy_handle);
						curl_easy_cleanup(partGroup[i]->easy_handle);
						fclose(partGroup[i]->file);

						EnterCriticalSection(&progressCriticalSection);
						numLockFlow -= 1;
						LeaveCriticalSection(&progressCriticalSection);
					}
				}
			}
		}
	} while (still_running);

	//fclose(debugFile);

	//for (int i = 0; i < numSubPartSize; i++) {
	//	curl_multi_remove_handle(multi_handle, partGroup[i]->easy_handle);
	//	curl_easy_cleanup(partGroup[i]->easy_handle);
	//	fclose(partGroup[i]->file);

	//	if ((partGroup[i]->endByte - partGroup[i]->startByte + 1) != partGroup[i]->readBytes) {
	//		Log("FileError:%ls;readBytes:%llu;totalSize:%llu;\r\n",
	//			partGroup[i]->filename, partGroup[i]->readBytes, (partGroup[i]->endByte - partGroup[i]->startByte + 1));
	//	}
	//}

	curl_slist_free_all(headers);
	curl_multi_cleanup(multi_handle);

	return 0;
}

size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
	long* pFilesize = (long*)userdata;

	if (strstr(buffer, "Content-Range")) {
		char* slash = strchr(buffer, '/');
		if (slash) {
			char* end;
			long filesize = strtol(slash + 1, &end, 10);
			if (end != slash + 1) {
				printf("File size: %ld bytes\n", filesize);
				*pFilesize = filesize;
			}
		}
	}

	return nitems * size;
}

unsigned long long CurlGetRemoteFileSize(const wchar_t* url) {
	CURL* curl;
	CURLcode res;
	curl = curl_easy_init();
	if (!curl) {
		Log("CURL create fail\r\n");
		return 0;
	}

	char mUrl[256];
	WToM(url, mUrl, sizeof(mUrl));

	ULONGLONG fileSize = 0;
	curl_slist* headers = NULL;

	// Append headers to the list
	headers = curl_slist_append(headers, "Connection: keep-alive");
	headers = curl_slist_append(headers, "Cache-Control: no-cache");
	headers = curl_slist_append(headers, "Accept: */*");
	//headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
	headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36");

	curl_easy_setopt(curl, CURLOPT_URL, mUrl);
	curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &fileSize);

	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirection if any
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
	curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		Log("curl_easy_perform() failed: %s\r\n", curl_easy_strerror(res));
		return 0;
	}

	curl_slist_free_all(headers);

	curl_easy_cleanup(curl);

	return fileSize;
}