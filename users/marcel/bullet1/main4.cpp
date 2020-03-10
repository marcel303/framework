#include "framework.h"
#include "FrameworkGuiHelper.h"
#include "FrameworkRenderInterface.h"
#include "SimpleDynamicsWorld.h"

#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.windowIsResizable = true;
	
	if (!framework.init(800, 600))
		return -1;

	SimpleDynamicsWorld world;
	world.init();
	
	Camera3d camera;
	camera.position[1] = 1.f;
	camera.position[2] = -1.4f;
	
	// add the ground plane
	
	btBoxShape boxShape(btVector3(100, 0.1, 100));
	world.createRigidBody(btTransform(btQuaternion::getIdentity()), 0.f, &boxShape)
		->setRestitution(.8f);
	
	// add a box to contain all of the interactable objects by adding four walls
	
	btBoxShape wallShape(btVector3(0.1, 2.0, 10.0));
	{
		const float boxSize = 8.f;
		
		world.createRigidBody(btTransform(btQuaternion::getIdentity(), btVector3(-boxSize, 0.0, 0.0)), 0.f, &wallShape)->setRestitution(.8f);
		world.createRigidBody(btTransform(btQuaternion::getIdentity(), btVector3(+boxSize, 0.0, 0.0)), 0.f, &wallShape)->setRestitution(.8f);
		
		world.createRigidBody(btTransform(btQuaternion(btVector3(0,1,0), M_PI/2.0), btVector3(0.0, 0.0, -boxSize)), 0.f, &wallShape)->setRestitution(.8f);
		world.createRigidBody(btTransform(btQuaternion(btVector3(0,1,0), M_PI/2.0), btVector3(0.0, 0.0, +boxSize)), 0.f, &wallShape)->setRestitution(.8f);
	}
	
	// add spheres
	
	btSphereShape sphereShape(0.5);
	for (int i = 0; i < 100; ++i)
	{
		world.createRigidBody(
			btTransform(
				btQuaternion::getIdentity(),
				btVector3(
					random<float>(-4.f, +4.f),
					4,
					random<float>(-4.f, +4.f))),
				2.5f,
				&sphereShape)
			->setRestitution(.8f);
	}

	// add composite objects consisting of three spheres

	btVector3 positions[] = { btVector3(0, 0, -.5f), btVector3(-.5f, 0, +.5f), btVector3(+.5f, 0, +.5f) };
	btScalar radii[] = { .5f, .5f, .5f };
	btMultiSphereShape spheresShape(positions, radii, 3);
	for (int i = 0; i < 10; ++i)
	{
		world.createRigidBody(
			btTransform(
				btQuaternion::getIdentity(),
				btVector3(
					random<float>(-4.f, +4.f),
					4,
					random<float>(-4.f, +4.f))),
				2.5f,
				&spheresShape)
			->setRestitution(.8f);
	}

	// prepare to draw the objects using the framework renderer
	
	FrameworkRenderInterface renderInterface;
	renderInterface.init();
	
	autogenerateGraphicsObjects(&renderInterface, world.dynamicsWorld);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;

		mouse.showCursor(false);
		mouse.setRelative(true);
		
		camera.tick(framework.timeStep, true);
		
		// remember the point we hit (if any) so we can draw the hit test indicator
		
		btVector3 hitPoint;
		bool hasHitPoint = false;

		// apply a force or impulse to push objects around when the mouse button is pressed
		
		//if (mouse.wentDown(BUTTON_LEFT)) // for applying impulse
		if (mouse.isDown(BUTTON_LEFT)) // for applying force
		{
			const Vec3 cameraDirection = camera.getWorldMatrix().GetAxis(2);
			
			const btVector3 origin(
				camera.position[0],
				camera.position[1],
				camera.position[2]);
			
			const btVector3 direction(
				cameraDirection[0],
				cameraDirection[1],
				cameraDirection[2]);
			
			const btVector3 target = origin + direction * 100.0;
			
			btCollisionWorld::ClosestRayResultCallback rayCallback(
				origin,
				target);

			rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;

			world.dynamicsWorld->rayTest(origin, target, rayCallback);
			
			if (rayCallback.hasHit())
			{
				btVector3 pickPos = rayCallback.m_hitPointWorld;
				btRigidBody* body = const_cast<btRigidBody*>(btRigidBody::upcast(rayCallback.m_collisionObject));

				if (body != nullptr && !body->isStaticObject())
				{
					const btTransform worldToBody = body->getWorldTransform().inverse();
					
					const btVector3 hitPoint_body = worldToBody * rayCallback.m_hitPointWorld;
					
					//body->applyImpulse(direction * 4.0, hitPoint_body);
					body->applyForce(direction * 100.0, hitPoint_body);
					body->activate();
					
					// remember the world-space location of the hit test result
					
					hitPoint = rayCallback.m_hitPointWorld;
					hasHitPoint = true;
				}
			}
		}
		
		// simulate world
		
		world.dynamicsWorld->stepSimulation(framework.timeStep);
		
		// update renderable shape properties (e.g. transforms)
		
		syncPhysicsToGraphics(world.dynamicsWorld, &renderInterface);
		
		// draw
		
		framework.beginDraw(40, 40, 40, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			{
				pushBlend(BLEND_OPAQUE);
				pushDepthTest(true, DEPTH_LESS);
				{
					// draw the shapes
					
					//world.debugDraw();
					
					renderInterface.renderScene();
					
					// draw the hit test indicator
					
					if (hasHitPoint)
					{
						setColor(colorRed);
						fillCube(Vec3(hitPoint[0], hitPoint[1], hitPoint[2]), Vec3(.02f, .02f, .02f));
					}
				}
				popDepthTest();
				popBlend();
			}
			camera.popViewMatrix();
			
			// draw the screen-space overlay
			
			projectScreen2d();
			
			int sx, sy;
			framework.getCurrentViewportSize(sx, sy);
			setColor(100, 100, 200, 100);
			fillCircle(sx/2, sy/2, 10, 10);
			
			setFont("engine/fonts/Roboto-Regular.ttf");
			setColor(0, 0, 0, 60);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(4, 4, 240, 40, 4.f);
			hqEnd();
			setColor(255, 255, 255, 200);
			drawText(10, 10, 14, +1, +1, "Bullet physics - ray test and apply impulse");
		}
		framework.endDraw();
	}

	renderInterface.removeAllInstances();

	world.shut();

	framework.shutdown();

	return 0;
}
