#include "Calc.h"
#include "GameObject.h"
#include "Log.h"
#include "OpenGLCompat.h"
#include "render.h"
#include "Shape.h"
#include "World.h"

GameObject::GameObject(Shape* shape, bool ownShape, int objectId, PrimType type)
{
	Assert(shape);
	Assert(ownShape);
	
	mShape = 0;
	mObjectId = 0;
	mType = type;
	mIsDead = false;
	
	//
	
	mShape = shape;
	mObjectId = objectId;
}

GameObject::~GameObject()
{
	delete mShape;
	mShape = 0;
}

void GameObject::Update(float dt)
{
}

void GameObject::Render()
{
	//LOG_DBG("go: render", 0);
	
	// get physical object state
	
	Vec2F position;
	float angle;
	
	gWorld->Describe(mObjectId, position, angle);
	
	// setup transform
	
	glPushMatrix();
	glTranslatef(position[0], position[1], 0.0f);
	glRotatef(Calc::RadToDeg(angle), 0.0f, 0.0f, 1.0f);
	
	// render shape
	
	switch (mShape->type)
	{
		case ShapeType_Circle:
			gRender->Circle(0.0f, 0.0f, mShape->circle.radius, Color(1.0f, 1.0f, 0.5f));
			break;
		case ShapeType_Convex:
			gRender->Poly(mShape->convex.coords, mShape->convex.coordCount, Color(1.0f, 0.5f, 1.0f));
			break;
	}
	
	glPopMatrix();
}

void GameObject::Disappear(ScreenSide side)
{
	mIsDead = true;
}
