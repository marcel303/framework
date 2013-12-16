/*
 *  Ship.h
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "Entity.h"

enum SHIPSTATE
{
	SEARCH,
	CLOSEIN,
	ATTACK,
};


class Ship : public Entity
{
public:
	Ship();
	virtual ~Ship();
	
	void Reset();
	
	void Init(const Vec2& pos, float rad);
	
	virtual void Update();
	virtual void Render();
	
	void SetTarget(Entity* tar);
	void SetTargetHeading(const Vec2& heading);
	
	
	Vec2 m_vel;
	Vec2 m_dir;
	Vec2 m_targetHeading;
	
	int m_angle;
	
	Rad m_avoidShape;
	
	bool m_boost;
	bool m_avoidSun;
	
	void OnDie();
	
	
	
protected:
	
	float m_chargetime;
	
	int m_energy;
	
	SHIPSTATE m_state;
	
private:
	
	void Attack();
	void CloseIn();
	void AvoidHoles();
	void AvoidFriendlies();
	void AvoidSun();
	
	void SelectTarget();
	
	void CheckTarget();
	
	Vec2 m_result;
	
	
	Entity* m_target;
};
