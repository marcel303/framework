#include "Debugging.h"
#include "framework.h"
#include "paging.h"
#include "StringEx.h"
#include <stdio.h>
#include <string.h>

#if defined(DEBUG)
	#define ENABLE_TIMING 1
#else
	#define ENABLE_TIMING 0
#endif

#if ENABLE_TIMING
	#include "Timer.h"
#endif

// todo-planetvis : use a single page file, use block-aligned IO
// todo-planetvis : make page file location an init option

PageIoManager * pageIoMgr = nullptr;

//

bool pageFilename(const char * name, const int baseSize, const int pageSize, const int cubeSide, const int levelSize, const int pageX, const int pageY, char * filename, const int filenameSize)
{
	sprintf_s(filename, filenameSize, "heightmap/%s-%06d-%04d-%01d-%06d-%03d-%03d",
	//sprintf_s(filename, filenameSize, "C:/PlanetVisHeightmap/%s-%06d-%04d-%01d-%06d-%03d-%03d",
		name,
		baseSize,
		pageSize,
		cubeSide,
		levelSize,
		pageX,
		pageY);

	return true;
}

bool pageSave(const char * name, const int baseSize, const int pageSize, const int cubeSide, const int levelSize, const int pageX, const int pageY, PageData & pageData)
{
	bool result = true;

	char filename[1024];

	if (result)
	{
		result &= pageFilename(name, baseSize, pageSize, cubeSide, levelSize, pageX, pageY, filename, sizeof(filename));
	}

	if (result)
	{
		result &= pageSave(filename, pageData);
	}

	return result;
}

bool pageLoad(const char * name, const int baseSize, const int pageSize, const int cubeSide, const int levelSize, const int pageX, const int pageY, PageData & pageData)
{
	bool result = true;

	char filename[1024];

#if ENABLE_TIMING
	const int64_t t1 = g_TimerRT.TimeUS_get();
#endif

	if (result)
	{
		result &= pageFilename(name, baseSize, pageSize, cubeSide, levelSize, pageX, pageY, filename, sizeof(filename));
	}

#if ENABLE_TIMING
	const int64_t t2 = g_TimerRT.TimeUS_get();
#endif
	
	if (result)
	{
		result &= pageLoad(filename, pageData);

		if (!result)
			logError("failed to load page %s", filename);
	}

#if ENABLE_TIMING
	const int64_t t3 = g_TimerRT.TimeUS_get();
#endif

#if ENABLE_TIMING
	printf("pageLoad took %dus + %dus\n", int(t2 - t1), int(t3 - t2));
#endif

	return result;
}

bool pageSave(const char * filename, PageData & pageData)
{
	bool result = true;

	FILE * f = fopen(filename, "wb");

	result &= f != nullptr;

	if (result)
	{
		result &= fwrite(pageData.bytes, sizeof(pageData.bytes), 1, f) == 1;

		if (!result)
			logError("failed to save page %s", filename);
	}

	if (f != nullptr)
	{
		fclose(f);
		f = nullptr;
	}

	return result;
}

bool pageLoad(const char * filename, PageData & pageData)
{
	bool result = true;

#if ENABLE_TIMING
	const int64_t t1 = g_TimerRT.TimeUS_get();
#endif

	FILE * f = fopen(filename, "rb");

	result &= f != nullptr;

#if ENABLE_TIMING
	const int64_t t2 = g_TimerRT.TimeUS_get();
#endif

	if (result)
	{
		result &= fread(pageData.bytes, sizeof(pageData.bytes), 1, f) == 1;

#if 0
		for (auto & b : pageData.bytes)
			b = rand();
#endif
	}

#if ENABLE_TIMING
	const int64_t t3 = g_TimerRT.TimeUS_get();
#endif

	if (f != nullptr)
	{
		fclose(f);
		f = nullptr;
	}

#if ENABLE_TIMING
	const int64_t t4 = g_TimerRT.TimeUS_get();
#endif

#if ENABLE_TIMING
	printf("pageLoad took %dus + %dus + %dus\n", int(t2 - t1), int(t3 - t2), int(t4 - t3));
#endif

	return result;
}

//

void pagingInit()
{
	Assert(pageIoMgr == nullptr);
	
	pageIoMgr = new PageIoManager();

	pageIoMgr->init();
}

void pagingShut()
{
	Assert(pageIoMgr != nullptr);

	pageIoMgr->shut();

	delete pageIoMgr;
	pageIoMgr = nullptr;
}

int pageRequest(const char * filename)
{
	return pageIoMgr->addRequest(filename);
}

void pageRequestAbort(const int id)
{
	auto request = pageIoMgr->getRequest(id);
	
	request->abort(pageIoMgr);
}

PageData * pageRequestPeek(const int id)
{
	auto request = pageIoMgr->getRequest(id);
	
	if (request->isReady)
		return &request->pageData;
	else
		return nullptr;
}

void pageRequestConsume(const int id)
{
	auto request = pageIoMgr->getRequest(id);
	
	request->consume(pageIoMgr);
}

//

PageIoRequest::PageIoRequest()
{
	memset(this, 0, sizeof(*this));
}

void PageIoRequest::abort(PageIoManager * ioMgr)
{
	Assert(isActive);

	ioMgr->condEnter();
	{
		if (isReady)
		{
			consume(ioMgr);
		}
		else
		{
			isAborted = true;
		}
	}
	ioMgr->condLeave();
}

void PageIoRequest::consume(PageIoManager * ioMgr)
{
	ioMgr->condEnter();
	{
		Assert(isActive);
		Assert(isReady);
		Assert(!isAborted);

		isActive = false;
		isReady = false;
	}
	ioMgr->condLeave();
}

//

PageIoManager::PageIoManager()
	: wRequestIndex(0)
	, rRequestIndex(0)
	, cond(nullptr)
	, mutex(nullptr)
	, thread(nullptr)
	, threadStop(false)
{
}

static int staticThreadMain(void * obj)
{
	PageIoManager * self = (PageIoManager*)obj;
	
	self->threadMain();

	return 0;
}

void PageIoManager::init()
{
	cond = SDL_CreateCond();
	mutex = SDL_CreateMutex();
	
	thread = SDL_CreateThread(staticThreadMain, "PageIoThread", this);
}

void PageIoManager::shut()
{
	threadStop = true;
	SDL_CondSignal(cond);
	SDL_WaitThread(thread, 0);

	if (cond != nullptr)
	{
		SDL_DestroyCond(cond);
		cond = nullptr;
	}

	if (mutex != nullptr)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
}

void PageIoManager::condEnter()
{
	SDL_LockMutex(mutex);
}

void PageIoManager::condLeave()
{
	SDL_UnlockMutex(mutex);
}

void PageIoManager::condWait()
{
	SDL_CondWait(cond, mutex);
}

void PageIoManager::threadMain()
{
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

	condEnter();

	while (!threadStop)
	{
		condWait();

		while (rRequestIndex != wRequestIndex && !threadStop)
		{
			auto & request = requests[rRequestIndex];

			if (!request.isAborted)
			{
				condLeave();
				{
					pageLoad(request.filename, request.pageData);
				}
				condEnter();

				if (!request.isAborted)
				{
					request.isReady = true;
				}
			}

			if (request.isAborted)
			{
				Assert(!request.isReady);

				request.isActive = false;
				request.isAborted = false;
			}

			rRequestIndex = (rRequestIndex + 1) % kMaxPageRequests;
		}
	}

	condLeave();
}

PageIoRequest * PageIoManager::getRequest(const int id)
{
	return &requests[id];
}

int PageIoManager::addRequest(const char * filename)
{
	int id;

	condEnter();
	{
		id = wRequestIndex;

		auto & request = requests[wRequestIndex];
		Assert(request.isActive == false);
		Assert(request.isReady == false);

		strcpy_s(request.filename, sizeof(request.filename), filename);

		request.isActive = true;

		wRequestIndex = (wRequestIndex + 1) % kMaxPageRequests;

		SDL_CondSignal(cond);
	}
	condLeave();

	return id;
}

//

#include <stdlib.h>

void testPageIo()
{
	pagingInit();
	{
		for (int i = 0; i < 1000; ++i)
		{
			const int id = pageRequest("test");

			if ((rand() %2) == 0)
			{
				while (pageRequestPeek(id) == nullptr)
				{
					SDL_Delay(0);
				}

				if ((rand() % 2) == 0)
				{
					pageRequestConsume(id);
				}
				else
				{
					pageRequestAbort(id);
				}
			}
			else
			{
				SDL_Delay(rand() % 10);

				pageRequestAbort(id);
			}
		}
	}
	pagingShut();
}
