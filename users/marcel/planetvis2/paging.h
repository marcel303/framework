#pragma once

#include <SDL2/SDL.h>
#include <stdint.h>

struct PageData;
struct PageIoManager;
struct PageIoRequest;

struct PageData
{
	static const int kPageSize = 256;

	uint8_t bytes[kPageSize * kPageSize];
};

bool pageFilename(const char * name, const int baseSize, const int pageSize, const int cubeSide, const int levelSize, const int pageX, const int pageY, char * filename, const int filenameSize);
bool pageSave(const char * name, const int baseSize, const int pageSize, const int cubeSide, const int levelSize, const int pageX, const int pageY, PageData & pageData);
bool pageLoad(const char * name, const int baseSize, const int pageSize, const int cubeSide, const int levelSize, const int pageX, const int pageY, PageData & pageData);

bool pageSave(const char * filename, PageData & pageData);
bool pageLoad(const char * filename, PageData & pageData);

void pagingInit();
void pagingShut();

//

int pageRequest(const char * filename);
void pageRequestAbort(const int id);
PageData * pageRequestPeek(const int id);
void pageRequestConsume(const int id);

//

struct PageIoRequest
{
	char filename[256];
	bool isActive;
	bool isReady;
	bool isAborted;

	PageData pageData;
	
	PageIoRequest();
	
	void abort(PageIoManager * ioMgr);
	void consume(PageIoManager * ioMgr);
};

struct PageIoManager
{
	//static const int kMaxPageRequests = 1024;
	static const int kMaxPageRequests = 256;
	
	PageIoRequest requests[kMaxPageRequests];
	int wRequestIndex;
	int rRequestIndex;
	
	SDL_cond * cond;
	SDL_mutex * mutex;
	SDL_Thread * thread;
	bool threadStop;
	
	PageIoManager();

	void init();
	void shut();
	
	void condEnter();
	void condLeave();
	void condWait();

	void threadMain();

	PageIoRequest * getRequest(const int id);
	int addRequest(const char * filename);
};

//

extern PageIoManager * pageIoMgr;

//

void testPageIo();
