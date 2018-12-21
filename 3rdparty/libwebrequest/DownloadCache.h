#include <map>
#include <string>

struct WebRequest;

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

		int getProgress() const;
	};
	
	std::map<std::string, Elem> queuedElems;
	std::map<std::string, Elem> activeElems;
	
	std::map<std::string, bool> completions;

	void add(const char * url, const char * filename);
	
	bool isProcessing(const char * filename) const;
	bool isEmpty() const;
	
	void checkCompletions();
	void scheduleDownloads(const int maxActiveDownloads);
	
	void cancelActiveDownloads();
	void cancelQueuedDownloads();
	void clearQueuedDownloads();
	
	void tick(const int maxActiveDownloads);
};

struct DownloadCache
{
	DownloadQueue downloadQueue;
	
	std::map<std::string, bool> readyFiles;
	
	void tick(const int maxActiveDownloads);
	
	void add(const char * url, const char * filename);
	
	void cancel();
	void clear();
};
