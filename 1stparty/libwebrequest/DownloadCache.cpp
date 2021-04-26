#include "Debugging.h"
#include "DownloadCache.h"
#include "FileStream.h"
#include "Log.h"
#include "StreamWriter.h"
#include "webrequest.h"

int DownloadQueue::Elem::getProgress() const
{
	if (webRequest == nullptr)
		return 0;
	else
		return webRequest->getProgress();
}

int DownloadQueue::Elem::getExpectedSize() const
{
	if (webRequest == nullptr)
		return -1;
	else
		return webRequest->getExpectedSize();
}

void DownloadQueue::add(const char * url, const char * filename)
{
	Assert(queuedElems.count(filename) == 0);
	
	Elem elem;
	
	elem.url = url;
	elem.filename = filename;
	
	queuedElems[filename] = elem;
}

bool DownloadQueue::isProcessing(const char * filename) const
{
	return
		queuedElems.count(filename) != 0 ||
		activeElems.count(filename) != 0 ||
		completions.count(filename) != 0;
}

bool DownloadQueue::isEmpty() const
{
	return
		queuedElems.empty() &&
		activeElems.empty() &&
		completions.empty();
}

void DownloadQueue::checkCompletions()
{
	completions.clear();
	
	for (auto i = activeElems.begin(); i != activeElems.end(); )
	{
		const std::string & filename = i->first;
		Elem & activeElem = i->second;

		if (activeElem.webRequest == nullptr)
		{
			// canceled before it started
			
			completions.insert(std::make_pair(filename, false));

			i = activeElems.erase(i);
		}
		else if (activeElem.webRequest->isDone())
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

					delete [] bytes;
					bytes = nullptr;
				}
				
				completions.insert(std::make_pair(filename, true));
			}
			else
			{
				completions.insert(std::make_pair(filename, false));
			}

			delete activeElem.webRequest;
			activeElem.webRequest = nullptr;

			i = activeElems.erase(i);
		}
		else
		{
			++i;
		}
	}
}

void DownloadQueue::scheduleDownloads(const int maxActiveDownloads)
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

void DownloadQueue::cancelActiveDownloads()
{
	for (auto & activeElem : activeElems)
		activeElem.second.webRequest->cancel();
}

void DownloadQueue::cancelQueuedDownloads()
{
	for (auto & queuedElem : queuedElems)
		queuedElem.second.isSuccess = false;
	
	activeElems.insert(queuedElems.begin(), queuedElems.end());
	
	queuedElems.clear();
}

void DownloadQueue::clearQueuedDownloads()
{
	queuedElems.clear();
}

void DownloadQueue::tick(const int maxActiveDownloads)
{
	checkCompletions();
	
	scheduleDownloads(maxActiveDownloads);
}

//

void DownloadCache::tick(const int maxActiveDownloads)
{
	downloadQueue.tick(maxActiveDownloads);

	readyFiles.insert(downloadQueue.completions.begin(), downloadQueue.completions.end());
}

void DownloadCache::add(const char * url, const char * filename)
{
	if (readyFiles.count(filename) != 0 && readyFiles[filename] == true)
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
		
		readyFiles.insert(std::make_pair(filename, true));
	}
	else
	{
		// file doesn't exist yet. schedule a download
		
		readyFiles.erase(filename);
		
		downloadQueue.add(url, filename);
	}
}

void DownloadCache::cancel()
{
	downloadQueue.cancelActiveDownloads();
	downloadQueue.cancelQueuedDownloads();
}

void DownloadCache::clear()
{
	downloadQueue.cancelActiveDownloads();
	downloadQueue.clearQueuedDownloads();
	
	for (auto & readyFile : readyFiles)
		FileStream::Delete(readyFile.first.c_str());
	
	readyFiles.clear();
}
