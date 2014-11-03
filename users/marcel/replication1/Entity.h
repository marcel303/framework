#ifndef ENTITY_H
#define ENTITY_H
#pragma once

#include <string>
#include <vector>
#include "AABBObject.h"
#include "CDObject.h"
#include "Mat4x4.h"
#include "ParameterList.h"
#include "PhyObject.h"
#include "ReplicationPriority.h"
#include "ResShader.h"
#include "Scene.h"

#undef GetClassName

class Entity
{
public:
	enum CAPS
	{
		CAP_STATIC_PHYSICS  = 1 << 0,
		CAP_DYNAMIC_PHYSICS = 1 << 1,
		CAP_PHYSICS         = CAP_STATIC_PHYSICS | CAP_DYNAMIC_PHYSICS
	};

	Entity();
	virtual ~Entity();
	virtual void PostCreate();

	ShEntity Self();

	void EnableCaps(int caps);
	void DisableCaps(int caps);

	Scene* GetScene() const;
	int GetID() const;
	virtual Mat4x4 GetTransform() const;
	Vec3 GetPosition() const;
	Vec3 GetOrientation() const;
	ParameterList* GetParameters();
	const std::string& GetClassName() const;

	void SetClassName(const std::string& className);
	void SetScene(Scene* scene);
	void SetActive(bool active);
	virtual void SetController(int id);
	void SetTransform(const Mat4x4& transform);

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void UpdateRender();
	virtual void Render();

	void ReplicatedMessage(int message, int value);

	virtual void OnSceneAdd(Scene* scene);
	virtual void OnSceneRemove(Scene* scene);

	virtual void OnActivate();
	virtual void OnDeActivate();

	virtual void OnMessage(int message, int value);

	virtual void OnTransformChange();

//protected: // FIXME
	void AddParameter(const Parameter& parameter);
	void AddParameter(ShParameter parameter);
protected:
	void UpdateCaps(int oldCaps, int newCaps);
	void AddAABBObject(AABBObject* object);

public:
//private: FIXME
	Scene* m_scene;
	int m_id;
	bool m_active;
	int m_caps;

	std::string m_className;
	ParameterList m_parameters;

	std::vector<AABBObject*> m_aabbObjects;

	int m_repObjectID;
	Replication::Priority m_priority;

	// CD & Physics.
	typedef std::vector<Phy::Object*> PhyObjectColl;
	typedef PhyObjectColl::iterator PhyObjectCollItr;

	CD::Group m_cdGroup;
	CD::GroupFltEX m_cdFlt;
	Phy::Object m_phyObject;

	Mat4x4 m_transform;
	ShShader m_shader;

	AABB m_aabb; // Derived from AABB objects.
	bool m_hasAABB;
};

#include "EntityLink.h"
#include "EntityPtr.h"

#endif
