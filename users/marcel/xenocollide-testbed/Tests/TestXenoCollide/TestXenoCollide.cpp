/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>

#include <list>
#include <map>
#include <set>
#include <stack>
#include <vector>

#include "Arbiter.h"
#include "Collide.h"
#include "CollideGeometry.h"
#include "Contact.h"
#include "Constraint.h"
#include "DrawUtil.h"
#include "MathUtil.h"
#include "OutlineFont.h"
#include "MapPtr.h"
#include "RenderPolytope.h"
#include "RigidBody.h"
#include "ShrinkWrap.h"
#include "TestModule.h"

#include "XenoTestbed.h"
#include "XenoTestbedWindow.h"

#include "framework.h"

#include "Timer.h"

using namespace std;

#undef ASSERT
#define ASSERT(a) if (a) *(int*)NULL = 0;

#define XENO_TODO_FONT 0

//////////////////////////////////////////////////////////////
// Global Data

class DebugWidget;

extern bool gTrackingOn;
extern list<DebugWidget*> g_debugQueue;

int32 gCollisionCount = 0;
int32 gCollisionHitCount = 0;
float32 gCollisionTime = 0;
float32 gTimeStep = 0;
float32 gTimeRatio = 0; // this ratio is equal to (gTimeStep / "old gTimeStep")
bool gFriction = true;
float32 gColorMin = 0.15f;
int32 gTimeStamp = 1;
bool gSimulate = false;
bool gDebug = false;
int32 gCurrentDebugTick = 0;
bool gCullFrontFace = false;
bool gPolygonModeFill = true;

//////////////////////////////////////////////////////////////

class TestXenoCollide : public TestModule
{

public:

	static TestModule* Create();

	TestXenoCollide();
	~TestXenoCollide();

	void Init();
	void Simulate(float32 dt);
	void DrawScene();
	void OnKeyDown(uint32 nChar);
	void OnChar(uint32 nChar);
	
	const Vector& GetBackgroundColor() { return mBackgroundColor; }

private:

	struct Shape
	{
		Shape() : geom(NULL), model(NULL) { q = Quat(0, 0, 0, 1); x = Vector(0, 0, 0); }
		Shape(CollideGeometry* _geom, RenderPolytope* _model, const Quat& _q, const Vector& _x) : geom(_geom), model(_model), q(_q), x(_x) {}
		Shape(CollideGeometry* _geom, RenderPolytope* _model) : geom(_geom), model(_model) { q = Quat(0, 0, 0, 1); x = Vector(0, 0, 0); }
		MapPtr<CollideGeometry>	geom;
		MapPtr<RenderPolytope>	model;
		Quat				q;
		Vector				x;
	};

	typedef list< MapPtr<Shape> > ShapeStack;
	ShapeStack	mShapeStack;
	Vector mBackgroundColor;

	typedef map<string, MapPtr<Shape> > ShapeSet;
	ShapeSet mShapeSet;

	vector < MapPtr<CollideGeometry> > mShapes;

	Vector x;
	Quat q;
	int32 mGenerationThreshold;
	vector< MapPtr<RigidBody> > mRigidBody;
	map<ArbiterKey, Arbiter*> mArbiterMap;
	int32 mFirstSimulatedRigidBody;
	float32 mLinearDrag;
	float32 mAngularDrag;

	bool mStackMode;
	bool mDebugTime;
	string mCommand;
	bool mCommandMode;
	float32 mFPS;

#if XENO_TODO_FONT == 1
	OutlineFont* mFont;
#endif

	string mStatusLine;

private:

	void CreateGround();
	void CreateRigidBody(CollideGeometry* collideModel, const Quat& q, const Vector& x, float32 inv_m, RenderPolytope* renderModel = NULL);
	RenderPolytope* CreateRenderModel(CollideGeometry* g);
	void ShuffleRigidBodies();
	Arbiter* FindArbiter(RigidBody* b1, RigidBody* b2);
	Arbiter* CreateArbiter(RigidBody* b1, RigidBody* b2);
	void ProcessCommand(string& cmd);
	void Status(const char* str);
	void ClearArbiters();

	void SwitchToSimulateMode();
	void SwitchToStackMode();

	// Commands
	void Drag(float32 linear, float32 angular);
	void Drop();
	void DropAll();
};

//////////////////////////////////////////////////////////////

RegisterTestModule gTestXenoCollide( TestXenoCollide::Create, "Xeno Collide");

//////////////////////////////////////////////////////////////

TestModule* TestXenoCollide::Create()
{
	return new TestXenoCollide();
}

//////////////////////////////////////////////////////////////

TestXenoCollide::TestXenoCollide()
: mGenerationThreshold(4)
, mFirstSimulatedRigidBody(5)
, mLinearDrag(0.90f)
, mAngularDrag(0.90f)
, mStackMode(true)
, mDebugTime(false)
, mCommandMode(true)
, mFPS(0)
{
	gSimulate = false;
	gCullFrontFace = false;

#if XENO_TODO_FONT == 1
	mFont = new OutlineFont(hdc, "Comic Sans MS Bold");
#endif

	mBackgroundColor = Vector(0, 0, 0);

	CreateGround();
}

//////////////////////////////////////////////////////////////

RenderPolytope* TestXenoCollide::CreateRenderModel(CollideGeometry* g)
{
	RenderPolytope* renderModel = new RenderPolytope();
	renderModel->Init(*g);
	return renderModel;
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::CreateRigidBody(CollideGeometry* collideModel, const Quat& q, const Vector& x, float32 inv_m, RenderPolytope* renderModel)
{
	if (!renderModel)
	{
		renderModel = CreateRenderModel(collideModel);
	}

	MapPtr<Body> body = new Body();
	body->q = q;
	body->x = x;
	body->inv_I.BuildIdentity();
	body->inv_I *= inv_m / 25.0f;
	body->inv_I(3,3) = 1;
	body->inv_m = inv_m;

	if (inv_m > 0)
	{
		ComputeMassProperties(body, renderModel, 1.0f);
	}

	float32 radiusNegX = Abs(collideModel->GetSupportPoint( Vector(-1, 0, 0) ).X());
	float32 radiusPosX = Abs(collideModel->GetSupportPoint( Vector( 1, 0, 0) ).X());
	float32 radiusNegY = Abs(collideModel->GetSupportPoint( Vector(0, -1, 0) ).Y());
	float32 radiusPosY = Abs(collideModel->GetSupportPoint( Vector(0,  1, 0) ).Y());
	float32 radiusNegZ = Abs(collideModel->GetSupportPoint( Vector(0, 0, -1) ).Z());
	float32 radiusPosZ = Abs(collideModel->GetSupportPoint( Vector(0, 0,  1) ).Z());

	Vector maxRadiusVector
	(
		Max(radiusNegX, radiusPosX),
		Max(radiusNegY, radiusPosY),
		Max(radiusNegZ, radiusPosZ)
	);

	float32 maxRadius = maxRadiusVector.Len3();

	mRigidBody.push_back(new RigidBody(body, collideModel, renderModel, maxRadius));
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::ShuffleRigidBodies()
{
	int32 count = (int32) mRigidBody.size();
	for (int32 i = mFirstSimulatedRigidBody; i < count; i++)
	{
		int32 j = rand() % (count - i);
		MapPtr<RigidBody> tmp = mRigidBody[i];
		mRigidBody[i] = mRigidBody[i+j];
		mRigidBody[i+j] = tmp;
	}
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::CreateGround()
{
	MapPtr<CollideBox> ground = new CollideBox( Vector(200, 200, 10));
	CreateRigidBody(ground, Quat(0, 0, 0, 1), Vector(0, 0, 0), 0 );
	mFirstSimulatedRigidBody = 1;
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::Init()
{
	srand(123456);
	mStatusLine = "Type 'help' for a list of commands";
	return;
}

//////////////////////////////////////////////////////////////

extern void DebugDraw();
extern float32 gAvgSupportCount;

void TestXenoCollide::DrawScene()
{
	setWireframe(false);

	if (mCommandMode)
	{
		gxColor4f( 1-mBackgroundColor.X(), 1-mBackgroundColor.Y(), 1-mBackgroundColor.Z(), 1);

		gxPushMatrix();
		gxLoadIdentity();
		gxTranslatef(-15, -11, -50);
	#if XENO_TODO_FONT == 1
		mFont->DrawString(mStatusLine.c_str());
	#endif
		gxPopMatrix();

		gxPushMatrix();
		gxLoadIdentity();
		gxTranslatef(-15, -12, -50);
		string displayString;
		if (gSimulate)
			displayString = "sim";
		else
			displayString = "edit";
		
		displayString += "> " + mCommand;
		if ((g_TimerRT.TimeMS_get() / 500) % 2)
		{
			displayString += "_";
		}
	#if XENO_TODO_FONT == 1
		mFont->DrawString(displayString.c_str());
	#else
		//fillCube(Vec3(), Vec3(100.0f));
		logDebug("%s", displayString.c_str());
	#endif
		gxPopMatrix();
	}

	setWireframe(gPolygonModeFill ? false : true);

	if (mStackMode)
	{
		for (ShapeStack::iterator it = mShapeStack.begin(); it != mShapeStack.end(); it++)
		{
			(*it)->model->Draw( (*it)->q, (*it)->x );
		}
		return;
	}

	if (gDebug)
	{
		DebugDraw();
		return;
	}

	if (gSimulate)
	{
		// Collide all objects against one another
		int32 rigidBodyCount = (int32) mRigidBody.size();
		for (int32 i=0; i < rigidBodyCount; i++)
		{
			RigidBody* b1 = mRigidBody[i];
			b1->renderModel->SetColor(b1->color);
			b1->renderModel->Draw(b1->body->q, b1->body->x - b1->body->q.Rotate(b1->body->com));
		}

		if (mDebugTime)
		{
			gxPushMatrix();
			gxLoadIdentity();
			gxTranslatef(-15, -13, -50);

			static float32 avg = 6;
			if (gCollisionCount > 0)
			{
				avg = 0.99f * avg + 0.01 * gCollisionTime / gCollisionCount;
			}
			char text[100];
			sprintf(text, "FPS %2.2f  Avg = %2.2f  Count = %d  Hits = %d  AvgTime = %.2f us", mFPS, gAvgSupportCount, gCollisionCount, gCollisionHitCount, avg);
			gxColor4f(1, 1, 1, 1);
		#if XENO_TODO_FONT == 1
			mFont->DrawString(text);
		#endif
			gxPopMatrix();
		}

		return;
	}

	return;
}

//////////////////////////////////////////////////////////////

Arbiter* TestXenoCollide::FindArbiter(RigidBody* b1, RigidBody* b2)
{
	ArbiterIt it = mArbiterMap.find( ArbiterKey(b1, b2) );
	if (it == mArbiterMap.end())
	{
		return NULL;
	}
	return (*it).second;
};

//////////////////////////////////////////////////////////////

Arbiter* TestXenoCollide::CreateArbiter(RigidBody* b1, RigidBody* b2)
{
	Arbiter* arb = new Arbiter(b1, b2);
	mArbiterMap[ ArbiterKey(b1, b2) ] = arb;
	return arb;
};

//////////////////////////////////////////////////////////////

void TestXenoCollide::OnChar(uint32 nChar)
{
	if (nChar == SDLK_ESCAPE)
	{
		mCommandMode = !mCommandMode;
	}
	else if (mCommandMode)
	{
		if (nChar == SDLK_BACKSPACE)
		{
			if (mCommand.size() >= 1)
			{
				mCommand = mCommand.substr(0, mCommand.size()-1);
			}
		}
		else if (nChar == SDLK_RETURN)
		{
			ProcessCommand(mCommand);
			mCommand = "";
		}
		else if (isprint(nChar))
		{
			mCommand += nChar;
		}
		return;
	}
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::OnKeyDown(uint32 nChar)
{
	if (mCommandMode)
	{
		return;
	}

	switch (nChar)
	{
		case 188: // ,
			{
				if (gDebug)
				{
					gCurrentDebugTick--;
					if (gCurrentDebugTick < 0) gCurrentDebugTick = 0;
				}
			}
			break;

		case 190: // .
			{
				if (gDebug)
				{
					gCurrentDebugTick++;
				}
			}
			break;
	}
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::Simulate(float32 dt)
{
	mFPS = 1.0f / dt;

	dt = 1.0f / 30.0f;

	if (dt > 1.0f / 30.f)
	{
		dt = 1.0f / 30.0f;
	}

	if (!gSimulate) return;

	if (gTimeStep > 0)
		gTimeRatio = dt / gTimeStep;
	else
		gTimeRatio = 1.0f;

	gTimeStep = dt;

	gTimeStamp++;

	int32 rigidBodyCount = (int32) mRigidBody.size();

	int32 i;

	// Integrate
	for (i=0; i < rigidBodyCount; i++)
	{
		Body* b = mRigidBody[i]->body;

		b->x += b->v * dt;
		if (i >= mFirstSimulatedRigidBody) b->v += Vector(0, 0, -400) * dt;

		b->q = b->q + dt * (0.5f * Quat(b->omega.X(), b->omega.Y(), b->omega.Z(), 0) * b->q);
		b->q.Normalize4();

		b->v *= mLinearDrag;
		b->omega *= mAngularDrag;
	}

	// Collide all objects against one another
	int32 collisionCount = 0;
	int32 hitCount = 0;
	gCollisionTime = 0;
	for (i=0; i < rigidBodyCount; i++)
	{
		RigidBody* b1 = mRigidBody[i];

		for (int32 j=i+1; j < rigidBodyCount; j++)
		{
			RigidBody* b2 = mRigidBody[j];

			Vector n;
			Vector p1;
			Vector p2;

			float32 cullingRadius = b1->maxRadius + b2->maxRadius;

			if (i < mFirstSimulatedRigidBody || (b1->body->x - b2->body->x).Len3Squared() < cullingRadius * cullingRadius)
			{
				uint32 startTime = g_TimerRT.TimeUS_get();
				collisionCount++;
				bool result = CollideAndFindPoint(*b1->collideModel, b1->body->q, b1->body->x - b1->body->q.Rotate(b1->body->com), *b2->collideModel, b2->body->q, b2->body->x - b2->body->q.Rotate(b2->body->com), &n, &p1, &p2);
				uint32 stopTime = g_TimerRT.TimeUS_get();
				gCollisionTime += stopTime - startTime;

				if (result)
				{
					hitCount++;

					Arbiter* arb = FindArbiter(b1, b2);

					// Find the support points
					Vector s1 = TransformSupportVert(*b1->collideModel, b1->body->q, b1->body->x - b1->body->q.Rotate(b1->body->com), -n);
					Vector s2 = TransformSupportVert(*b2->collideModel, b2->body->q, b2->body->x - b2->body->q.Rotate(b2->body->com), n);

					Vector pp1 = (s1 - p1) * n * n + p1;
					Vector pp2 = (s2 - p2) * n * n + p2;
		
					// We have a collision -- store away the collision point
					if (!arb) arb = CreateArbiter(b1, b2);
					arb->AddContact(pp1, pp2, n);
				}
			}
		}
	}

	gCollisionCount = collisionCount;
	gCollisionHitCount = hitCount;

	// Resolve momentum exchanges
	for (int32 iteration=0; iteration < 20; iteration++)
	{
		ArbiterIt it = mArbiterMap.begin();
		ArbiterIt end = mArbiterMap.end();
		while (it != end)
		{
			ContactList::iterator contactIt = (*it).second->contacts.begin();
			ContactList::iterator endContactIt = (*it).second->contacts.end();
			while (contactIt != endContactIt)
			{
				bool zapIt = false;

				if ( (*contactIt)->Distance() > 0.2f && gTimeStamp != (*contactIt)->timeStamp )
				{
					zapIt = true;
				}

				if ( zapIt )
				{
					ContactList::iterator next = contactIt;
					next++;
					delete (*contactIt);
					(*it).second->contacts.erase(contactIt);
					contactIt = next;
					continue;
				}

				if (iteration == 0)
				{
					(*contactIt)->constraint->PrepareForIteration();
				}
				else
				{
					(*contactIt)->constraint->Iterate();
				}
				contactIt++;
			}
			it++;
		}
	}
}

//////////////////////////////////////////////////////////////

TestXenoCollide::~TestXenoCollide()
{
	ClearArbiters();

	mShapeStack.clear();
	mShapes.clear();
	mRigidBody.clear();
	mShapeSet.clear();

#if XENO_TODO_FONT == 1
	delete mFont;
	mFont = NULL;
#endif
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::ProcessCommand(string& cmd)
{
	for (uint32 i=0; i < cmd.size(); i++)
	{
		cmd[i] = tolower(cmd[i]);
	}

	if (cmd == "" || cmd == "cls" || cmd.substr(0,1) == "#")
	{
		Status("");
	}
	else if (cmd == "axis")
	{
		float32 x = 10;
		sscanf(cmd.c_str(), " axis %f", &x);
		MapPtr<CollideGeometry> geom = new CollideSegment(x);
		MapPtr<RenderPolytope> model = CreateRenderModel(geom);
		model->SetColor( Vector(1, 0, 0) );
		mShapeStack.push_back( new Shape(geom, model) );
		model = CreateRenderModel(geom);
		model->SetColor( Vector(0, 1, 0) );
		mShapeStack.push_back( new Shape(geom, model, Quat(0, 0, sinf(PI/4), cosf(PI/4)), Vector(0, 0, 0) ) );
		model = CreateRenderModel(geom);
		model->SetColor( Vector(0, 0, 1) );
		mShapeStack.push_back( new Shape(geom, model, Quat(0, sinf(PI/4), 0, cosf(PI/4)), Vector(0, 0, 0) ) );

		Status("Axis created.");
		SwitchToStackMode();
	}
	else if (cmd.substr(0, 10) == "background")
	{
		float32 r = 0;
		float32 g = 0;
		float32 b = 0;
		sscanf(cmd.c_str(), " background %f %f %f", &r, &g, &b);
		mBackgroundColor = Vector(r, g, b);

		Status("Background color set.");
	}
	else if (cmd.substr(0,3) == "box")
	{
		float32 x = 5;
		float32 y = 5;
		float32 z = 5;
		sscanf(cmd.c_str(), "box %f %f %f", &x, &y, &z);
		MapPtr<CollideGeometry> geom = new CollideBox(Vector(x, y, z));
		MapPtr<RenderPolytope> model = new RenderPolytope();
		model->Init(*geom, mGenerationThreshold);
		mShapeStack.push_back( new Shape(geom, model) );

		Status("Box created.");
		SwitchToStackMode();
	}
	else if (cmd == "clear")
	{
		while (mRigidBody.size() > 1)
		{
			mRigidBody.pop_back();
		}

		ClearArbiters();

		Status("Removed all rigid bodies.");
		SwitchToSimulateMode();
	}
	else if (cmd == "collide" || cmd == "step")
	{
		if (gDebug)
		{
			gDebug = false;
			Status("Single-stepping disabled.");
		}
		else if (mShapeStack.size() >= 2)
		{
			MapPtr<Shape> s1 = mShapeStack.back();
			mShapeStack.pop_back();

			MapPtr<Shape> s2 = mShapeStack.back();
			mShapeStack.push_back(s1);

			g_debugQueue.clear();
			gCurrentDebugTick = 0;
			gTrackingOn = true;
			CollideAndFindPoint(*s1->geom, s1->q, s1->x, *s2->geom, s2->q, s2->x, NULL, NULL, NULL);
			gTrackingOn = false;

			gCullFrontFace = true;
			gDebug = true;
			mStackMode = false;

			Status("Single-stepping enabled.");
		}
		else
		{
			Status("Collision single-stepping requires two shapes on the stack.");
		}
	}
	else if (cmd.substr(0, 5) == "color")
	{
		if (mShapeStack.size() >= 1)
		{
			float32 r = 0.5f;
			float32 g = 0.5f;
			float32 b = 0.5f;
			sscanf(cmd.c_str(), "color %f %f %f", &r, &g, &b);
			mShapeStack.back()->model->SetColor( Vector(r, g, b) );

			Status("Color set.");
			SwitchToStackMode();
		}
		else
		{
			Status("Shape stack is empty.");
		}
	}
	else if (cmd.substr(0, 6) == "detail")
	{
		int32 level = 2;
		sscanf(cmd.c_str(), " detail %d", &level);
		if (level < 1) level = 1;
		if (level > 6) level = 6;
		mGenerationThreshold = level;

		Status("Detail level set.");
		SwitchToStackMode();
	}
	else if (cmd == "diff")
	{
		if (mShapeStack.size() >= 2)
		{
			MapPtr<Shape> shape2 = mShapeStack.back();
			mShapeStack.pop_back();

			MapPtr<Shape> shape1 = mShapeStack.back();
			mShapeStack.pop_back();

			MapPtr<Shape> newShape = new Shape();
			newShape->geom = new CollideDiff(shape1->geom, shape1->q, shape1->x, shape2->geom, shape2->q, shape2->x);
			newShape->model = new RenderPolytope();
			newShape->model->Init( *(newShape->geom), mGenerationThreshold );
			mShapeStack.push_back(newShape);

			Status("Minkowski difference created.");
			SwitchToStackMode();
		}
		else
		{
			Status("Minkowski difference requires two shapes.");
		}
	}
	else if (cmd.substr(0,4) == "disc")
	{
		float32 x = 5;
		float32 y = 5;
		int32 count = sscanf(cmd.c_str(), "disc %f %f", &x, &y);
		MapPtr<CollideGeometry> geom;
		if (count == 2)
		{
			geom = new CollideEllipse(x, y);
		}
		else
		{
			geom = new CollideDisc(x);
		}
		MapPtr<RenderPolytope> model = new RenderPolytope();
		MapPtr<CollideSphere> renderSphere = new CollideSphere(0.01f);
		MapPtr<CollideGeometry> renderShape = new CollideSum(geom, renderSphere);
		model->Init(*renderShape, mGenerationThreshold);
		mShapeStack.push_back( new Shape(geom, model) );

		Status("Disc created.");
		SwitchToStackMode();
	}
	else if (cmd.substr(0, 4) == "drag")
	{
		float32 linear = 0;
		float32 angular = 0;
		int32 count = sscanf(cmd.c_str(), "drag %f %f", &linear, &angular);
		if (count == 1)
			Drag(linear, linear);
		else if (count == 2)
			Drag(linear, angular);

		Status("Drag set.");
		SwitchToSimulateMode();
	}
	else if (cmd == "drawmode")
	{
		switch (gPolygonModeFill)
		{
			case false:
				gPolygonModeFill = true;
				Status("Draw mode changed to filled polygons.");
				break;
			case true:
				gPolygonModeFill = false;
				Status("Draw mode changed to lines.");
				break;
		}
	}
	else if (cmd == "drop")
	{
		Drop();
		Status("Rigid body dropped.");
		SwitchToSimulateMode();
	}
	else if (cmd == "drop all")
	{
		DropAll();
		Status("All rigid bodies dropped.");
		SwitchToSimulateMode();
	}
	else if (cmd.substr(0, 3) == "dup")
	{
		uint32 n = 1;
		int32 count = sscanf(cmd.c_str(), "dup %d", &n);
		if (mShapeStack.size() >= n)
		{
			ShapeStack::iterator it = mShapeStack.end();
			for (uint32 i=0; i < n; i++)
			{
				it--;
			}
			for (uint32 i=0; i < n; i++)
			{
				MapPtr<Shape> s = new Shape();
				*s = **it;
				mShapeStack.push_back( s );
				it++;

				Status("Shape(s) duplicated.");
				SwitchToStackMode();
			}
		}
		else
		{
			Status("Too few shapes on stack.");
		}
	}
	else if (cmd.substr(0, 7) == "execute" || cmd.substr(0, 3) == "run")
	{
		char name[100];
		if (sscanf(cmd.c_str(), " execute %s", name) == 1 || sscanf(cmd.c_str(), " run %s", name) == 1)
		{
			string filename = string("scripts\\") + name;
			FILE* fp = fopen(filename.c_str(), "rt");
			if (!fp)
			{
				filename += ".txt";
				fp = fopen(filename.c_str(), "rt");
			}
			if (fp)
			{
				while (!feof(fp) && fgets(name, 100, fp))
				{
					for (char* tmp = name; *tmp; tmp++)
					{
						if (*tmp == 10 || *tmp == 13)
						{
							*tmp = 0;
							break;
						}
					}
					string cmd(name);
					ProcessCommand( cmd );
				}
				fclose(fp);
				Status("Execution complete.");
			}
			else
			{
				Status("Could not read file.");
			}
		}
		else
		{
			Status("Could not read file.");
		}
	}
	else if (cmd == "exit" || cmd == "quit")
	{
		framework.quitRequested = true;
	}
	else if (cmd == "friction")
	{
		gFriction = !gFriction;
		if (gFriction)
			Status("Friction enabled.");
		else
			Status("Friction disabled");
		SwitchToSimulateMode();
	}
	else if (cmd.substr(0,8) == "football")
	{
		float32 l = 5.5f;
		float32 w = 3.5f;
		sscanf(cmd.c_str(), "football %f %f", &l, &w);
		MapPtr<CollideGeometry> geom = new CollideFootball(l,w);
		MapPtr<RenderPolytope> model = new RenderPolytope();
		model->Init(*geom, mGenerationThreshold);
		mShapeStack.push_back( new Shape(geom, model) );

		Status("Football created.");
		SwitchToStackMode();
	}
	else if (cmd.substr(0, 4) == "help")
	{
		system("notepad.exe commands.txt");
	}
	else if (cmd.substr(0,4) == "load")
	{
		char name[100];
		if (sscanf(cmd.c_str(), " load %s", name) == 1)
		{
			for (char* c = name; *c; c++)
			{
				*c = tolower(*c);
			}
			ShapeSet::iterator it = mShapeSet.find(name);
			if (it != mShapeSet.end())
			{
				MapPtr<Shape> s = new Shape();
				*s = *(it->second);
				s->model = CreateRenderModel(s->geom);
				mShapeStack.push_back(s);
				Status("Shape loaded.");
				SwitchToStackMode();
			}
			else
			{
				Status("Shape not found.");
			}
		}
	}
	else if (cmd.substr(0, 6) == "create")
	{
		int32 count = 1;
		char name[100];

		name[0] = 0;
		if (sscanf(cmd.c_str(), " create %d %s", &count, name) < 1)
		{
			sscanf(cmd.c_str(), " create %s", name);
		}

		for (char* c = name; *c; c++)
		{
			*c = tolower(*c);
		}
		ShapeSet::iterator it = mShapeSet.find(name);
		if (it != mShapeSet.end())
		{
			Shape& s = *(it->second);
			MapPtr<RenderPolytope> model = CreateRenderModel(s.geom);
			for (int32 i=0; i < count; i++)
			{
				CreateRigidBody(s.geom, Quat(0, 0, 0, 1), 20 * Vector(i % 10 - 5, (i / 10) % 10 - 5, i / 100 + 5), 1, model);
			}
			Status("Rigid body (bodies) created.");
			SwitchToSimulateMode();
		}
		else
		{
			Status("Shape not found.");
		}
	}
	else if (cmd == "mode")
	{
		if (mStackMode)
		{
			SwitchToSimulateMode();
			Status("Mode: Rigid body simulation");
		}
		else
		{
			SwitchToStackMode();
			Status("Mode: Shape creation");
		}
	}
	else if (cmd.substr(0, 4) == "move")
	{
		if (!mShapeStack.empty())
		{
			float32 x = 0;
			float32 y = 0;
			float32 z = 0;
			sscanf(cmd.c_str(), " move %f %f %f", &x, &y, &z);
			mShapeStack.back()->x += Vector(x, y, z);
			Status("Shape moved.");
			SwitchToStackMode();
		}
		else
		{
			Status("Shape stack is empty.");
		}
	}
	else if (cmd.substr(0, 5) == "point")
	{
		float32 x = 0;
		float32 y = 0;
		float32 z = 0;
		sscanf(cmd.c_str(), " point %f %f %f", &x, &y, &z);
		MapPtr<CollideGeometry> geom = new CollidePoint(Vector(x, y, z));
		MapPtr<RenderPolytope> model = new RenderPolytope();
		MapPtr<CollideSphere> renderSphere = new CollideSphere(0.25f);
		MapPtr<CollideGeometry> renderShape = new CollideSum(geom, renderSphere);
		model->Init(*renderShape, mGenerationThreshold);
		mShapeStack.push_back( new Shape(geom, model) );
		Status("Point created.");
		SwitchToStackMode();
	}
	else if (cmd == "pop")
	{
		if (!mShapeStack.empty())
		{
			MapPtr<Shape> shape = mShapeStack.back();
			shape->model = NULL;
			mShapeStack.pop_back();
			Status("Shape popped from stack.");
			SwitchToStackMode();
		}
		else
		{
			Status("Shape stack is empty.");
		}
	}
	else if (cmd.substr(0,4) == "rect")
	{
		float32 x = 5;
		float32 y = 5;
		sscanf(cmd.c_str(), " rect %f %f", &x, &y);
		MapPtr<CollideGeometry> geom = new CollideRectangle(x, y);
		MapPtr<RenderPolytope> model = new RenderPolytope();
		MapPtr<CollideSphere> renderSphere = new CollideSphere(0.25f);
		MapPtr<CollideGeometry> renderShape = new CollideSum(geom, renderSphere);
		model->Init(*renderShape, mGenerationThreshold);
		mShapeStack.push_back( new Shape(geom, model) );
		Status("Rectangle created.");
		SwitchToStackMode();
	}
	else if (cmd.substr(0, 3) == "rot")
	{
		if (!mShapeStack.empty())
		{
			float32 x = 0;
			float32 y = 0;
			float32 z = 0;
			sscanf(cmd.c_str(), " rot %f %f %f", &x, &y, &z);
			Quat quatLog( DegToRad(x), DegToRad(y), DegToRad(z), 0);
			Quat q = QuatExp(quatLog);
			mShapeStack.back()->q = q * mShapeStack.back()->q;
			Status("Shape rotated.");
			SwitchToStackMode();
		}
		else
		{
			Status("Shape stack is empty.");
		}
	}
	else if (cmd.substr(0,6) == "saucer")
	{
		float32 r = 3;
		float32 t = 1;
		sscanf(cmd.c_str(), " saucer %f %f", &r, &t);
		MapPtr<CollideGeometry> geom = new CollideSaucer(r,t);
		MapPtr<RenderPolytope> model = new RenderPolytope();
		model->Init(*geom, mGenerationThreshold);
		mShapeStack.push_back( new Shape(geom, model) );
		Status("Saucer created.");
		SwitchToStackMode();
	}
	else if (cmd.substr(0,4) == "save")
	{
		if (mShapeStack.size() >= 1)
		{
			char name[100];
			if (sscanf(cmd.c_str(), " save %s", name) == 1)
			{
				for (char* c = name; *c; c++)
				{
					*c = tolower(*c);
				}
				mShapeSet[name] = mShapeStack.back();
				mShapeStack.pop_back();
			}
			Status("Saved shape.");
			SwitchToStackMode();
		}
		else
		{
			Status("Shape stack is empty.");
		}
	}
	else if (cmd.substr(0,7) == "segment")
	{
		float32 x = 5;
		sscanf(cmd.c_str(), " segment %f", &x);
		MapPtr<CollideGeometry> geom = new CollideSegment(x);
		MapPtr<RenderPolytope> model = new RenderPolytope();
		MapPtr<CollideSphere> renderSphere = new CollideSphere(0.25f);
		MapPtr<CollideGeometry> renderShape = new CollideSum(geom, renderSphere);
		model->Init(*renderShape, mGenerationThreshold);
		mShapeStack.push_back( new Shape(geom, model) );
		Status("Segment created.");
		SwitchToStackMode();
	}
	else if (cmd.substr(0,6) == "sphere")
	{
		float32 rx = 5;
		float32 ry = 5;
		float32 rz = 5;
		int32 count = sscanf(cmd.c_str(), "sphere %f %f %f", &rx, &ry, &rz);
		MapPtr<CollideGeometry> geom;
		if (count == 1)
		{
			geom = new CollideSphere(rx);
		}
		else
		{
			geom = new CollideEllipsoid(Vector(rx, ry, rz));
		}
		MapPtr<RenderPolytope> model = new RenderPolytope();
		model->Init(*geom, mGenerationThreshold);
		mShapeStack.push_back( new Shape(geom, model) );
		Status("Sphere created.");
		SwitchToStackMode();
	}
	else if (cmd == "swap")
	{
		if (mShapeStack.size() >= 2)
		{
			MapPtr<Shape> s1 = mShapeStack.back();
			mShapeStack.pop_back();

			MapPtr<Shape> s2 = mShapeStack.back();
			mShapeStack.pop_back();

			mShapeStack.push_back(s1);
			mShapeStack.push_back(s2);

			Status("Shapes swapped.");
			SwitchToStackMode();
		}
		else
		{
			Status("Shape stack is empty.");
		}
	}
	else if (cmd == "sweep")
	{
		if (mShapeStack.size() >= 2)
		{
			MapPtr<Shape> shape1 = mShapeStack.back();
			mShapeStack.pop_back();

			MapPtr<Shape> shape2 = mShapeStack.back();
			mShapeStack.pop_back();

			MapPtr<Shape> newShape = new Shape();
			newShape->geom = new CollideSum(shape1->geom, shape1->q, shape1->x, shape2->geom, shape2->q, shape2->x);
			newShape->model = new RenderPolytope();
			newShape->model->Init( *(newShape->geom), mGenerationThreshold );
			mShapeStack.push_back(newShape);

			Status("Minkowski sum (sweep) created.");
			SwitchToStackMode();
		}
		else
		{
			Status("Minkowski sum requires two shapes.");
		}
	}
	else if (cmd == "time")
	{
		mDebugTime = !mDebugTime;
		if (mDebugTime)
			Status("Timing stats enabled.");
		else
			Status("Timing stats disabled.");
		SwitchToSimulateMode();
	}
	else if (cmd == "wrap")
	{
		if (mShapeStack.size() >= 2)
		{
			MapPtr<Shape> shape1 = mShapeStack.back();
			mShapeStack.pop_back();

			MapPtr<Shape> shape2 = mShapeStack.back();
			mShapeStack.pop_back();

			MapPtr<Shape> newShape = new Shape();
			newShape->geom = new CollideMax(shape1->geom, shape1->q, shape1->x, shape2->geom, shape2->q, shape2->x);
			newShape->model = new RenderPolytope();
			newShape->model->Init( *(newShape->geom), mGenerationThreshold );
			mShapeStack.push_back(newShape);

			Status("Minkowski max (shrink wrap) created.");
			SwitchToStackMode();
		}
		else
		{
			Status("Minkowski max requires two shapes.");
		}
	}
	else
	{
		Status("Command not recognized.  Type 'help' for list of commands.");
	}
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::DropAll()
{
	gTimeStamp += 100;
	ShuffleRigidBodies();
	for (uint32 i = mFirstSimulatedRigidBody; i < mRigidBody.size(); i++)
	{
		mRigidBody[i]->body->x = Vector(0, 0, i*25);
		mRigidBody[i]->body->v = Vector(0, 0, 0);
		mRigidBody[i]->body->omega = Vector( RandFloat(-1, 1), RandFloat(-1, 1), RandFloat(-1, 1) );
	}

	ArbiterIt it = mArbiterMap.begin();
	ArbiterIt end = mArbiterMap.end();
	while (it != end)
	{
		ContactList::iterator contactIt = (*it).second->contacts.begin();
		ContactList::iterator endContactIt = (*it).second->contacts.end();
		while (contactIt != endContactIt)
		{
			ContactList::iterator next = contactIt;
			next++;
			delete (*contactIt);
			(*it).second->contacts.erase(contactIt);
			contactIt = next;
		}

		delete (*it).second;

		it++;
	}

	mArbiterMap.clear();
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::Drop()
{
	static int32 b = 0;
	b++;
	if ( b < mFirstSimulatedRigidBody ) b = mFirstSimulatedRigidBody;
	if ( b >= (int32) mRigidBody.size() ) b = mFirstSimulatedRigidBody;
	mRigidBody[b]->body->x = Vector(0, 0, 60);
	mRigidBody[b]->body->v = Vector(0, 0, 0);
	mRigidBody[b]->body->omega = Vector(0, 0, 0);
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::Drag(float32 linear, float32 angular)
{
	mLinearDrag = linear;
	mAngularDrag = angular;
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::Status(const char* str)
{
	mStatusLine = str;
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::ClearArbiters()
{
		ArbiterIt it;
		it = mArbiterMap.begin();
		while (it != mArbiterMap.end())
		{
			delete it->second;
			it++;
		}
		mArbiterMap.clear();

}

//////////////////////////////////////////////////////////////

void TestXenoCollide::SwitchToSimulateMode()
{
	mStackMode = false;
	gSimulate = true;
	gCullFrontFace = false;
}

//////////////////////////////////////////////////////////////

void TestXenoCollide::SwitchToStackMode()
{
	mStackMode = true;
	gSimulate = false;
	gCullFrontFace = false;
}

//////////////////////////////////////////////////////////////

