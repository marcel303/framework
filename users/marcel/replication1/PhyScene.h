#ifndef PHYSCENE_H
#define PHYSCENE_H
#pragma once

#include "CDCube.h"
#include "CDScene.h"
#include "PhyObject.h"
#include "Vec3.h"

namespace Phy
{
	class Scene
	{
	public:
		Scene();
		~Scene();

		void AddObject(Object* object);
		void RemoveObject(Object* object);
		void Clear();

		void Gravity(Vec3 force);

		void Update(float dt);

		CD::Scene* GetCDScene();

		void Debug_Render();

	private:
		typedef std::map<Object*, int> ObjectColl;
		typedef ObjectColl::iterator ObjectCollItr;

		ObjectColl m_objects;

		CD::Scene m_cdScene;
	};
}

#endif
