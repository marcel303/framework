#include "Debugging.h"
#include "FileStream.h"
#include "Log.h"
#include "srt.h"
#include "StreamReader.h"

const SrtFrame * Srt::findFrameByTime(const double time) const
{
	for (auto & frame : frames)
	{
		if (time >= frame.time && time < frame.time + frame.duration)
			return &frame;
	}

	return nullptr;
}

enum ReadState
{
	kReadState_Number,
	kReadState_Time,
	kReadState_Text,
	kReadState_Error
};

static double convertTimeStampToSeconds(int h, int m, int s, int msec)
{
	return h * 3600 + m * 60 + s + msec / 1000.0;
}

bool loadSrt(const char * filename, Srt & srt)
{
	try
	{
		FileStream stream(filename, OpenMode_Read);
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();
		
		SrtFrame srtFrame;
		ReadState readState = kReadState_Number;
		
		for (size_t i = 0; i < lines.size(); ++i)
		{
			const std::string & line = lines[i];
			
			if (line.empty())
			{
				Assert(readState == kReadState_Text);
				srt.frames.push_back(srtFrame);
				srtFrame = SrtFrame();
				
				readState = kReadState_Number;
			}
			else
			{
				if (readState == kReadState_Number)
				{
					readState = kReadState_Time;
				}
				else if (readState == kReadState_Time)
				{
					// format = 00:00:14,600 --> 00:00:20,100

					int h1, m1, s1, msec1;
					int h2, m2, s2, msec2;

					if (sscanf_s(line.c_str(), "%02d:%02d:%02d,%d --> %02d:%02d:%02d,%d", &h1, &m1, &s1, &msec1, &h2, &m2, &s2, &msec2) == 8)
					{
						//LOG_DBG("time parse success!", 0);

						const double t1 = convertTimeStampToSeconds(h1, m1, s1, msec1);
						const double t2 = convertTimeStampToSeconds(h2, m2, s2, msec2);

						srtFrame.time = t1;
						srtFrame.duration = t2 - t1;

						readState = kReadState_Text;
					}
					else
					{
						LOG_WRN("time parse failure!", 0);

						readState = kReadState_Error;
					}
				}
				else if (readState == kReadState_Text)
				{
					srtFrame.lines.push_back(line);
				}
				else if (readState == kReadState_Error)
				{
				}
			}
		}
		
		if (readState == kReadState_Text)
		{
			srt.frames.push_back(srtFrame);
			srtFrame = SrtFrame();
		}
		
		return true;
	}
	catch (std::exception & e)
	{
		LOG_ERR(e.what());
		return false;
	}
}
