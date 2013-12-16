#pragma once

#define MAX_SCENE_COUNT 100

namespace Game
{
	class ISceneMgr
	{
	public:
		virtual ~ISceneMgr();
		virtual void PostUpdate() = 0;
	};
	
	class SceneMgt
	{
	public:
		SceneMgt();
		
		void Add(ISceneMgr* mgr);
		void Remove(ISceneMgr* mgr);
		void PostUpdate();
		
	private:
		ISceneMgr* mSceneList[MAX_SCENE_COUNT];
	};
	
	extern SceneMgt g_SceneMgt;
}
