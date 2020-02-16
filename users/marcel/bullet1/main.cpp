#include "btBulletDynamicsCommon.h"
#include "BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.h"
#include "BulletDynamics/Featherstone/btMultiBodyConstraintSolver.h"

#include "FrameworkDebugDrawer.h"
#include "FrameworkGuiHelper.h"
#include "FrameworkRenderInterface.h"
#include "framework.h"

static btRigidBody * createRigidBody(
	btDynamicsWorld * dynamicsWorld,
	float mass,
	const btTransform & startTransform,
	btCollisionShape * shape,
	const btVector4 & color = btVector4(1, 0, 0, 1))
{
	btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));
	
	const bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		shape->calculateLocalInertia(mass, localInertia);

	auto * myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, shape, localInertia);
	auto * body = new btRigidBody(cInfo);

	body->setUserIndex(-1);
	
	dynamicsWorld->addRigidBody(body);
	
	return body;
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.msaaLevel = 4;
	
	if (!framework.init(800, 600))
		return -1;
	
	// collision configuration contains default setup for memory, collision setup
	auto * collisionConfiguration = new btDefaultCollisionConfiguration();

	//auto * filterCallback = new MyOverlapFilterCallback2();
	btOverlapFilterCallback * filterCallback = nullptr;
	auto * dispatcher = new btCollisionDispatcher(collisionConfiguration);

	auto * pairCache = new btHashedOverlappingPairCache();
	pairCache->setOverlapFilterCallback(filterCallback);

	auto * broadphase = new btDbvtBroadphase(pairCache);

	auto * solver = new btMultiBodyConstraintSolver();

	//
	
	auto * dynamicsWorld = new btMultiBodyDynamicsWorld(
		dispatcher,
		broadphase,
		solver,
		collisionConfiguration);

	dynamicsWorld->setGravity(btVector3(0, -10, 0));
	
	//
	
	auto * debugDrawer = new FrameworkDebugDrawer();
	
	debugDrawer->setDebugMode(
		1*btIDebugDraw::DBG_DrawWireframe |
		0*btIDebugDraw::DBG_DrawAabb |
		0*btIDebugDraw::DBG_DrawContactPoints |
		1*btIDebugDraw::DBG_DrawNormals);
	
	dynamicsWorld->setDebugDrawer(debugDrawer);
	
	//
	
	std::vector<btCollisionShape*> shapes;
	
	
	// add static ground plane
	
	{
		auto * shape = new btBoxShape(btVector3(40, 1, 40));
		shapes.push_back(shape);
		
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(0, 0, 0));
		auto * body = createRigidBody(dynamicsWorld, 0.f, transform, shape);
		body->setRestitution(.9f);
	}
	
	// add dynamic bodies
	
	{
		auto * boxShape = new btBoxShape(btVector3(1, 1, 1));
		shapes.push_back(boxShape);
		
		auto * sphereShape = new btSphereShape(1);
		shapes.push_back(sphereShape);
		
		for (int i = -10; i <= +10; i += 2)
		{
			btTransform transform;
			transform.setIdentity();
			transform.setOrigin(btVector3(i, 10, 0));
			transform.setRotation(
				btQuaternion(
					btVector3(1, 1, 1),
					random<float>(0.f, float(2.0*M_PI))));
			
			auto * body = createRigidBody(dynamicsWorld, 0.f, transform, boxShape);
			body->setRestitution(.9f);
			
			transform.setOrigin(btVector3(i, 6, 0));
			createRigidBody(dynamicsWorld, 0.f, transform, boxShape);
		}
		
		for (int i = 0; i < 2000; ++i)
		{
			const float x = random<float>(-40.f, +40.f);
			const float z = random<float>(-40.f, +40.f);
			
			btTransform transform;
			transform.setIdentity();
			transform.setOrigin(btVector3(x, 20, z));
			transform.setRotation(
				btQuaternion(
					btVector3(1, 1, 1),
					random<float>(0.f, float(2.0*M_PI))));
			
			const bool isRigid = (rand() % 3) == 0;
			
			btCollisionShape * shape = nullptr;
			if ((rand() % 3) == 0)
				shape = sphereShape;
			else
				shape = boxShape;
			
			auto * body = createRigidBody(dynamicsWorld, isRigid ? 0.f : 1.f, transform, shape);
			body->setRestitution(random<float>(.1f, .9f));
		}
	}
	
	// add camera body (so we can push things around)
	
	btRigidBody * cameraBody = nullptr;
	
	{
		auto * shape = new btBoxShape(btVector3(2, 2, 2));
		shapes.push_back(shape);
		
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(0, 1, 0));
		transform.setRotation(
			btQuaternion(
				btVector3(1, 1, 1),
				random<float>(0.f, float(2.0*M_PI))));
		
		cameraBody = createRigidBody(dynamicsWorld, 1.f, transform, shape);
		cameraBody->setCollisionFlags(btRigidBody::CF_DISABLE_VISUALIZE_OBJECT);
	}
	
	//
	
	FrameworkRenderInterface * renderInterface = new FrameworkRenderInterface();
	
	renderInterface->init();
	
	autogenerateGraphicsObjects(renderInterface, dynamicsWorld);
	
	//
	
	Camera3d camera;
	camera.position.Set(0, 4, -10);
	camera.maxForwardSpeed *= 3.f;
	camera.maxStrafeSpeed *= 3.f;
	camera.maxUpSpeed *= 3.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(camera.position[0], camera.position[1], camera.position[2]));
		//cameraBody->setWorldTransform(transform);
		cameraBody->setLinearVelocity(btVector3(0, 0, 0));
		cameraBody->activate();
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
			{
				btCollisionObject * obj = dynamicsWorld->getCollisionObjectArray()[i];
				btRigidBody * body = btRigidBody::upcast(obj);
				
				body->applyForce(btVector3(0, 100, 0), btVector3(.1, .1, .1));
				body->activate();
			}
		}
		
		const float dt = framework.timeStep;
		
		camera.tick(dt, true);
		
		dynamicsWorld->stepSimulation(dt);
		
		int viewSx;
		int viewSy;
		framework.getCurrentViewportSize(viewSx, viewSy);
		
		renderInterface->resize(viewSx, viewSy);
		
		syncPhysicsToGraphics(dynamicsWorld, renderInterface);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			
			camera.pushViewMatrix();
			{
			#if 1
				renderInterface->renderScene();
			#else
				for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
				{
					btCollisionObject * obj = dynamicsWorld->getCollisionObjectArray()[i];
					btRigidBody * body = btRigidBody::upcast(obj);
					
					if (body == cameraBody)
						continue;
					
					auto * shape = body->getCollisionShape();
					if (shape->getShapeType() != BOX_SHAPE_PROXYTYPE)
						continue;
					
					auto * boxShape = static_cast<btBoxShape*>(shape);
					
					float matrix[16];
					body->getWorldTransform().getOpenGLMatrix(matrix);
					
					const auto & extents = boxShape->getHalfExtentsWithoutMargin();
					
					pushShaderOutputs("n");
					gxPushMatrix();
					gxMultMatrixf(matrix);
					setColor(colorRed);
					fillCube(Vec3(0, 0, 0), Vec3(extents[0], extents[1], extents[2]) * .9f);
					gxPopMatrix();
					popShaderOutputs();
				}
			#endif
				
				//dynamicsWorld->debugDrawWorld();
			}
			camera.popViewMatrix();
			
			popDepthTest();
		}
		framework.endDraw();
	}
	
	renderInterface->removeAllInstances();
	delete renderInterface;
	renderInterface = nullptr;
	
	dynamicsWorld->setDebugDrawer(nullptr);
	delete debugDrawer;
	debugDrawer = nullptr;
	
	if (dynamicsWorld != nullptr)
	{
		for (int i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
		{
			dynamicsWorld->removeConstraint(dynamicsWorld->getConstraint(i));
		}

		for (int i = dynamicsWorld->getNumMultiBodyConstraints() - 1; i >= 0; i--)
		{
			btMultiBodyConstraint * mbc = dynamicsWorld->getMultiBodyConstraint(i);
			
			dynamicsWorld->removeMultiBodyConstraint(mbc);
			
			delete mbc;
		}

		for (int i = dynamicsWorld->getNumMultibodies() - 1; i >= 0; i--)
		{
			btMultiBody * mb = dynamicsWorld->getMultiBody(i);
			
			dynamicsWorld->removeMultiBody(mb);
			
			delete mb;
		}
		
		for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
		{
			btCollisionObject * obj = dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody * body = btRigidBody::upcast(obj);
			
			if (body != nullptr && body->getMotionState())
			{
				delete body->getMotionState();
			}
			
			dynamicsWorld->removeCollisionObject(obj);
			
			delete obj;
		}
	}
	
	for (auto * shape : shapes)
		delete shape;
	shapes.clear();
	
	delete dynamicsWorld;
	dynamicsWorld = nullptr;

	delete solver;
	solver = nullptr;

	delete broadphase;
	broadphase = nullptr;

	delete dispatcher;
	dispatcher = nullptr;

	delete pairCache;
	pairCache = nullptr;

	delete filterCallback;
	filterCallback = nullptr;

	delete collisionConfiguration;
	collisionConfiguration = nullptr;
	
	framework.shutdown();
	
	return 0;
}
