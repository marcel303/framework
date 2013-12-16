#include "Debugging.h"
#include "SceneMgt.h"

namespace Game
{
	SceneMgt g_SceneMgt;
	
	//
	
	ISceneMgr::~ISceneMgr()
	{
	}
	
	//
	
	SceneMgt::SceneMgt()
	{
		for (int i = 0; i < MAX_SCENE_COUNT; ++i)
			mSceneList[i] = 0;
	}
	
	void SceneMgt::Add(ISceneMgr* mgr)
	{
		for (int i = 0; i < MAX_SCENE_COUNT; ++i)
		{
			if (mSceneList[i])
				continue;
			
			mSceneList[i] = mgr;
			
			return;
		}
		
		Assert(false);
	}
	
	void SceneMgt::Remove(ISceneMgr* mgr)
	{
		for (int i = 0; i < MAX_SCENE_COUNT; ++i)
		{
			if (mSceneList[i] != mgr)
				continue;
			
			mSceneList[i] = 0;
			
			return;
		}
	}
	
	void SceneMgt::PostUpdate()
	{
		for (int i = 0; i < MAX_SCENE_COUNT; ++i)
		{
			if (mSceneList[i] == 0)
				continue;
			
			mSceneList[i]->PostUpdate();
		}
	}
}
