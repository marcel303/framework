#include "framework.h"
#include "FrameworkGuiHelper.h"
#include "FrameworkRenderInterface.h"
#include "SimpleDynamicsWorld.h"

// todo : need textures for sphere for render interface shapes. will visualize rolling objects much better

static const btVector3 & toBT(const Vec3 & v)
{
	return *(btVector3*)&v;
}

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
	
	btStaticPlaneShape planeShape(btVector3(0, 1, 0), 0);
	world.createRigidBody(btTransform(btQuaternion::getIdentity()), 0.f, &planeShape)
		->setRestitution(.8f);
	
	btVector3 positions[] = { btVector3(0, 0, -.1f), btVector3(-.1f, 0, +.1f), btVector3(+.1f, 0, +.1f) };
	btScalar radii[] = { .1f, .1f, .1f };
	btMultiSphereShape sphereShape(positions, radii, 3);
	for (int i = 0; i < 140; ++i)
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
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;

		mouse.showCursor(false);
		mouse.setRelative(true);
		
		camera.tick(framework.timeStep, true);
		
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
					mouse.isDown(BUTTON_LEFT)
					? 10.f
					: 2.f;
				
				strength *= lerp<float>(1.f, 2.f, sinf(framework.time / 3.456f) + 1.f);
				
				body->applyCentralForce(delta * strength);
			}
		}
		
		syncPhysicsToGraphics(world.dynamicsWorld, &renderInterface);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			{
				pushBlend(BLEND_OPAQUE);
				pushDepthTest(true, DEPTH_LESS);
				{
					setColor(colorRed);
					fillCube(whirl_pos, Vec3(.1f, .1f, .1f));
					
					//world.debugDraw();
					
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
			camera.popViewMatrix();
		}
		framework.endDraw();
	}

	renderInterface.removeAllInstances();
	
	world.shut();

	framework.shutdown();

	return 0;
}