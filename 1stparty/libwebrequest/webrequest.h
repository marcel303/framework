#pragma once

#include <stdint.h>
#include <stdlib.h>

struct WebRequest
{
	virtual ~WebRequest() { }
	
    virtual int getProgress() = 0;
    virtual int getExpectedSize() = 0;
    virtual bool isDone() = 0;
    virtual bool isSuccess() = 0;
    virtual bool getResultAsData(uint8_t *& bytes, size_t & numBytes) = 0;
    virtual bool getResultAsCString(char *& result) = 0;
    virtual void cancel() = 0;
};

WebRequest * createWebRequest(const char * url);
