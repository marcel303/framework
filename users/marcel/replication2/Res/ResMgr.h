#ifndef RESMGR_H
#define RESMGR_H
#pragma once

#include <map>
#include <SDL/SDL.h>
#include <string>
#include <vector>
#include "Debug.h"
#include "Res.h"
#include "ResLoader.h"

// Multi threaded loading:
//
// - Load resources in background.
// - If loaded, replace previously returned instances..
//   - But how?
// Incompatible with lock/unlock mechanism?
// - Safe memory by loading once, throwing away after uploading to device memory -> restrics usage to single device.
// - Let loader load, once ready, in main thread/or lock, and replace resource data, invalidate? Let loader decide.
//   transfer must be fast, eg loader loads, decompresses, waits for memcopy or smt.
// - Replace pointers.. Ugly solution..

class ResMgr
{
public:
	enum PRIORITY
	{
		PRI_RT,
		PRI_HIGH,
		PRI_NORMAL,
		PRI_LOW
	};

	class Task
	{
	public:
		inline void Initialize(ShBaseResPtr ptr, const std::string& name, ResLoader* loader, PRIORITY priority)
		{
			m_ptr = ptr;
			m_name = name;
			m_loader = loader;
			m_priority = priority;
			m_res = 0;
		}

		ShBaseResPtr m_ptr;
		std::string m_name;
		ResLoader* m_loader;
		PRIORITY m_priority;
		Res* m_res;

		inline bool operator<(const Task& other) const
		{
			if (m_priority < other.m_priority)
				return true;
			if (m_priority > other.m_priority)
				return false;

			return false;
		}
	};

	static ResMgr& I();

	ShFont GetFont(const std::string& name);
	ShPS GetPS(const std::string& name);
	ShShader GetShader(const std::string& name);
	ShSnd GetSnd(const std::string& name);
	ShTex GetTex(const std::string& name, bool immediate = true);
	ShVS GetVS(const std::string& name);

	void Initialize();
	void Shutdown();

	void RegisterLoader(RES_TYPE type, ResLoader* loader);

	void Update();

private:
	ResMgr();
	~ResMgr();

	ShBaseResPtr Get(RES_TYPE type, const std::string& name, bool immediate = true);

	void AddTask(ShBaseResPtr ptr, const std::string& name, ResLoader* loader, PRIORITY priority);
	void Load();

	void ThreadExec();
	static int ThreadExecX(void* up);

	void Mux();
	void DeMux();

	Res* CreateStub(RES_TYPE type);

	INITSTATE;

	SDL_mutex* m_mutex;
	SDL_Thread* m_thread;
	bool m_stop;
	ResLoader* m_loaders[RES_COUNT];
	std::vector<Task> m_tasks;
	std::vector<Task> m_completedTasks;
	std::map<std::string, RefBaseResPtr> m_cache;
};

#define RESMGR ResMgr::I()

#endif
