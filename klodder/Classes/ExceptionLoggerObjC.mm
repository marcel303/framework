#import "Exception.h"
#import "ExceptionLogger.h"
#import "ExceptionLoggerObjC.h"

void ExceptionLoggerObjC::Log(NSException* e)
{
	ExceptionLogger::Log(ExceptionVA([e.reason cStringUsingEncoding:NSASCIIStringEncoding]));
}
