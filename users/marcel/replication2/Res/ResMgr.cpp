#include <algorithm>
#include "Mem.h"
#include "ResFont.h"
#include "ResMgr.h"
#include "ResPS.h"
#include "ResShader.h"
#include "ResSnd.h"
#include "ResTex.h"
#include "ResVS.h"

static bool DoLoad(ResMgr::Task& task);

ResMgr& ResMgr::I()
{
	static ResMgr mgr;
	return mgr;
}

ShFont ResMgr::GetFont(const std::string& name)
{
	return ShFont(Get(RES_FONT, name, true));
}

ShPS ResMgr::GetPS(const std::string& name)
{
	return ShPS(Get(RES_PS, name));
}

ShShader ResMgr::GetShader(const std::string& name)
{
	return ShShader(Get(RES_SHADER, name));
}

ShSnd ResMgr::GetSnd(const std::string& name)
{
	return ShSnd(Get(RES_SND, name));
}

ShTex ResMgr::GetTex(const std::string& name, bool immediate)
{
	return ShTex(Get(RES_TEX, name, immediate));
}

ShVS ResMgr::GetVS(const std::string& name)
{
	return ShVS(Get(RES_VS, name));
}

ShBaseResPtr ResMgr::Get(RES_TYPE type, const std::string& name, bool immediate)
{
	INITCHECK(true);

	if (m_cache.count(name) != 0)
		if (!m_cache[name].expired())
			return m_cache[name].lock();

	ShBaseResPtr r = ShBaseResPtr(new ResPtrGeneric);

	ResLoader* loader = m_loaders[type];

	if (!loader)
	{
		Assert(0);
	}

	r->m_res = ShBaseRes(CreateStub(type));
	r->m_res->m_name = name;

	if (immediate == false)
		AddTask(r, name, loader, PRI_RT);
	else
	{
		Mux();
		{
			Task task;
			
			task.Initialize(r, name, loader, PRI_RT);

			DoLoad(task);

			task.m_ptr->m_res = SharedPtr<Res>(task.m_res);
		}
		DeMux();
	}

	m_cache[name] = r;

	return r;
}

void ResMgr::Initialize()
{
	INITCHECK(false);

	m_stop = false;

	// Create mutex.
	m_mutex = SDL_CreateMutex();

	INITSET(true);

	// Spawn loading thread.
	m_thread = SDL_CreateThread(ThreadExecX, this);
}

void ResMgr::Shutdown()
{
	INITCHECK(true);

	// TODO: Close loading thread.
	m_stop = true;

	SDL_WaitThread(m_thread, 0);

	Update(); // Make sure newly loaded/allocated resources are copied.

	Assert(m_tasks.size() == 0);
	Assert(m_completedTasks.size() == 0);
	// todo: not zero at exit? Assert(m_cache.size() == 0);

	INITSET(false);
}

void ResMgr::RegisterLoader(RES_TYPE type, ResLoader* loader)
{
	Assert(loader);

	m_loaders[type] = loader;
}

void ResMgr::Update()
{
	INITCHECK(true);

	for (std::map<std::string, RefBaseResPtr>::iterator i = m_cache.begin(); i != m_cache.end();)
	{
		if (i->second.expired())
		{
			std::map<std::string, RefBaseResPtr>::iterator next = i;
			++next;

			m_cache.erase(i);

			i = next;
		}
		else
			++i;
	}

	Mux();

	for (size_t i = 0; i < m_completedTasks.size(); ++i)
	{
		Task task = m_completedTasks[i];

		task.m_ptr->m_res = SharedPtr<Res>(task.m_res);
	}

	m_completedTasks.clear();

	DeMux();
}

ResMgr::ResMgr()
{
	INITINIT;

	for (int i = 0; i < RES_COUNT; ++i)
		m_loaders[i] = 0;
}

ResMgr::~ResMgr()
{
	INITCHECK(false);

	for (int i = 0; i < RES_COUNT; ++i)
		SAFE_FREE(m_loaders[i]);
}

void ResMgr::AddTask(ShBaseResPtr ptr, const std::string& name, ResLoader* loader, PRIORITY priority)
{
	INITCHECK(true);

	Task task;

	task.Initialize(ptr, name, loader, priority);

	Mux();
	{
		m_tasks.push_back(task);
	}
	DeMux();
}

static bool DoLoad(ResMgr::Task& task)
{
	if (!task.m_loader)
	{
		DB_ERR("Warning: Task with no loader.\n");
		return false;
	}

	Res* res = task.m_loader->Load(task.m_name);

	if (res)
	{
		task.m_res = res;
		task.m_res->m_name = task.m_name;

		return true;
	}
	else
	{
		DB_ERR("Warning: Unable to load %s.\n", task.m_name.c_str());

		return false;
	}
}

void ResMgr::Load()
{
	INITCHECK(true);

	std::sort(m_tasks.begin(), m_tasks.end());

	while (m_tasks.size() > 0)
	{
		std::vector<Task>::iterator i = m_tasks.begin();

		Task task = *i;

		if (DoLoad(task))
			m_completedTasks.push_back(task);

		// fixme

		m_tasks.erase(i);

		return;
	}
}

void ResMgr::ThreadExec()
{
	// TODO: Check stop flag.
	while (!m_stop)
	{
		Mux();
		{
#if 0 // fixme, remove
			if (m_tasks.size() > 100 || m_completedTasks.size() > 100)
				Assert(0);
#endif

			Load();
		}
		DeMux();

		//printf("ResMgr.\n");

		// TODO: Use reset.
		SDL_Delay(10);
	}

	Mux();
	{
		while (m_tasks.size() > 0)
			Load();
	}
	DeMux();
}

int ResMgr::ThreadExecX(void* up)
{
	ResMgr* mgr = (ResMgr*)up;

	mgr->ThreadExec();

	return 0;
}

void ResMgr::Mux()
{
	INITCHECK(true);

	// TODO: Mux access to tasks & completed.
	SDL_LockMutex(m_mutex);
}

void ResMgr::DeMux()
{
	INITCHECK(true);

	// TODO: Demux access to tasks & completed.
	SDL_UnlockMutex(m_mutex);
}

Res* ResMgr::CreateStub(RES_TYPE type)
{
	switch (type)
	{
	case RES_FONT: return new ResFont(); break;
	case RES_PS: { ResPS* r = new ResPS(); r->Load("shaders/default_ps.cg"); return r; } break;
	case RES_TEX: return new ResTex(); break;
	case RES_SHADER: return new ResShader(); break;
	case RES_SND: return new ResSnd(); break;
	case RES_VS: { ResVS* r = new ResVS(); r->Load("shaders/default_vs.cg"); return r; } break;
	default: DB_ERR("Unknown resource type"); Assert(0); return 0; break;
	}
}
