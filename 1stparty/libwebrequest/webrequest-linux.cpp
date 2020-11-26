#include "webrequest.h"

// libc++ includes
#include <mutex>
#include <string>
#include <thread>

// libc includes
#include <assert.h>
#include <stdarg.h> // va_*
#include <stdio.h> // vsnprintf

// socket includes
#if defined(WINDOWS)
	#include <winsock2.h>
#else
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <unistd.h> // close
	#include <netdb.h> // addrinfo

	#define closesocket close
#endif

// todo : rename implementation depending on api used: posix

std::string ApplyFormat(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	const int n = vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	
	assert(n >= 0 && n < 1024);
	if (n < 0 || n >= 1024)
		text[0] = 0;
	
	return text;
}

struct WebRequest_Posix : WebRequest
{
	std::thread thread;
	std::mutex mutex;
	
	std::string url;
	std::string result;
	
	int sock = -1;
	int progress = 0;
	
	bool canceled = false;
	bool done = false;
	bool success = false;
	
	WebRequest_Posix(const char * in_url)
	{
		url = in_url;
		
		thread = std::thread([this]()
		{
			bool ok = true;
			
			// extract protocol, hostname, port number
			// pattern = <protocol>://<hostname>[:<port>][/]
			
			const char * url_ptr = url.c_str();
			
			if (ok)
			{
				const char * protocol_begin = url_ptr;
				const char * protocol_end = protocol_begin;
			
				// extract protocol
				
				while (protocol_end[0] != 0)
				{
					if (protocol_end[0] == ':' &&
						protocol_end[1] == '/' &&
						protocol_end[2] == '/')
					{
						break;
					}
					
					protocol_end++;
				}
				
				// check if we found the protocol
				
				if (protocol_end[0] == 0)
				{
					// invalid url. expected protocol
					
					ok = false;
				}
				else
				{
					// check if the protocol is 'http'
					
					const int protocol_length = protocol_end - protocol_begin;
					assert(protocol_length >= 0);
					
					if (protocol_length == 0)
					{
						// invalid url. protocol missing
						
						ok = false;
					}
					else if (
					// todo : case insensitivity
						protocol_length != 4 ||
						memcmp(protocol_begin, "http", 4) != 0)
					{
						// unsupported protocol
						
						ok = false;
					}
				}
				
				if (ok)
				{
					// skip '://'
					protocol_end += 3;
					
					// advance url pointer
					url_ptr = protocol_end;
				}
			}
			
			char hostname[1024];
			hostname[0] = 0;
			
			if (ok)
			{
				const char * hostname_begin = url_ptr;
				const char * hostname_end = hostname_begin;
			
				// extract hostname
				
				while (
					hostname_end[0] != 0 &&
					hostname_end[0] != '/' &&
					hostname_end[0] != ':')
				{
					hostname_end++;
				}
			
				const int hostname_length = hostname_end - hostname_begin;
				assert(hostname_length >= 0);
				
				// check if we found the hostname
				
				if (hostname_length == 0)
				{
					// hostname missing
					
					ok = false;
				}
				else if (hostname_length >= 1024)
				{
					// hostname too long
					
					ok = false;
				}
				else
				{
					// register hostname
					
					memcpy(
						hostname,
						hostname_begin,
						hostname_length);
					hostname[hostname_length] = 0;
				}
				
				if (ok)
				{
					// advance url pointer
					
					url_ptr = hostname_end;
				}
			}
			
			char portnumber[32];
			strcpy(portnumber, "80");
			
			// does the url have an optional port number?
			
			if (ok && url_ptr[0] == ':')
			{
				const char * portnumber_begin = url_ptr + 1;
				const char * portnumber_end = portnumber_begin;
			
				// extract the port number
				
				while (
					portnumber_end[0] != 0 &&
					portnumber_end[0] != '/')
				{
					portnumber_end++;
				}
				
				const int portnumber_length = portnumber_end - portnumber_begin;
				assert(portnumber_length >= 0);
				
				if (portnumber_length == 0)
				{
					// port number missing
					
					ok = false;
				}
				else if (portnumber_length >= 32)
				{
					// port number too long
					
					ok = false;
				}
				else
				{
					// register port number
					
					memcpy(
						portnumber,
						portnumber_begin,
						portnumber_length);
					portnumber[portnumber_length] = 0;
				}
				
				// we're done parsing the url. reset url pointer
				
				url_ptr = nullptr;
			}
			
			addrinfo * addrinfo = nullptr;
			
			if (ok)
			{
				// resolve hostname

				struct addrinfo baseaddr = { };
				baseaddr.ai_family = AF_INET;
				baseaddr.ai_socktype = SOCK_STREAM;

				if (getaddrinfo(
					hostname,
					portnumber,
					&baseaddr,
					&addrinfo) != 0)
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
				{
					// webrequest got canceled
					
					ok = false;
				}
			}

			if (ok)
			{
				// avoid SIGPIPE signals from being generated. they will crash the program if left unhandled. we check error codes from send/recv and assume the user does so too, so we don't need signals to kill our app
				
				signal(SIGPIPE, SIG_IGN);
				
				// connect to remote endpoint
				
				if (connect(
					sock,
					addrinfo->ai_addr,
					addrinfo->ai_addrlen) == -1)
				{
					//LOG_ERR("failed to connect to remote endpoint");
					
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

				const auto hdr_end = "\r\n\r\n";
				
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

					const int numBytesReceived =
						recv(
							sock,
							bytes,
							sizeof(bytes) / sizeof(bytes[0]),
							0);

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
									// todo : check response code is 200 (OK)
										firstLine = false;
										end += 2; // skip \r\n
										begin = end;
										continue;
									}
									
									// decode header key-value pair
									// format: <key>: <value>
									
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
										// ':' not found. not a key-value pair
									}
									else
									{
										const int keyLength = keyEnd - keyBegin;
										
										int valueBegin = keyEnd + 1; // +1 to skip ':'
										int valueEnd = end;
										
										// trim white space before the value
										
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
									
									end += 2; // skip \r\n
									
									begin = end;
								}
								else
								{
									end++;
								}
							}
							
							if (hasContentLength == false)
							{
								// missing content length
								
								ok = false;
								break;
							}
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
				{
					// (flush)
				}
				
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
