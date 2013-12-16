#include "Box2D.h"
#include "Mat4x4.h"
#include "Shape.h"
#include "World.h"

#define SCALE 30.0f
#define W2B(x) ((x) / SCALE)
#define W2Bv(x) Vec2F(x[0] / SCALE, x[1] / SCALE)
#define B2Wv(x) Vec2F(x[0] * SCALE, x[1] * SCALE)

World* gWorld = 0;

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

static Mat4x4 ToMat(b2Transform transform)
{
	const Vec2F translation = Vec2F(transform.position(0), transform.position(1));
	const float angle = transform.GetAngle();
	
	Mat4x4 matT;
	Mat4x4 matR;
	matT.MakeTranslation(Vec3(translation[0], translation[1], 0.0f));
	matR.MakeRotationZ(angle);
	Mat4x4 mat = matT * matR;

	return mat;
}

class MyQueryCallback : public b2QueryCallback
{
public:
	MyQueryCallback(Vec2F _point)
	{
		point.Set(_point[0], _point[1]);
		result = 0;
	}
	
	bool ReportFixture(b2Fixture* fixture)
	{
		b2Body* body = fixture->GetBody();
//		b2Shape* shape = fixture->GetShape();
		
//		b2Transform transform = body->GetTransform();

//		bool hit = shape->TestPoint(transform, point);
		bool hit = fixture->TestPoint(point);
		
		if (hit)
		{
			result = body;
			
			pointInObject = body->GetLocalPoint(point);
/*			Mat4x4 mat = ToMat(body->GetTransform());
			mat = mat.CalcInv();
			Vec3 pointInObject2 = mat * Vec3(point(0), point(1), 0.0f);
			Assert(pointInObject2[2] == 0.0f);
			pointInObject = Vec2F(pointInObject2[0], pointInObject2[1]);*/
			
			return false;
		}
		
		return true;
	}
	
	b2Vec2 point;
	b2Body* result;
	b2Vec2 pointInObject;
};

World::World()
{
	mWorld = 0;
	mCurrentId = 0;
	
	// create Box2D world
	
//	b2Vec2 gravity(0.0f, 10.0f);
	b2Vec2 gravity(0.0f, 0.0f);
	bool doSleep = true;

	mWorld = new b2World(gravity, doSleep);
}

World::~World()
{
	delete mWorld;
	mWorld = 0;
}

int World::Add(Shape* shape, Vec2F position, float angle)
{
	position = W2Bv(position);
	
	int id = mCurrentId;
	
	mCurrentId++;
	
	// todo: add to Box2D
	
	b2BodyDef bodyDef;
	if (!shape->fixed)
		bodyDef.type = b2_dynamicBody;
	else
		bodyDef.type = b2_staticBody;
	bodyDef.position.Set(position[0], position[1]);
	bodyDef.angle = angle;
	bodyDef.userData = new int(id);
	float damping = 0.8f;
	bodyDef.linearDamping = damping;
	bodyDef.angularDamping = damping;
	b2Body* body = mWorld->CreateBody(&bodyDef);
	
	b2Shape* b2shape = 0;
	
	b2CircleShape circleShape;
	b2PolygonShape polyShape;
	
	if (shape->type == ShapeType_Circle)
	{
		circleShape.m_radius = W2B(shape->circle.radius);
		b2shape = &circleShape;
	}
	if (shape->type == ShapeType_Convex)
	{
		b2Vec2 vertices[MAX_SIDES];
		for (int i = 0; i < shape->convex.coordCount; ++i)
		{
			Vec2F v = W2B(shape->convex.coords[i]);
			vertices[i].Set(v[0], v[1]);// = b2Vec2(v[0], v[1]);
		}
		polyShape.Set(vertices, shape->convex.coordCount);
		b2shape = &polyShape;
	}
	
	Assert(b2shape);
		
//	b2PolygonShape b2shape;
//	b2shape.SetAsBox(shape->circle.radius, shape->circle.radius); // fixme
	
	b2FixtureDef fixtureDef;
	fixtureDef.shape = b2shape;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	fixtureDef.filter.maskBits = 1;
	fixtureDef.filter.categoryBits = 1;
	body->CreateFixture(&fixtureDef);
	
	mBodiesById[id] = body;
	
	return id;
}

void World::Remove(int objectId)
{
	// remove from Box2D
	
	std::map<int, b2Body*>::iterator i = mBodiesById.find(objectId);
	
	if (i == mBodiesById.end())
		throw ExceptionVA("body does not exst");
	
	b2Body* body = i->second;
	
	mBodiesById.erase(i);
	
	int* id = (int*)body->GetUserData();
	delete id;
	
	mWorld->DestroyBody(body);
}

void World::SetPosition(int objectId, Vec2F position)
{
	position = W2Bv(position);
	
	// todo: move Box2D body
	
	std::map<int, b2Body*>::iterator i = mBodiesById.find(objectId);
	
	if (i == mBodiesById.end())
		throw ExceptionVA("body does not exst");
	
	b2Body* body = i->second;
	
	b2Transform tf = body->GetTransform();
	tf.position.x = position[0];
	tf.position.y = position[1];
	
	body->SetTransform(tf.position, tf.GetAngle());
	
	body->SetAwake(true);
}

void World::ApplyForce(int objectId, Vec2F position, Vec2F force, bool inBody)
{
	position = W2Bv(position);
	force = W2Bv(force);
	
	std::map<int, b2Body*>::iterator i = mBodiesById.find(objectId);
	
	if (i == mBodiesById.end())
		throw ExceptionVA("body does not exst");
	
	b2Body* body = i->second;
	
	b2Vec2 position2(position[0], position[1]);
	
	if (inBody)
	{
		position2 = body->GetWorldPoint(position2);
	}
	
	body->ApplyForce(b2Vec2(force[0], force[1]), position2);
}

void World::SetLinearVelocity(int objectId, Vec2F speed)
{
	speed = W2Bv(speed);
	
	std::map<int, b2Body*>::iterator i = mBodiesById.find(objectId);
	
	if (i == mBodiesById.end())
		throw ExceptionVA("body does not exst");
	
	b2Body* body = i->second;
	
	body->SetLinearVelocity(b2Vec2(speed[0], speed[1]));
}

void World::Update(float dt)
{
	// update Box2D
	
	int velIterationCount = 10;
	int posIterationCount = 10;
	
	mWorld->Step(dt, velIterationCount, posIterationCount);

	if (!mWorld->GetAutoClearForces())
		mWorld->ClearForces();
}

void World::Describe(int objectId, Vec2F& out_Position, float& out_Angle)
{
	// query Box2D for position/angle
	
	std::map<int, b2Body*>::iterator i = mBodiesById.find(objectId);
	
	if (i == mBodiesById.end())
		throw ExceptionVA("body does not exst");
	
	b2Body* body = i->second;
	
	b2Transform tf = body->GetTransform();
	
	out_Position[0] = tf.position.x;
	out_Position[1] = tf.position.y;
	out_Angle = tf.GetAngle();
	
	out_Position = B2Wv(out_Position);
}

WorldObjectDesc World::Describe(int objectId)
{
	WorldObjectDesc desc;
	
	Describe(objectId, desc.position, desc.angle);
	
	return desc;
}

int World::TestPoint(Vec2F position, Vec2F& out_PointInObject)
{
	position = W2Bv(position);
	
	MyQueryCallback callback(position);
	b2AABB aabb;
	aabb.lowerBound.Set(position[0], position[1]);
	aabb.upperBound.Set(position[0], position[1]);
	mWorld->QueryAABB(&callback, aabb);
	
	if (!callback.result)
		return -1;
	
	int* id = (int*)callback.result->GetUserData();
	out_PointInObject = B2Wv(Vec2F(callback.pointInObject(0), callback.pointInObject(1)));
	
	return *id;
}

Vec2F World::ObjectToWorld(int objectId, Vec2F position)
{
	position = W2Bv(position);
	
	std::map<int, b2Body*>::iterator i = mBodiesById.find(objectId);
	
	if (i == mBodiesById.end())
		throw ExceptionVA("body does not exst");
	
	b2Body* body = i->second;
	
	b2Vec2 position2(position[0], position[1]);
	
	position2 = body->GetWorldPoint(position2);
	
	return B2Wv(Vec2F(position2(0), position2(1)));
}
