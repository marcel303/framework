#pragma once

#include <stdlib.h>

struct WebRequest
{
	virtual ~WebRequest() { }
	
    virtual float getProgress() = 0;
    virtual bool isDone() = 0;
    virtual bool isSuccess() = 0;
    virtual bool getResultAsData(uint8_t *& bytes, size_t & numBytes) = 0;
    virtual bool getResultAsCString(char *& result) = 0;
};

WebRequest * createWebRequest(const char * url);
