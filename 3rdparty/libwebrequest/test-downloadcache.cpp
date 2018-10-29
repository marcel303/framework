#include "framework.h"
#include "StringEx.h"
#include "webrequest.h"

#include "FileStream.h"
#include "Log.h"
#include "StreamWriter.h"
#include <map>
#include <set>
#include <vector>

struct DownloadQueue
{
	struct Elem
	{
		std::string url;
		std::string filename;
		
		WebRequest * webRequest = nullptr;
		
		bool isSuccess = false;
		
		uint8_t * bytes = nullptr;
		size_t numBytes = 0;
	};
	
	std::map<std::string, Elem> queuedElems;
	std::map<std::string, Elem> activeElems;
	
	std::set<std::string> completions;

	void add(const char * url, const char * filename)
	{
		Assert(queuedElems.count(filename) == 0);
		
		Elem elem;
		
		elem.url = url;
		elem.filename = filename;
		
		queuedElems[filename] = elem;
	}
	
	bool isProcessing(const char * filename) const
	{
		return
			queuedElems.count(filename) != 0 ||
			activeElems.count(filename) != 0 ||
			completions.count(filename) != 0;
	}
	
	bool isEmpty() const
	{
		return
			queuedElems.empty() &&
			activeElems.empty() &&
			completions.empty();
	}
	
	void checkCompletions()
	{
		completions.clear();
		
		for (auto i = activeElems.begin(); i != activeElems.end(); )
		{
			const std::string & filename = i->first;
			const Elem & activeElem = i->second;

			if (activeElem.webRequest->isDone())
			{
				if (activeElem.webRequest->isSuccess())
				{
					uint8_t * bytes;
					size_t numBytes;

					if (activeElem.webRequest->getResultAsData(bytes, numBytes))
					{
						try
						{
							FileStream stream(filename.c_str(), OpenMode_Write);
							StreamWriter writer(&stream, false);
							
							writer.WriteBytes(bytes, numBytes);
							
							stream.Close();
						}
						catch (std::exception & e)
						{
							LOG_ERR("failed to write download to disk: %s", e.what());
						}
					}
				}

				completions.insert(filename);

				i = activeElems.erase(i);
			}
			else
			{
				++i;
			}
		}
	}

	void scheduleDownloads(const int maxActiveDownloads)
	{
		while (activeElems.size() < maxActiveDownloads && !queuedElems.empty())
		{
			auto scheduledElemItr = queuedElems.begin();
			{
				const std::string & filename = scheduledElemItr->first;
				Elem & scheduledElem = scheduledElemItr->second;
				
				Elem activeElem = scheduledElem;

				activeElem.webRequest = createWebRequest(scheduledElem.url.c_str());

				activeElems[filename] = activeElem;
			}
			queuedElems.erase(scheduledElemItr);
		}
	}
	
	void tick(const int maxActiveDownloads)
	{
		checkCompletions();
		
		scheduleDownloads(maxActiveDownloads);
	}
};

struct DownloadCache
{
	DownloadQueue downloadQueue;
	
	std::set<std::string> readyFiles;
	
	void tick(const int maxActiveDownloads)
	{
		downloadQueue.tick(maxActiveDownloads);

		readyFiles.insert(downloadQueue.completions.begin(), downloadQueue.completions.end());
	}
	
	void add(const char * url, const char * filename)
	{
		if (readyFiles.count(filename) != 0)
		{
			// already completed
		}
		else if (downloadQueue.isProcessing(filename))
		{
			// already being downloaded
		}
		else if (FileStream::Exists(filename))
		{
			// already exists on disk
			
			readyFiles.insert(filename);
		}
		else
		{
			// file doesn't exist yet. schedule a download
			
			downloadQueue.add(url, filename);
		}
	}
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	if (!framework.init(800, 600))
		return -1;
	
	DownloadCache downloadCache;

	const char * url = "http://centuryofthecat.nl/shared_media/framework/tests/deepbelief/jetpac.ntwk";

	downloadCache.add(url, "jetpac.ntwk");
	
	for (int i = 0; i < 100; ++i)
	{
		const char * url = "http://webserver.com/";
		const std::string filename = String::FormatC("testfile%03d.txt", i);
		
		downloadCache.add(url, filename.c_str());
	}
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		downloadCache.tick(10);
		
		if (downloadCache.downloadQueue.isEmpty())
			break;
		
		LOG_DBG("downloading..", 0);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			
			int y = 10;
			
			for (auto i : downloadCache.downloadQueue.activeElems)
			{
				auto & e = i.second;
				
				setColor(colorYellow);
				drawText(10, y, 12, +1, +1, "[downloading %d%%] %s -> %s",
					int(e.webRequest->getProgress() * 100.f),
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
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
