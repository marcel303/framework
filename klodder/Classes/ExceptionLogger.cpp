#include <time.h>
#include "ExceptionLogger.h"
#include "FileStream.h"
#include "KlodderSystem.h"
#include "Log.h"
#include "StreamWriter.h"

void ExceptionLogger::Log(const std::exception & e)
{
	LOG_ERR("error: %s", e.what());
	
	const std::string fileName = gSystem.GetDocumentPath("exception.log");
	
	FileStream stream;
	
	stream.Open(fileName.c_str(), (OpenMode)(OpenMode_Write | OpenMode_Append));
	
	StreamWriter writer(&stream, false);
	
	const time_t t = time(0);
	
	const std::string dateString = asctime(localtime(&t));
	
	writer.WriteLine(dateString + e.what());
}
