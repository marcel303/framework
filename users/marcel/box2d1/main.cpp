#include "Box2D/Box2D.h"
#include "framework.h"

static void drawBox2dPolygonShape(b2PolygonShape & shape, b2Body & body)
{
	const b2Vec2 position = body.GetPosition();
	const float32 angle = body.GetAngle() * 180.f / M_PI;
	
	gxPushMatrix();
	{
		gxTranslatef(position.x, position.y, 0);
		gxRotatef(angle, 0, 0, 1);
		
		gxBegin(GX_TRIANGLE_FAN);
		{
			for (int i = 0; i < b2_maxPolygonVertices; ++i)
				gxVertex2f(shape.m_vertices[i].x, shape.m_vertices[i].y);
		}
		gxEnd();
	}
	gxPopMatrix();
}

struct Box
{
	b2PolygonShape shape;
	b2Body * body = nullptr;
	Color color;
	
	void init(b2World & world, const float x, const float y, const float size)
	{
		// Define the dynamic body. We set its position and call the body factory.
		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(x, y);
		body = world.CreateBody(&bodyDef);

		// Define a box shape for our dynamic body.
		shape.SetAsBox(size, size);

		// Define the dynamic body fixture.
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &shape;

		// Set the box density to be non-zero, so it will be dynamic.
		fixtureDef.density = 1.0f;

		// Override the default friction.
		fixtureDef.friction = 0.8f;
		
		fixtureDef.restitution = .8f;

		// Add the shape to the body.
		body->CreateFixture(&fixtureDef);
	}
};

struct ContactListener : b2ContactListener
{
	virtual void BeginContact(b2Contact * contact) override
	{
		auto * manifold = contact->GetManifold();
		
		for (int i = 0; i < manifold->pointCount; ++i)
		{
			// todo : come up with something better
			
			auto velA = contact->GetFixtureA()->GetBody()->GetLinearVelocity();
			auto velB = contact->GetFixtureA()->GetBody()->GetLinearVelocity();
			auto dot = b2Dot(velA, velB);
			auto amount = sqrtf(fmaxf(dot, 0.f));
			
			const int volume = clamp<int>(amount * 10.f, 0, 255);
			
			Sound("menuselect.ogg").play(volume);
		}
	}

	virtual void EndContact(b2Contact * contact) override
	{
	}
};

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	if (!framework.init(800, 600))
		return -1;
	
	// Define the gravity vector.
	b2Vec2 gravity(0.0f, -10.0f);

	// Construct a world object, which will hold and simulate the rigid bodies.
	b2World world(gravity);
	
	ContactListener contactListener;
	world.SetContactListener(&contactListener);
	
	// Define the ground body.
	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(0.0f, -10.0f);

	// Call the body factory which allocates memory for the ground body
	// from a pool and creates the ground box shape (also from a pool).
	// The body is also added to the world.
	b2Body* groundBody = world.CreateBody(&groundBodyDef);

	// Define the ground box shape.
	b2PolygonShape groundBox;

	// The extents are the half-widths of the box.
	groundBox.SetAsBox(50.0f, 1.0f);

	// Add the ground fixture to the ground body.
	groundBody->CreateFixture(&groundBox, 0.0f);
	
	// Create a bunch of boxes.
	std::vector<Box*> boxes;
	for (int i = 0; i < 20; ++i)
	{
		Box * box = new Box();
		box->init(world, random<float>(-6.f, +6.f), random<float>(2.f, 50.f), 1.f);
		box->color = Color::fromHSL(.5f + i / 20.f * .1f, .5f, .5f);
		boxes.push_back(box);
	}

	// Prepare for simulation. Typically we use a time step of 1/60 of a
	// second (60Hz) and 10 iterations. This provides a high quality simulation
	// in most game scenarios.
	float32 maxTimeStep = 1.0f / 30.0f;
	int32 velocityIterations = 6;
	int32 positionIterations = 2;
	
	const float viewScale = 10.f;
	
	// This is our little game loop.
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		const float timeStep = fminf(framework.timeStep, maxTimeStep);
		
		const Vec2 mousePos_world = (Vec2(mouse.x, -mouse.y) - Vec2(400, -300)) / viewScale;
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			for (auto * box : boxes)
			{
				const float x = mousePos_world[0] + random<float>(-2.f, +2.f);
				const float y = mousePos_world[1] + random<float>(2.f, 50.f);
				
				box->body->SetTransform(b2Vec2(x, y), 0.f);
			}
		}
		
		// Instruct the world to perform a single step of simulation.
		// It is generally best to keep the time step and iterations fixed.
		world.Step(timeStep, velocityIterations, positionIterations);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxTranslatef(400, 300, 0);
			gxScalef(1, -1, 0);
			gxScalef(viewScale, viewScale, 1);
			
			setColor(200, 200, 200);
			drawBox2dPolygonShape(groundBox, *groundBody);
			
			for (auto * box : boxes)
			{
				setColor(box->color);
				drawBox2dPolygonShape(box->shape, *box->body);
			}
		}
		framework.endDraw();
	}
	
	for (auto & box : boxes)
	{
		delete box;
		box = nullptr;
	}
	
	boxes.clear();
	
	framework.shutdown();
	
	return 0;
}
