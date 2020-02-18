#include "CollisionScene.h"
#include "CollisionSphere.h"
#include "framework.h"

static int g_allocCount = 0;

int main()
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	CollisionScene scene;

	scene.Initialize();

	const float size = 125.0f;
	const int sphereCount = 4000;

	CollisionSphere * spheres = new CollisionSphere[sphereCount];

	for (int i = 0; i < sphereCount; ++i)
	{
		const float x = random(-size, +size);
		const float y = random(-size, +size * 200.0f);
		const float z = 0.0f;

		const float scale = random(0.5f, 1.5f);
		const float radius = 4.0f * scale;

		//logDebug("adding sphere: %+03.3f %+03.3f %+03.3f, %+03.3f", x, y, z, radius);

		CollisionSphere * sphere = &spheres[i];

		sphere->m_position[0] = x;
		sphere->m_position[1] = y;
		sphere->m_position[2] = z;
		sphere->m_radius = radius;

		scene.Add(sphere);
	}

	spheres[0].m_radius = 40.0f;

	logInfo("done initializing scene");

	framework.enableDepthBuffer = true;
	framework.init(640, 480);

	Vec3 cameraPosition(0.0f, -25.0f, 100.0f);
	Vec3 cameraTarget(0.0f, -100.0f, 0.0f);

	float time = 0.0f;

	bool enableWhirpool = false;
	bool enableBoxConstraint = false;
	bool enableDeactivations = true;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;

		const float deltaTime = fminf(1.f / 30.f, framework.timeStep);

		time += deltaTime;

		// handle input

		if (keyboard.wentDown(SDLK_b))
			enableBoxConstraint = !enableBoxConstraint;
		if (keyboard.wentDown(SDLK_w))
			enableWhirpool = !enableWhirpool;
		if (keyboard.wentDown(SDLK_d))
			enableDeactivations = !enableDeactivations;
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			cameraPosition.Set(random(-size, +size), -25.0f, random(-size, +size));
		}

		float speedX = 0.0f;
		float speedZ = 0.0f;

		if (keyboard.isDown(SDLK_UP))
			speedZ -= 1.0f;
		if (keyboard.isDown(SDLK_DOWN))
			speedZ += 1.0f;
		if (keyboard.isDown(SDLK_LEFT))
			speedX -= 1.0f;
		if (keyboard.isDown(SDLK_RIGHT))
			speedX += 1.0f;

		cameraPosition[0] += 100.0f * speedX * deltaTime;
		cameraPosition[2] += 100.0f * speedZ * deltaTime;

		// Update movement.
		
		float whirlpoolPosition[3];
		whirlpoolPosition[0] = sinf(time * 0.1f) * size * 2.0f / 3.0f;
		whirlpoolPosition[1] = 0.0f;
		whirlpoolPosition[2] = cosf(time * 0.1f) * size * 2.0f / 3.0f;
		
		const float falloff = (float)pow(0.28, deltaTime);
		
		for (size_t j = 0; j < scene.m_spheres.size(); ++j)
		{
			CollisionSphere * sphere = scene.m_spheres[j];

			if (sphere->m_inactive == false)
			{
				scene.MoveBegin(sphere);

				const int MAX_SPHERES = 100;

				CollisionSphere * spheres[MAX_SPHERES];
				int sphereCount;

				scene.m_hashSpace.GetItems(
					sphere->m_hashes,
					spheres,
					MAX_SPHERES,
					sphereCount);

				//printf("Query returned %d spheres.\n", spheres.size());

				for (size_t i = 0; i < sphereCount; ++i)
				{
					CollisionSphere * other = spheres[i];

					if (other == sphere)
						Assert(false); // always false, because we remove the sphere from the scene using MoveBegin
					
					IntersectionInfo intersection;

					const bool intersect = sphere->Intersect(*other, intersection);

					if (intersect)
					{
					#if 1
						// use forces to slowly resolve collisions
						
						//printf("Intersect.\n");

						float force = sphere->m_mass * intersection.m_distance;

						//force *= 0.25f;
						force *= 0.5f;

						sphere->AddForce(
							intersection.m_normal[0] * force,
							intersection.m_normal[1] * force,
							intersection.m_normal[2] * force);

						// TODO: Directly calculate impact on speed instead of force + correct position?

						force *= -1.0f;

						other->AddForce(
							intersection.m_normal[0] * force,
							intersection.m_normal[1] * force,
							intersection.m_normal[2] * force);
					#else
						// offset sphere locations to resolve collisions
						
						sphere->m_position[0] += intersection.m_normal[0] * intersection.m_distance * 0.5f;
						sphere->m_position[1] += intersection.m_normal[1] * intersection.m_distance * 0.5f;
						sphere->m_position[2] += intersection.m_normal[2] * intersection.m_distance * 0.5f;

						other->m_position[0] -= intersection.m_normal[0] * intersection.m_distance * 0.5f;
						other->m_position[1] -= intersection.m_normal[1] * intersection.m_distance * 0.5f;
						other->m_position[2] -= intersection.m_normal[2] * intersection.m_distance * 0.5f;
					#endif
					}
				}

				//printf("Old sphere position: %f %f %f.\n", sphere->m_position[0], sphere->m_position[1], sphere->m_position[2]);

				sphere->m_position[0] += sphere->m_speed[0] * deltaTime;
				sphere->m_position[1] += sphere->m_speed[1] * deltaTime;
				sphere->m_position[2] += sphere->m_speed[2] * deltaTime;

				if (enableBoxConstraint)
				{
					if (sphere->m_position[0] < -30.0f)
						sphere->m_position[0] = -30.0f;
					if (sphere->m_position[0] > +30.0f)
						sphere->m_position[0] = +30.0f;
					if (sphere->m_position[2] < -30.0f)
						sphere->m_position[2] = -30.0f;
					if (sphere->m_position[2] > +30.0f)
						sphere->m_position[2] = +30.0f;
				}

				//printf("New sphere position: %f %f %f.\n", sphere->m_position[0], sphere->m_position[1], sphere->m_position[2]);

				scene.MoveEnd(sphere);
			}

			// Intersect with ground.
			if (sphere->m_position[1] - sphere->m_radius < -100.0f)
			{
				sphere->m_position[1] = -100.0f + sphere->m_radius;
				sphere->m_speed[1] *= -0.98f;
			}

			sphere->AddForce(0.0f, -5.0f, 0.0f);
			//sphere->AddForce(0.0f, -1.0f, 0.0f);
			//sphere->AddForce(0.0f, -0.5f, 0.0f);
			//sphere->AddForce(0.0f, -0.1f, 0.0f);

			if (enableWhirpool)
			{
				// Whirlpool thingy.

				const float dx = sphere->m_position[0] - whirlpoolPosition[0];
				const float dz = sphere->m_position[2] - whirlpoolPosition[2];

				const float scale = 0.03f;

				const float strength = sinf(time * 0.2f) * 2.f * scale;

				sphere->m_force[0] += -dz * strength - dx * scale;
				sphere->m_force[2] += +dx * strength - dz * scale;
			}

			// Add a tiny bit of jitter for stability of the physical system.
			{
				const float jitter = ((rand() % 3) - 1) * 0.001f;
				sphere->AddForce(jitter, jitter, jitter);
			}

			// Counter force.
			if (true)
			{
				const float counterForce = 0.2f;

				if (fabsf(sphere->m_force[0]) < counterForce)
					sphere->m_force[0] = 0.0f;
				if (fabsf(sphere->m_force[2]) < counterForce)
					sphere->m_force[2] = 0.0f;
			}

			sphere->m_speed[0] += sphere->m_force[0] * sphere->m_invMass;
			sphere->m_speed[1] += sphere->m_force[1] * sphere->m_invMass;
			sphere->m_speed[2] += sphere->m_force[2] * sphere->m_invMass;
			
			// Apply dampening.
			if (true)
			{
				sphere->m_speed[0] *= falloff;
				sphere->m_speed[1] *= falloff;
				sphere->m_speed[2] *= falloff;
			}

			sphere->m_force[0] = 0.0f;
			sphere->m_force[1] = 0.0f;
			sphere->m_force[2] = 0.0f;

			if (enableDeactivations)
			{
				const float speedSquared =
					sphere->m_speed[0] * sphere->m_speed[0] +
					sphere->m_speed[1] * sphere->m_speed[1] +
					sphere->m_speed[2] * sphere->m_speed[2];

				//const float speed = sqrt(speedSquared);

				const float treshold = 1.0f;
				const float tresholdSquared = treshold * treshold;

				if (speedSquared > tresholdSquared)
				{
					//if (sphere->m_inactive == true)
						//printf("Inactive -> Active.\n");
					sphere->m_inactive = false;
				}
				else
				{
					//if (sphere->m_inactive == false)
						//printf("Active -> Inactive.\n");
					sphere->m_inactive = true;
				}
			}
			else
			{
				sphere->m_inactive = false;
			}
		}

		if (true)
		{
			scene.MoveBegin(&spheres[0]);
			spheres[0].m_position[0] = sinf(time * 1.0000f) * size * 2.0f / 3.0f;
			spheres[0].m_position[1] = -100.0f + 2.0f;
			spheres[0].m_position[2] = cosf(time * 1.1234f) * size * 2.0f / 3.0f;
			scene.MoveEnd(&spheres[0]);
		}

		//printf("Update complete.\n");
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setDepthTest(true, DEPTH_LESS);
			
			Mat4x4 matV;
			matV.MakeLookat(cameraPosition, cameraTarget, Vec3(0.0f, 1.0f, 0.0f));

			projectPerspective3d(90.f, 0.05f, 10000.0f);
			gxMultMatrixf(matV.m_v);
			pushShaderOutputs("n");
			
			beginCubeBatch();
			for (size_t j = 0; j < scene.m_spheres.size(); ++j)
			{
				const CollisionSphere * sphere = scene.m_spheres[j];

				const float x = sphere->m_position[0];
				const float y = sphere->m_position[1];
				const float z = sphere->m_position[2];
				const float radius = sphere->m_radius;

				setColor(colorWhite);
				fillCube(Vec3(x, y, z), Vec3(radius, radius, radius));
			}
			endCubeBatch();
			
			popShaderOutputs();
		}
		framework.endDraw();

		//if ((rand() & 31) == 0)
			printf("AllocCount: %d.\n", g_allocCount);
	}

	printf("Done.\n");

	framework.shutdown();

	return 0;
}

void* operator new(size_t size)
{
	++g_allocCount;

	return malloc(size);
}

void* operator new[](size_t size)
{
	++g_allocCount;

	return malloc(size);
}

void operator delete(void* p) noexcept
{
	free(p);
}

void operator delete[](void* p) noexcept
{
	free(p);
}
