#pragma once

#import <Foundation/NSException.h>
#import "ExceptionLogger.h"

class ExceptionLoggerObjC
{
public:
	static void Log(NSException* e);
};

#define HandleExceptionObjcBegin() \
	try \
	{ \
		@try \
		{

#define HandleExceptionObjcEnd(rethrow) \
		} \
		@catch (NSException* e) \
		{ \
			ExceptionLoggerObjC::Log(e); \
			if (rethrow) \
				@throw e; \
		} \
	} \
	catch (std::exception& e) \
	{ \
		ExceptionLogger::Log(e); \
		if (rethrow) \
			throw e; \
	}
