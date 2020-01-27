#include "DownloadCache.h"
#include "framework.h"
#include "StringEx.h"
#include "webrequest.h"

/*
#include "FileStream.h"
#include "Log.h"
#include "StreamWriter.h"
#include <map>
#include <set>
#include <vector>
*/
int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	if (!framework.init(800, 600))
		return -1;
	
	DownloadCache downloadCache;

	auto addFilesToDownloadCache = [&]()
	{
		const char * url = "http://centuryofthecat.nl/shared_media/framework/tests/deepbelief/jetpac.ntwk";

		downloadCache.add(url, "jetpac.ntwk");
		
		for (int i = 0; i < 20; ++i)
		{
			const char * url = "http://webserver.com/";
			//const char * url = "http://yahoo.com/";
			const std::string filename = String::FormatC("testfile%03d.txt", i);
			
			downloadCache.add(url, filename.c_str());
		}
	};
	
	addFilesToDownloadCache();
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		if (framework.quitRequested)
			break;
		
		if (keyboard.wentDown(SDLK_r))
		{
			downloadCache.clear();
			
			addFilesToDownloadCache();
		}
		
		if (keyboard.wentDown(SDLK_c))
		{
			downloadCache.cancel();
		}
		
		if (keyboard.wentDown(SDLK_k))
		{
			downloadCache.downloadQueue.cancelActiveDownloads();
			downloadCache.downloadQueue.clearQueuedDownloads();
		}
		
		if (keyboard.wentDown(SDLK_a))
		{
			addFilesToDownloadCache();
		}
		
		downloadCache.tick(2);
		
		if (downloadCache.downloadQueue.isEmpty() == false)
			logInfo("downloading..", 0);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			
			int y = 10;
			
			setColor(200, 200, 200);
			drawText(10, y, 12, +1, +1, "Press 'R' to clear download cache and reschedule downloads");
			y += 20;
			drawText(10, y, 12, +1, +1, "Press 'C' to cancel active download(s)");
			y += 20;
			drawText(10, y, 12, +1, +1, "Press 'K' to cancel active download(s) and clear download queue");
			y += 20;
			drawText(10, y, 12, +1, +1, "Press 'A' to schedule downloads");
			y += 20;
			
			for (auto i : downloadCache.downloadQueue.activeElems)
			{
				auto & e = i.second;
				
				setColor(colorYellow);
				drawText(10, y, 12, +1, +1, "[downloading %dkb] %s -> %s",
					e.webRequest->getProgress() / 1024,
					e.url.c_str(),
					e.filename.c_str());
				
				y += 14;
			}
			
			for (auto i : downloadCache.downloadQueue.queuedElems)
			{
				auto & e = i.second;
				
				setColor(200, 200, 200);
				drawText(10, y, 12, +1, +1, "[queued] %s -> %s", e.url.c_str(), e.filename.c_str());
				y += 14;
			}
			
			for (auto & readyFile : downloadCache.readyFiles)
			{
				auto & filename = readyFile.first;
				auto isSuccess = readyFile.second;
				
				if (isSuccess)
					setColor(220, 220, 220);
				else
					setColor(220, 180, 180);
				
				drawText(10, y, 12, +1, +1, "[%s] %s", isSuccess ? "ready" : "failed", filename.c_str());
				y += 14;
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
