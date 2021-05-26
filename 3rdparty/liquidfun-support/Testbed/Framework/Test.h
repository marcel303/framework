/*
* Copyright (c) 2006-2009 Erin Catto http://www.box2d.org
* Copyright (c) 2013 Google, Inc.
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef TEST_H
#define TEST_H

#include <Box2D/Box2D.h>
#include "DebugDraw.h"
#include "ParticleParameter.h"

enum GLFW_KEY
{
	GLFW_KEY_UNKNOWN,
	
	GLFW_KEY_A,
	GLFW_KEY_B,
	GLFW_KEY_C,
	GLFW_KEY_D,
	GLFW_KEY_E,
	GLFW_KEY_F,
	GLFW_KEY_G,
	GLFW_KEY_H,
	GLFW_KEY_I,
	GLFW_KEY_J,
	GLFW_KEY_K,
	GLFW_KEY_L,
	GLFW_KEY_M,
	GLFW_KEY_N,
	GLFW_KEY_O,
	GLFW_KEY_P,
	GLFW_KEY_Q,
	GLFW_KEY_R,
	GLFW_KEY_S,
	GLFW_KEY_T,
	GLFW_KEY_U,
	GLFW_KEY_V,
	GLFW_KEY_W,
	GLFW_KEY_X,
	GLFW_KEY_Y,
	GLFW_KEY_Z,
	
	GLFW_KEY_0,
	GLFW_KEY_1,
	GLFW_KEY_2,
	GLFW_KEY_3,
	GLFW_KEY_4,
	GLFW_KEY_5,
	GLFW_KEY_6,
	GLFW_KEY_7,
	GLFW_KEY_8,
	GLFW_KEY_9,
	
	GLFW_KEY_LEFT,
	GLFW_KEY_RIGHT,
	
	GLFW_KEY_ESCAPE,
	GLFW_KEY_COMMA
};

struct DebugCamera
{
	b2Vec2 m_center;
	float m_zoom = 1.f;
	
	float m_width = 0.f;
	float m_height = 0.f;
	
	void Apply() const;
	b2Vec2 ConvertScreenToWorld(b2Vec2 v) const;
};

extern DebugCamera g_camera;

extern DebugDraw g_debugDraw;

#include <stdlib.h>

class Test;
struct Settings;

typedef Test* TestCreateFcn();

#define	RAND_LIMIT	32767
#define DRAW_STRING_NEW_LINE 25

/// Random number in range [-1,1]
inline float32 RandomFloat()
{
	float32 r = (float32)(rand() & (RAND_LIMIT));
	r /= RAND_LIMIT;
	r = 2.0f * r - 1.0f;
	return r;
}

/// Random floating point number in range [lo, hi]
inline float32 RandomFloat(float32 lo, float32 hi)
{
	float32 r = (float32)(rand() & (RAND_LIMIT));
	r /= RAND_LIMIT;
	r = (hi - lo) * r + lo;
	return r;
}

/// Test settings. Some can be controlled in the GUI.
struct Settings
{
	Settings()
	{
		viewCenter.Set(0.0f, 20.0f);
		hz = 60.0f;
		velocityIterations = 8;
		positionIterations = 3;
		// Particle iterations are needed for numerical stability in particle
		// simulations with small particles and relatively high gravity.
		// b2CalculateParticleIterations helps to determine the number.
		particleIterations = b2CalculateParticleIterations(10, 0.04f, 1 / hz);
		drawShapes = 1;
		drawParticles = 1;
		drawJoints = 0;
		drawAABBs = 0;
		drawContactPoints = 0;
		drawContactNormals = 0;
		drawContactImpulse = 0;
		drawFrictionImpulse = 0;
		drawCOMs = 0;
		drawStats = 0;
		drawProfile = 0;
		enableWarmStarting = 1;
		enableContinuous = 1;
		enableSubStepping = 0;
		enableSleep = 1;
		pause = 0;
		singleStep = 0;
		printStepTimeStats = 1;
		stepTimeOut = 0;
		strictContacts = 0;
	}

	b2Vec2 viewCenter;
	float32 hz;
	int32 velocityIterations;
	int32 positionIterations;
	int32 particleIterations;
	bool drawShapes;
	bool drawParticles;
	bool drawJoints;
	bool drawAABBs;
	bool drawContactPoints;
	bool drawContactNormals;
	bool drawContactImpulse;
	bool drawFrictionImpulse;
	bool drawCOMs;
	bool drawStats;
	bool drawProfile;
	bool enableWarmStarting;
	bool enableContinuous;
	bool enableSubStepping;
	bool enableSleep;
	bool pause;
	bool singleStep;
	bool printStepTimeStats;
	bool strictContacts;

	/// Measures how long did the world step took, in ms
	float32 stepTimeOut;
};

struct TestMain // todo : remove
{
	static void SetRestartOnParticleParameterChange(bool value);
	static void SetParticleParameters(const ParticleParameter::Definition * params, uint32 count);
	static uint32 GetParticleParameterValue();
	static uint32 SetParticleParameterValue(uint32 value);
};

struct TestEntry
{
	const char *name;
	TestCreateFcn *createFcn;
};

extern TestEntry g_testEntries[];
// This is called when a joint in the world is implicitly destroyed
// because an attached body is destroyed. This gives us a chance to
// nullify the mouse joint.
class DestructionListener : public b2DestructionListener
{
public:
	void SayGoodbye(b2Fixture* fixture) { B2_NOT_USED(fixture); }
	void SayGoodbye(b2Joint* joint);
	void SayGoodbye(b2ParticleGroup* group);

	Test* test;
};

const int32 k_maxContactPoints = 2048;

struct ContactPoint
{
	b2Fixture* fixtureA;
	b2Fixture* fixtureB;
	b2Vec2 normal;
	b2Vec2 position;
	b2PointState state;
	float32 normalImpulse;
	float32 tangentImpulse;
	float32 separation;
};

class Test : public b2ContactListener
{
public:

	Test();
	virtual ~Test();

    void DrawTitle(const char *string);
	virtual void Step(Settings* settings);
	virtual void Keyboard(unsigned char key) { B2_NOT_USED(key); }
	virtual void KeyboardUp(unsigned char key) { B2_NOT_USED(key); }
	void ShiftMouseDown(const b2Vec2& p);
	virtual void MouseDown(const b2Vec2& p);
	virtual void MouseUp(const b2Vec2& p);
	virtual void MouseMove(const b2Vec2& p);
	void LaunchBomb();
	void LaunchBomb(const b2Vec2& position, const b2Vec2& velocity);
	
	void SpawnBomb(const b2Vec2& worldPt);
	void CompleteBombSpawn(const b2Vec2& p);

	// Let derived tests know that a joint was destroyed.
	virtual void JointDestroyed(b2Joint* joint) { B2_NOT_USED(joint); }

	// Let derived tests know that a particle group was destroyed.
	virtual void ParticleGroupDestroyed(b2ParticleGroup* group) { B2_NOT_USED(group); }

	// Callbacks for derived classes.
	virtual void BeginContact(b2Contact* contact) { B2_NOT_USED(contact); }
	virtual void EndContact(b2Contact* contact) { B2_NOT_USED(contact); }
	virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
	virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{
		B2_NOT_USED(contact);
		B2_NOT_USED(impulse);
	}

	void ShiftOrigin(const b2Vec2& newOrigin);
	virtual float32 GetDefaultViewZoom() const;

	// Apply a preset range of colors to a particle group.
	// A different color out of k_ParticleColors is applied to each
	// particlesPerColor particles in the specified group.
	// If particlesPerColor is 0, the particles in the group are divided into
	// k_ParticleColorsCount equal sets of colored particles.
	void ColorParticleGroup(b2ParticleGroup * const group,
							uint32 particlesPerColor);

	// Remove particle parameters matching "filterMask" from the set of
	// particle parameters available for this test.
	void InitializeParticleParameters(const uint32 filterMask);

	// Restore default particle parameters.
	void RestoreParticleParameters();

protected:
	friend class DestructionListener;
	friend class BoundaryListener;
	friend class ContactListener;

	b2Body* m_groundBody;
	b2AABB m_worldAABB;
	ContactPoint m_points[k_maxContactPoints];
	int32 m_pointCount;
	DestructionListener m_destructionListener;
	DebugDraw m_debugDraw;
	int32 m_textLine;
	b2World* m_world;
	b2ParticleSystem* m_particleSystem;
	b2Body* m_bomb;
	b2MouseJoint* m_mouseJoint;
	b2Vec2 m_bombSpawnPoint;
	bool m_bombSpawning;
	b2Vec2 m_mouseWorld;
	bool m_mouseTracing;
	b2Vec2 m_mouseTracerPosition;
	b2Vec2 m_mouseTracerVelocity;
	int32 m_stepCount;

	b2Profile m_maxProfile;
	b2Profile m_totalProfile;

	// Valid particle parameters for this test.
	ParticleParameter::Value* m_particleParameters;
	ParticleParameter::Definition m_particleParameterDef;

	static const b2ParticleColor k_ParticleColors[];
	static const uint32 k_ParticleColorsCount;
};

#endif
