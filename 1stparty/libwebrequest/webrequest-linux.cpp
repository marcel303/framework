#include "webrequest.h"

#include <assert.h>
#include <string>

#if defined(WINDOWS)
	#include <winsock2.h>
#else
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <unistd.h> // close, sleep
	#include <netdb.h> // addrinfo

	#define closesocket close
#endif

// todo : rename implementation depending on api used: posix

#include <stdarg.h>
#include <stdio.h> // vsnprintf

#include <mutex>
#include <thread>

std::string ApplyFormat(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	
	return text;
}

struct WebRequest_Posix : WebRequest
{
	std::thread thread;
	std::mutex mutex;
	
	std::string url;
	std::string result;
	int progress = 0;
	int sock = -1;
	bool canceled = false;
	bool done = false;
	bool success = false;
	
	WebRequest_Posix(const char * in_url)
	{
		url = in_url;
		
		thread = std::thread([this]()
		{
			bool ok = true;
			
			// extract hostname. pattern = <protocol>://hostname[:port][/]
			
			const char * hostname_begin = url.c_str();
			
			while (hostname_begin[0] != 0)
			{
				// todo : check if the protocol is 'http'
				// todo : extract port number
				
				if (hostname_begin[0] == '/' && hostname_begin[1] == '/')
				{
					hostname_begin += 2;
					break;
				}
				
				hostname_begin++;
			}
			
			const char * hostname_end = hostname_begin;
			
			if (hostname_begin[0] != 0)
			{
				hostname_end++;
				
				while (hostname_end[0] != 0 && hostname_end[0] != '/' && hostname_end[0] != ':')
					hostname_end++;
			}
			
			const size_t hostname_len = hostname_end - hostname_begin;
			
			char hostname[1024];
			
			if (hostname_begin[0] != 0 && hostname_len > 0 && hostname_len < 1024)
			{
				memcpy(hostname, hostname_begin, hostname_len);
				hostname[hostname_len] = 0;
			}
			else
			{
				hostname[0] = 0;
				ok = false;
			}
			
			addrinfo * addrinfo = nullptr;
			
			if (ok)
			{
				// resolve hostname

				struct addrinfo baseaddr = { };
				baseaddr.ai_family = AF_INET;
				baseaddr.ai_socktype = SOCK_STREAM;

				if (getaddrinfo(hostname, "80", &baseaddr, &addrinfo) != 0)
				{
					//LOG_ERR("failed to resolve hostname");
					ok = false;
				}
			}
			
			// connect to the remote endpoint

			if (ok)
			{
				mutex.lock();
				{
					if (canceled == false)
					{
						sock = socket(addrinfo->ai_family, addrinfo->ai_socktype, 0);
					}
				}
				mutex.unlock();

				if (sock == -1)
					ok = false;
			}

			if (ok)
			{
				// avoid SIGPIPE signals from being generated. they will crash the program if left unhandled. we check error codes from send/recv and assume the user does so too, so we don't need signals to kill our app
				
				signal(SIGPIPE, SIG_IGN);
				
				if (connect(sock, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1)
				{
					//LOG_ERR("failed to connect socket");
					ok = false;
				}
			}
			
			if (ok)
			{
				// generate HTTP headers

				const auto hdr_method = ApplyFormat("%s %s HTTP/1.1\r\n",
					"GET",
					url.c_str());
				
				const auto hdr_host = ApplyFormat("Host: %s\r\n", hostname);

				const char * hdr_end = "\r\n\r\n";
				
				const auto hdrs =
					hdr_method +
					hdr_host +
					hdr_end;
				
				// send the request

				const char * bytes = hdrs.data();
				const char * bytes_end = bytes + hdrs.size();

				while (bytes != bytes_end)
				{
					assert(bytes < bytes_end);
					
					const auto numBytesRemaining = bytes_end - bytes;
					const int numBytesSent = send(sock, bytes, numBytesRemaining, 0);
					
					if (numBytesSent < 0)
					{
						//LOG_ERR("failed to send");
						ok = false;
						break;
					}

					bytes += numBytesSent;

					if (bytes == bytes_end)
					{
						break;
					}
				}
			}

			std::string response;

			int contentOffset = 0;
			bool hasContentOffset = false;
			
			int contentLength = 0;
			bool hasContentLength = false;
			
			if (ok)
			{
				// receive the response
				
			// todo : add Transfer-Encoding support ?
			
				for (;;)
				{
					char bytes[1024];

					const auto numBytesReceived = recv(sock, bytes, sizeof(bytes) / sizeof(bytes[0]), 0);

					// connection closed normally
					if (numBytesReceived == 0)
						break;

					// connection closed unexpectedly
					if (numBytesReceived < 0)
					{
						ok = false;
						break;
					}

					response.append(bytes, numBytesReceived);
					
					if (hasContentOffset == false)
					{
						while (contentOffset + 4 <= response.size())
						{
							if (response[contentOffset + 0] == '\r' &&
								response[contentOffset + 1] == '\n' &&
								response[contentOffset + 2] == '\r' &&
								response[contentOffset + 3] == '\n')
							{
								contentOffset += 4;
								hasContentOffset = true;
								break;
							}
							
							contentOffset++;
						}
						
						if (hasContentOffset)
						{
							// we reached the end of the headers
							
							// decode headers
							
							int begin = 0;
							int end = 0;
							
							bool firstLine = true;
							
							while (end + 1 < contentOffset)
							{
								// end of header line?
								
								if (response[end + 0] == '\r' &&
									response[end + 1] == '\n')
								{
									if (firstLine)
									{
									// todo : decode result code
										firstLine = false;
										end += 2;
										begin = end;
										continue;
									}
									
									// decode header key-value pair
									// format: key: value
									
									int keyBegin = begin;
									int keyEnd = begin;
									
									while (keyEnd < end)
									{
										if (response[keyEnd] == ':')
											break;
										else
											keyEnd++;
									}
									
									if (keyEnd == end)
									{
										// not a key-value pair
									}
									else
									{
										const int keyLength = keyEnd - keyBegin;
										
										int valueBegin = keyEnd + 1; // +1 to skip ':'
										int valueEnd = end;
										
										while (valueBegin < valueEnd)
										{
											if (response[valueBegin] == ' ')
												valueBegin++;
											else
												break;
										}
									
										const int valueLength = valueEnd - valueBegin;
										
									#define is_header(key) (keyLength == sizeof(key)-1 && memcmp(response.data() + keyBegin, key, keyLength) == 0)
										
										if (is_header("Content-Length") && valueLength < 256)
										{
											char value[256];
											memcpy(value, response.data() + valueBegin, valueLength);
											value[valueLength] = 0;
											
											// register content length
											contentLength = atoi(value);
											hasContentLength = true;
										}
										
									#undef is_header
									}
									
									end += 2;
									
									begin = end;
								}
								else
								{
									end++;
								}
							}
							
							// todo : if no content length, abort
						}
					}
					
					if (hasContentOffset)
					{
						const int receivedContentLength = response.size() - contentOffset;
						
						mutex.lock();
						{
							progress = receivedContentLength;
						}
						mutex.unlock();
					
						if (hasContentLength)
						{
							if (receivedContentLength >= contentLength)
							{
								break;
							}
						}
					}
				}
			}

			if (ok)
			{
				// todo : check response code is 200 (OK)
			}

			if (ok)
			{
				// decode result data
				
				if (hasContentOffset == false)
				{
					ok = false;
				}
				else
				{
					const char * result_start = response.data() + contentOffset;
				
					result = result_start;
				}
			}
			
			if (sock != -1)
			{
				// begin graceful shutdown TCP layer
				
				if (shutdown(sock, SHUT_WR) == -1)
				{
					//LOG_ERR("failed to shutdown TCP layer");
				}
				
				// wait for graceful shutdown TCP layer
				
				// perform recv on the socket. a recv of zero bytes means the underlying TCP
				// layer has completed the shutdown sequence. a recv of -1 means an error
				// has occurred. we want to keep recv'ing until either condition occurred
				char c;
				while (recv(sock, &c, 1, 0) == 1)
					sleep(0);
				
				// close the socket
				
				closesocket(sock);
				sock = -1;
			}
			
			mutex.lock();
			{
				done = true;
				
				success = ok;
			}
			mutex.unlock();
		});
	}

	virtual ~WebRequest_Posix() override
	{
		thread.join();
	}
	
    virtual int getProgress() override
	{
		int result;
		
		mutex.lock();
		{
			result = progress;
		}
		mutex.unlock();
		
		return result;
	}

    virtual bool isDone() override
	{
		bool result;
		
		mutex.lock();
		{
			result = done;
		}
		mutex.unlock();
		
		return result;
	}

    virtual bool isSuccess() override
	{
		bool result;
		
		mutex.lock();
		{
			result = success;
		}
		mutex.unlock();
		
		return result;
	}

    virtual bool getResultAsData(uint8_t *& bytes, size_t & numBytes) override
	{
		assert(isDone());
		if (!isDone())
			return false;
		
		if (success)
		{
			bytes = new uint8_t[result.size()];
			numBytes = result.size();
			
			memcpy(bytes, result.data(), result.size());
			
			return true;
		}
		else
		{
			bytes = nullptr;
			numBytes = 0;

			return false;
		}
	}

    virtual bool getResultAsCString(char *& cstring) override
	{
		assert(isDone());
		if (!isDone())
			return false;
		
		if (success)
		{
			cstring = new char[result.size() + 1];
			memcpy(cstring, result.data(), result.size());
			cstring[result.size()] = 0;
			
			return true;
		}
		else
		{
			cstring = nullptr;

			return false;
		}
	}

    virtual void cancel() override
	{
		mutex.lock();
		{
			canceled = true;
			
			if (sock != -1)
			{
				if (shutdown(sock, SHUT_RDWR) == -1)
				{
					// todo : log error
				}
			}
		}
		mutex.unlock();
	}
};

WebRequest * createWebRequest(const char * url)
{
	return new WebRequest_Posix(url);
}
