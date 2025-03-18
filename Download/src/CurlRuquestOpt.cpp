#include "framework.h"
#include <wininet.h>
#include "BaseFileOpt.h"
#include "Log.h"
#include "CurlRequestOpt.h"
#include "curl/curl.h"
#include "DownloadThread.h"
#include "Common.h"
#include <math.h>
#include <io.h>

char* my_strndup(const char* s, size_t n) {
	size_t len = strnlen(s, n);
	char* new_str = (char*)malloc(len + 1);
	if (new_str == NULL) {
		return NULL;
	}
	strncpy_s(new_str, len + 1, s, len);
	new_str[len] = '\0';
	return new_str;
}

size_t subpart_write_data(void* buffer, size_t size, size_t nmemb, void* userp) {
	DownloadPart* part = (DownloadPart*)userp;

	if (part->statusCode != 206) {
		//Log("file:%s; nmemb:%llu; currentBytes:%llu;totalBytes:%llu; statusCode:%d\r\n",
		//	part->filename, nmemb, part->currentStartByte, part->totalBytesLength, part->statusCode);
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
	char* response_line = my_strndup(buffer, size * nitems);  // Use custom my_strndup function

	if (response_line != NULL) {
		if (strstr(response_line, "HTTP/") == response_line) {
			char* status_code_str = strchr(response_line, ' ') + 1; // Skip "HTTP/1.1"
			int status_code = atoi(status_code_str);

			part->statusCode = status_code;
		}

		free(response_line);
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
	curl_easy_setopt(part->easy_handle, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(part->easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(part->easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(part->easy_handle, CURLOPT_LOW_SPEED_LIMIT, 1024 * 256);
	curl_easy_setopt(part->easy_handle, CURLOPT_LOW_SPEED_TIME, 8);
}

DWORD CurlMultipleDownloadThread(LPVOID param, const int numSubPartSize) {
	EnterCriticalSection(&progressCriticalSection);
	if (numLockFlow >= numLockFlowMax) {
		LeaveCriticalSection(&progressCriticalSection);
		return 0;
	}

	numLockFlow += numSubPartSize;
	LeaveCriticalSection(&progressCriticalSection);

	DownloadPart** partGroup = (DownloadPart**)param;
	//ULONGLONG uTmp = GetTickCount64();

	curl_slist* headers = NULL;
	// Append headers to the list
	headers = curl_slist_append(headers, "Connection: keep-alive");
	headers = curl_slist_append(headers, "Cache-Control: no-cache");
	headers = curl_slist_append(headers, "Accept: */*");
	//headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
	//headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36");
	headers = curl_slist_append(headers, "User-Agent: curl/8.11.0");

	CURLM* multi_handle = curl_multi_init();

	for (int i = 0; i < numSubPartSize; i++) {
		// Here ab+N means that the files created and opened after the execution of fopen_s are not allowed 
		// to be occupied by sub-processes (N must be added because this piece is covered in the ProcessMode place).
		if (fopen_s(&partGroup[i]->file, partGroup[i]->filepath, "ab+N")) {
			partGroup[i]->status = -1;
			Log("timestamp: %llu;Failed to open file: %s errorCode:%d\r\n", partGroup[i]->timestamp, partGroup[i]->filepath, GetLastError());
			break;
		}

		//CURL* easy_handle;
		SetCurlComponent(partGroup[0]->url, partGroup[i], headers);

		curl_multi_add_handle(multi_handle, partGroup[i]->easy_handle);
	}

	int still_running = 1;
	do {
		curl_multi_setopt(multi_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, numLockFlowMax);
		curl_multi_setopt(multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, numLockFlowMax);

		int numfds;
		CURLMcode mc_wait = curl_multi_wait(multi_handle, NULL, 0, 100, &numfds);
		if (mc_wait != CURLM_OK) {
			numLockFlow -= numSubPartSize;
			Log("curl_multi_wait failed with code %d\n", mc_wait);
			break;
		}

		CURLMcode mc = curl_multi_perform(multi_handle, &still_running);

		int num_messages;
		CURLMsg* m;
		while ((m = curl_multi_info_read(multi_handle, &num_messages))) {
			if (m->msg == CURLMSG_DONE) {
				for (int i = 0; i < numSubPartSize; i++)
				{
					if (partGroup[i]->easy_handle == m->easy_handle) {
						// judge is last element
						if (m->data.result != CURLE_OK) {
							Log("Download detected and need to retry, CURLE_ERROR:%s\r\n", curl_easy_strerror(m->data.result));
							EnterCriticalSection(&progressCriticalSection);
							abnormalCount += 1;

							// mark next retry
							partGroup[i]->status = -1;
							LeaveCriticalSection(&progressCriticalSection);
						}
						else {
							EnterCriticalSection(&progressCriticalSection);
							partGroup[i]->status = 1;
							LeaveCriticalSection(&progressCriticalSection);
						}

						EnterCriticalSection(&progressCriticalSection);
						numLockFlow -= 1;
						LeaveCriticalSection(&progressCriticalSection);
					}
				}

				Log("SetEvent\r\n");
				SetEvent(hEvent);
			}
		}
	} while (still_running);

	for (int i = 0; i < numSubPartSize; i++) {
		if (partGroup[i]->file != NULL) {
			curl_multi_remove_handle(multi_handle, partGroup[i]->easy_handle);

			fclose(partGroup[i]->file);
		}

		curl_easy_cleanup(partGroup[i]->easy_handle);
	}

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

unsigned long long CurlGetRemoteFileSize(const char* url) {
	CURL* curl;
	CURLcode res;
	curl = curl_easy_init();
	if (!curl) {
		MessageBoxA(NULL, "CURL error", NULL, 0);
		Log("CURL create fail\r\n");
		return 0;
	}

	ULONGLONG fileSize = 0;
	curl_slist* headers = NULL;

	// Append headers to the list
	headers = curl_slist_append(headers, "Connection: keep-alive");
	headers = curl_slist_append(headers, "Cache-Control: no-cache");
	headers = curl_slist_append(headers, "Accept: */*");
	//headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
	//headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36");
	headers = curl_slist_append(headers, "User-Agent: curl/8.11.0");

	curl_easy_setopt(curl, CURLOPT_URL, url);
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
	//curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);

	//FILE* debug_file;
	//fopen_s(&debug_file, "curl_debug_output.txt", "w+");
	// 	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	//curl_easy_setopt(curl, CURLOPT_STDERR, debug_file);

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