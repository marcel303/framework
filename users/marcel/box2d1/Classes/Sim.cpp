#include <OpenGLES/ES1/gl.h>
#include "Calc.h"
#include "Sim.h"
#include "SpriteGfx.h"

#define GROUP_THINGIES (1 << 0)
#define GROUP_BASE (1 << 1)
#define GROUP_ALL 0xFFFF

#define N1 50
#define N2 25
//#define N1 100
//#define N2 50

class MyContactListener : b2ContactListener
{
public:
	virtual void BeginContact(b2Contact* contact)
	{
		B2_NOT_USED(contact);
	}

	virtual void EndContact(b2Contact* contact) 
	{
		B2_NOT_USED(contact); 
	}
};

static inline float Random(float min, float max)
{
	float t = (rand() & 4095) / 4095.0f;

	return min + (max - min) * t;
}

static void GenerateWorld(b2World* w)
{
	// generate ground plane

	{
		b2BodyDef bodyDef;
		bodyDef.type = b2_staticBody;
		bodyDef.position.Set(0.0f, 12.0f);
		b2PolygonShape shape;
		shape.SetAsBox(20.0f, 0.5f);
		b2Body* body = w->CreateBody(&bodyDef);
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &shape;
		fixtureDef.density = 0.0f;
		fixtureDef.filter.maskBits = GROUP_ALL;
		fixtureDef.filter.categoryBits = GROUP_BASE;
		body->CreateFixture(&fixtureDef);
	}

	// generate boxes

	for (int i = 0; i < N2; ++i)
	{
		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(Random(-10.0f, +10.0f), Random(0.0f, 10.0f));
		bodyDef.angle = Random(0.0f, b2_pi);
		b2PolygonShape shape;
		shape.SetAsBox(Random(0.4f, 0.7f), Random(0.4f, 0.7f));
		b2Body* body = w->CreateBody(&bodyDef);
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &shape;
		fixtureDef.density = 1.0f;
		fixtureDef.filter.maskBits = GROUP_ALL;
		fixtureDef.filter.categoryBits = GROUP_BASE;
		body->CreateFixture(&fixtureDef);
	}
}


Sim::Sim()
{
	world = 0;
}

Sim::~Sim()
{
	delete world;
	world = 0;
}

void Sim::Setup()
{
	b2Vec2 gravity(0.0f, 10.0f);
	bool doSleep = true;

	world = new b2World(gravity, doSleep);

	b2BodyDef staticBodyDef;
	staticBodyDef.position.Set(0.0f, 0.0f);
	b2Body* staticBody = world->CreateBody(&staticBodyDef);
	b2PolygonShape staticShape;
	staticShape.SetAsBox(2.0f, 2.0f);
	staticBody->CreateFixture(&staticShape, 0.0f);

	//

	GenerateWorld(world);

	for (int i = 0; i < N1; ++i)
	{
		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(Random(-10.0f, +10.0f), Random(-10.0f, 0.0f));

		b2Body* body = world->CreateBody(&bodyDef);
		b2PolygonShape shape;
		shape.SetAsBox(0.5f, 0.5f);
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &shape;
		fixtureDef.density = 1.0f;
		//fixtureDef.friction = 0.1f;
		fixtureDef.restitution = 0.9f;
		fixtureDef.filter.maskBits = GROUP_BASE | GROUP_THINGIES;
//		fixtureDef.filter.maskBits = 0;
		fixtureDef.filter.categoryBits = GROUP_THINGIES;
		body->CreateFixture(&fixtureDef);
		b2DistanceJointDef jointDef;
		jointDef.Initialize(staticBody, body, staticBody->GetPosition(), body->GetPosition() + b2Vec2(0.0f, 0.0f));
		world->CreateJoint(&jointDef);
		bodyList.push_back(body);
	}
}

void Sim::Update(float dt)
{
	int velIterationCount = 10;
	int posIterationCount = 10;
	
	world->Step(dt, velIterationCount, posIterationCount);

	if (!world->GetAutoClearForces())
		world->ClearForces();
}

void Sim::RenderGL(SpriteGfx* gfx)
{
	for (b2Body* body = world->GetBodyList(); body; body = body->GetNext())
	{
		LOG_DBG("renderBody", 0);
		
		b2Transform tf = body->GetTransform();
		float angle = tf.GetAngle();
		if (angle < 0.0f)
			angle += Calc::m2PI;
		
		float axis[4];
		Calc::RotAxis_Fast(angle, axis);

		for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext())
		{
			b2Shape* shape = fix->GetShape();
			
			if (shape->m_type == b2Shape::e_polygon)
			{
				b2PolygonShape* poly = (b2PolygonShape*)shape;

				float c = (sinf(angle) + 1.0f) * 0.5f;
				c = 0.5f + c * (1.0f - 0.5f);
				
				int vertexCount = poly->GetVertexCount();
				int indexCount = (vertexCount - 2) * 3;
				
				gfx->Reserve(vertexCount, indexCount);
				gfx->WriteBegin();
				for (int i = 0; i < vertexCount; ++i)
				{
					b2Vec2 pos = poly->GetVertex(i);
					float x = axis[0] * pos.x + axis[1] * pos.y + tf.position.x;
					float y = axis[2] * pos.x + axis[3] * pos.y + tf.position.y;
					
//					gfx->WriteVertex(x, y, SpriteColor_Make(255, 255, 255, 255).rgba, 0.0f, 0.0f);
					gfx->WriteVertex(x, y, SpriteColor_MakeF(c, c, c, 1.0f).rgba, 0.0f, 0.0f);
				}
				for (int i = 0; i < vertexCount - 2; ++i)
				{
					int v1 = 0;
					int v2 = i + 1;
					int v3 = i + 2;
					gfx->WriteIndex3(v1, v2, v3);
				}
				gfx->WriteEnd();
				
			}
		}
	}
}
