#include "webrequest.h"
#include <stdio.h>

int main(int argc, char * argv[])
{
	auto webRequest = createWebRequest("https://www.amazon.co.uk/");
	
	for (;;)
	{
		if (webRequest->isDone())
			break;
		
		printf("progress: %.2f%%\n", webRequest->getProgress() * 100.f);
	}
	
	char * buffer = nullptr;
	
	if (webRequest->getResultAsCString(buffer))
	{
		printf("%s\n", buffer);
		
		delete [] buffer;
		buffer = nullptr;
	}
	
	uint8_t * bytes = nullptr;
	size_t numBytes = 0;
	
	if (webRequest->getResultAsData(bytes, numBytes))
	{
		printf("numBytes: %zU\n", numBytes);
		
		delete [] bytes;
		bytes = nullptr;
	}
	
	return 0;
}
