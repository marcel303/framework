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

	Res* CreateStub(RES_TYPE type);

	INITSTATE;

	ResLoader* m_loaders[RES_COUNT];
	std::map<std::string, RefBaseResPtr> m_cache;
};

#define RESMGR ResMgr::I()

#endif
