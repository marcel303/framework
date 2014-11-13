#include <algorithm>
#include "Mem.h"
#include "ResFont.h"
#include "ResMgr.h"
#include "ResPS.h"
#include "ResShader.h"
#include "ResSnd.h"
#include "ResTex.h"
#include "ResVS.h"

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

	Assert(loader);

	r->m_res = loader->Load(name);
	r->m_res->m_name = name;

	if (r->m_res.get() == 0)
		r->m_res = CreateStub(type);

	m_cache[name] = r;

	return r;
}

void ResMgr::Initialize()
{
	INITCHECK(false);

	INITSET(true);
}

void ResMgr::Shutdown()
{
	INITCHECK(true);

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
	default: DB_ERR("unknown resource type"); Assert(0); return 0; break;
	}
}
