#include "webrequest.h"
#include <assert.h>
#include <atomic>
#include <memory>
#include <functional>
#include <vector>

#include <Windows.h>
#include <winhttp.h>

#include <stdio.h>

#pragma comment(lib, "Winhttp.lib")

struct Worker
{
	HINTERNET hSession = nullptr;
	HINTERNET hConnect = nullptr;
	HINTERNET hRequest = nullptr;

	std::function<void (const bool done, const bool failure, void * bytes, const int numBytes, const float progress)> updateHandler;

	~Worker()
	{
		shut();
	}

	bool init(const char * url);
	bool initImpl(const char * url);
	void shut();

	void handleError(const char * errorMessage)
	{
		if (updateHandler != nullptr)
			updateHandler(true, true, nullptr, 0, 1.f);
	}

	void handleDone()
	{
		if (updateHandler != nullptr)
			updateHandler(true, false, nullptr, 0, 1.f);
	}
};

bool Worker::init(const char * url)
{
	if (initImpl(url) == false)
	{
		shut();

		return false;
	}
	else
	{
		return true;
	}
}

bool Worker::initImpl(const char * url)
{
	// todo : add WINHTTP_FLAG_ASYNC at some point

	hSession = WinHttpOpen(
		L"libwebrequest",  
		WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	if (hSession == nullptr)
	{
		handleError("failed to create WinHTTP session");
		return false;
	}

	// convert url to wide char

	const int kMaxLength = 4096;
	wchar_t url_wchar[kMaxLength];

	if (MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, url, -1, url_wchar, kMaxLength) == 0)
	{
		handleError("failed to create wchar url");
		return false;
	}

	// get server name and request path from url

	URL_COMPONENTS urlComp;
	ZeroMemory(&urlComp, sizeof(urlComp));
	
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.dwSchemeLength    = (DWORD)-1;
	urlComp.dwHostNameLength  = (DWORD)-1;
	urlComp.dwUrlPathLength   = (DWORD)-1;
	urlComp.dwExtraInfoLength = (DWORD)-1;

	if (WinHttpCrackUrl(url_wchar, wcslen(url_wchar), 0, &urlComp) == FALSE)
	{
		handleError("failed to split url into server name and request path");
		return false;
	}

	auto c = urlComp.lpszHostName[urlComp.dwHostNameLength];
	urlComp.lpszHostName[urlComp.dwHostNameLength] = 0;
	{
		hConnect = WinHttpConnect(
			hSession, urlComp.lpszHostName,
			INTERNET_DEFAULT_PORT,
			0);
	}
	urlComp.lpszHostName[urlComp.dwHostNameLength] = c;

	if (hConnect == nullptr)
	{
		handleError("failed to create WinHTTP connection");
		return false;
	}

	c = urlComp.lpszUrlPath[urlComp.dwUrlPathLength];
	urlComp.lpszUrlPath[urlComp.dwUrlPathLength] = 0;
	{
		hRequest = WinHttpOpenRequest(
			hConnect, L"GET",
			urlComp.lpszUrlPath,
			nullptr,
			WINHTTP_NO_REFERER, 
			WINHTTP_DEFAULT_ACCEPT_TYPES, 
			WINHTTP_FLAG_SECURE*0);
	}
	urlComp.lpszUrlPath[urlComp.dwUrlPathLength] = c;

	if (hRequest == nullptr)
	{
		handleError("failed to open WinHTTP request");
		return false;
	}

	if (WinHttpSetTimeouts(hRequest, 2000, 2000, 2000, 2000) == FALSE)
	{
		handleError("failed to set WinHTTP timeouts");
		return false;
	}

	BOOL bResult = WinHttpSendRequest(
		hRequest,
		WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0,
		0, 0);

	if (bResult == FALSE)
	{
		handleError("failed to send WinHTTP request");
		return false;
	}

	bResult = WinHttpReceiveResponse(hRequest, nullptr);

	if (bResult == FALSE)
	{
		handleError("failed to receive WinHTTP response");
		return false;
	}

	for (;;)
	{
		DWORD numBytesAvailable = 0;

		if (WinHttpQueryDataAvailable(hRequest, &numBytesAvailable) == FALSE)
		{
			handleError("failed to qeury WinHTTP data");
			return false;
		}

		// done ?

		if (numBytesAvailable == 0)
		{
			handleDone();
			break;
		}

		std::vector<uint8_t> buffer;
		buffer.resize(numBytesAvailable);

		DWORD numBytesDownloaded = 0;

		if (WinHttpReadData(hRequest, &buffer.front(), numBytesAvailable, &numBytesDownloaded) == FALSE)
		{
			handleError("failed to read WinHTTP data");
			return false;
		}
		else if (numBytesDownloaded != 0)
		{
			if (updateHandler != nullptr)
				updateHandler(false, false, &buffer.front(), numBytesDownloaded, 0.f);
		}
	}

	shut();

	return true;
}

void Worker::shut()
{
	if (hRequest != nullptr)
	{
		WinHttpCloseHandle(hRequest);
		hRequest = nullptr;
	}

	if (hConnect != nullptr)
	{
		WinHttpCloseHandle(hConnect);
		hConnect = nullptr;
	}

	if (hSession != nullptr)
	{
		WinHttpCloseHandle(hSession);
		hSession = nullptr;
	}
}

struct WebRequest_WinHttp : WebRequest
{
	std::string url;

	Worker worker;

	std::atomic<bool> failure;
	std::atomic<bool> done;
	std::atomic<int64_t> numBytesReceived;

	std::vector<uint8_t> contents;

	HANDLE thread;

	WebRequest_WinHttp(const char * in_url)
		: url(in_url)
		, worker()
		, failure(false)
		, done(false)
		, numBytesReceived(0)
		, thread(nullptr)
	{
		worker.updateHandler = [&](const bool done, const bool failure, void * bytes, const int numBytes, const float progress)
		{
			assert(this->done == false);

			contents.insert(contents.end(), (uint8_t*)bytes, (uint8_t*)bytes + numBytes);

			numBytesReceived += numBytes;

			if (failure)
			{
				this->failure = true;
			}

			if (done)
			{
				if (this->failure)
					contents.clear();

				//

				this->done = true;
			}
		};

		auto threadMain = [](void * userData) -> DWORD
		{
			WebRequest_WinHttp * self = (WebRequest_WinHttp*)userData;

			self->worker.init(self->url.c_str());

			return 0;
		};

		thread = CreateThread(
			nullptr,
			4096,
			threadMain,
			this,
			0,
			nullptr);
	}

	virtual ~WebRequest_WinHttp() override
	{
		if (thread != nullptr)
		{
			WaitForSingleObject(thread, INFINITE);
			thread = nullptr;
		}
	}

	virtual int getProgress() override
	{
		return numBytesReceived;
	}

	virtual bool isDone() override
	{
		return done;
	}

	virtual bool isSuccess() override
	{
		return failure == false;
	}

	virtual bool getResultAsData(uint8_t *& bytes, size_t & numBytes) override
	{
		if (done && failure == false)
		{
			bytes = new uint8_t[contents.size()];
			numBytes = contents.size();

			if (!contents.empty())
				memcpy(bytes, &contents.front(), contents.size());

			return true;
		}
		else
		{
			return false;
		}
	}

	virtual bool getResultAsCString(char *& result) override
	{
		if (done && failure == false)
		{
			result = new char[contents.size() + 1];

			if (!contents.empty())
				memcpy(result, &contents.front(), contents.size());

			result[contents.size()] = 0;

			return true;
		}
		else
		{
			return false;
		}
	}

	virtual void cancel()
	{
	}
};

WebRequest * createWebRequest(const char * url)
{
	return new WebRequest_WinHttp(url);
}
