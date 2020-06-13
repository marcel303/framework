#include "framework.h"
#include "FrameworkGuiHelper.h"
#include "FrameworkRenderInterface.h"
#include "SimpleDynamicsWorld.h"

#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"

static const btVector3 toBT(const Vec3 & v)
{
	return btVector3(v[0], v[1], v[2]);
}

static btRigidBody * raycast(btCollisionWorld * world, int pointerIndex)
{
	Vec3 cameraPosition = framework.getHeadTransform().GetTranslation();
	Vec3 cameraDirection = framework.getHeadTransform().GetAxis(2);

	if (pointerIndex != -1)
	{
		if (vrPointer[pointerIndex].hasTransform)
		{
			const Mat4x4 transform = vrPointer[pointerIndex].getTransform(framework.vrOrigin);
			cameraPosition = transform.GetTranslation();
			cameraDirection = transform.GetAxis(2);
		}
		else
		{
			return nullptr;
		}
	}

	const btVector3 origin(
		cameraPosition[0],
		cameraPosition[1],
		cameraPosition[2]);

	const btVector3 direction(
		cameraDirection[0],
		cameraDirection[1],
		cameraDirection[2]);

	const btVector3 target = origin + direction * 100.0;

	btCollisionWorld::ClosestRayResultCallback rayCallback(
		origin,
		target);

	rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;

	world->rayTest(origin, target, rayCallback);

	if (rayCallback.hasHit())
	{
		btVector3 pickPos = rayCallback.m_hitPointWorld;
		btRigidBody* body = const_cast<btRigidBody*>(btRigidBody::upcast(rayCallback.m_collisionObject));

		if (body != nullptr && !body->isStaticObject())
		{
			return body;
		}
	}
	
	return nullptr;
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.windowIsResizable = true;
	
	framework.vrMode = true;
	framework.enableVrMovement = true;

	if (!framework.init(800, 600))
		return -1;

	SimpleDynamicsWorld world;
	world.init();

	framework.vrOrigin[1] = 1.f;
	framework.vrOrigin[2] = -1.4f;

	btBoxShape boxShape(btVector3(100, 0.1, 100));
	world.createRigidBody(btTransform(btQuaternion::getIdentity()), 0.f, &boxShape)
		->setRestitution(.8f);
	
	btSphereShape sphereShape1(btScalar(.2f));
	btRigidBody* previousBody = nullptr;
	for (int i = 0; i < 20; ++i)
	{
		auto * body = world.createRigidBody(
			btTransform(
				btQuaternion::getIdentity(),
				btVector3(
					lerp<float>(-20.f, +20.f, i / float(20 - 1)),
					4,
					0)),
				(i % 3) ? .5f : 0.f,
				&sphereShape1);
		
		body->setRestitution(.8f);
		
		if (previousBody != nullptr)
		{
			auto * rope = world.connectWithRope(previousBody, body, 20, 1.f);
			
			rope->generateBendingConstraints(10);
		}
		
		previousBody = body;
	}
	
	btVector3 positions[] = { btVector3(0, 0, -.1f), btVector3(-.1f, 0, +.1f), btVector3(+.1f, 0, +.1f) };
	btScalar radii[] = { .1f, .1f, .1f };
	btMultiSphereShape sphereShape(positions, radii, 3);
	for (int i = 0; i < 100; ++i)
	{
		world.createRigidBody(
			btTransform(
				btQuaternion::getIdentity(),
				btVector3(
					random<float>(-10.f, +10.f),
					4,
					random<float>(-10.f, +10.f))),
				.5f,
				&sphereShape)
			->setRestitution(.8f);
	}
	
	FrameworkRenderInterface renderInterface;
	renderInterface.init();
	
	autogenerateGraphicsObjects(&renderInterface, world.dynamicsWorld);
	
	btRigidBody * pickedBodies[3] = { };
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;

		mouse.showCursor(false);
		mouse.setRelative(true);
		
		world.dynamicsWorld->stepSimulation(framework.timeStep);
		
		Vec3 whirl_pos(
			cosf(framework.time / 1.234f) * 1.f,
			1.5f,
			cosf(framework.time / 2.345f) * 1.f);
		
		auto & collisionObjectArray = world.dynamicsWorld->getCollisionObjectArray();
		for (int i = 0; i < collisionObjectArray.size(); ++i)
		{
			auto * body = btRigidBody::upcast(collisionObjectArray[i]);
			
			if (body != nullptr)
			{
				auto & pos = body->getWorldTransform().getOrigin();
				auto delta = (toBT(whirl_pos) - pos).safeNormalize();
				
				auto strength =
					(mouse.isDown(BUTTON_LEFT) || vrPointer[0].isDown(VrButton_Trigger))
					? 10.f
					: 0.f;
				
				strength *= lerp<float>(1.f, 2.f, sinf(framework.time / 3.456f) + 1.f);
				
				body->applyCentralForce(delta * strength);
				body->activate();
			}
		}

		for (int i = 0; i < VrSide_COUNT; ++i)
		{
			auto & pointer = vrPointer[i];

			if (pointer.wentDown(VrButton_GripTrigger) && pointer.hasTransform)
			{
				pickedBodies[i] = raycast(world.dynamicsWorld, i);
			}
			
			if (pointer.isDown(VrButton_GripTrigger) && pointer.hasTransform)
			{
				auto * body = pickedBodies[i];

				if (body != nullptr)
				{
					const Mat4x4 pointerTransform = pointer.getTransform(framework.vrOrigin);
					
					btTransform transform;
					transform.setFromOpenGLMatrix(pointerTransform.m_v);
					
					body->setWorldTransform(transform);
					
				// fixme : don't reset velocities, and use some impulse/constraint method for re-positioning bodies. this doesn't work properly, as no energy is passed on to the objects the body will collide with, causing little physical reaction (just movement due to solving for collisions)
					body->setLinearVelocity(btVector3(0, 0, 0));
					body->setAngularVelocity(btVector3(0, 0, 0));
					body->activate();
				}
			}
			else
			{
				pickedBodies[i] = nullptr;
			}
		}

		syncPhysicsToGraphics(world.dynamicsWorld, &renderInterface);

		for (int i = 0; i < framework.getEyeCount(); ++i)
		{
			framework.beginEye(i, Color(40, 40, 40, 0));
			{
				pushBlend(BLEND_OPAQUE);
				pushDepthTest(true, DEPTH_LESS);
				{
					setColor(colorRed);
					fillCube(whirl_pos, Vec3(.1f, .1f, .1f));
					
					world.debugDraw();
					
					renderInterface.renderScene();
				}
				popDepthTest();
				popBlend();
				
				pushDepthTest(true, DEPTH_LESS, false);
				{
					gxPushMatrix();
					gxTranslatef(0, .01f, 0);
					setColor(0, 127, 255, 63);
					drawGrid3dLine(10, 10, 0, 2);
					gxPopMatrix();
				}
				popDepthTest();
			}
			
			if (!framework.isStereoVr())
			{
				projectScreen2d();
				setFont("engine/fonts/Roboto-Regular.ttf");
				setColor(0, 0, 0, 60);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				hqFillRoundedRect(4, 4, 240, 40, 4.f);
				hqEnd();
				setColor(255, 255, 255, 200);
				drawText(10, 10, 14, +1, +1, "Bullet physics - rope example");
			}

			framework.endEye();
		}

		framework.present();
	}

	renderInterface.removeAllInstances();
	
	world.shut();

	framework.shutdown();

	return 0;
}
