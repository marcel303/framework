#include "dataTypes.h"
#include "framework.h"
#include "imgui-framework.h"
#include "Noise.h"

/*
Coding Challenge #124: Flocking Simulation
https://www.youtube.com/watch?v=mhjuuHl6qHM
*/

static float kSteeringSpeed = 100.f;
static float kSeparationForce = 100000.f;
static float kMaxForce = 100.f;

static Vec3 randomVec3()
{
	// make a point on a sphere using the fibonacci sphere formula
	
	Vec3 point;
	
	const int numPoints = 1000 * 1000;
	
    const float rnd = 1.f;
	
    const float offset = 2.f / numPoints;
    const float increment = M_PI * (3.f - sqrtf(5.f));
	
	//for (int i = 0; i < numPoints; ++i)
    {
		const int i = rand() % numPoints;
		
        const float y = ((i * offset) - 1) + (offset / 2.f);
        const float r = sqrtf(1.f - y * y);

        const float phi = fmodf((i + rnd), numPoints) * increment;

        const float x = cosf(phi) * r;
        const float z = sinf(phi) * r;
		
        point.Set(x, y, z);
	}
	
	return point;
}

static void limitVec3(Vec3 & vec, const float maxMagnitude)
{
	const float mag = vec.CalcSize();
	
	if (mag > maxMagnitude)
	{
		vec = vec / mag * maxMagnitude;
	}
}


struct Boid
{
	Vec3 position;
	Vec3 velocity;
	Vec3 acceleration;
	
	void tick(const float dt)
	{
		velocity += acceleration * dt;
		position += velocity * dt;
		
		//
		
		acceleration.SetZero();
	}
};

typedef std::vector<Boid> BoidsArray;

static void alignment(Boid & boid, const BoidsArray & other_boids, const float dt)
{
	const float perception = 60.f;
	
	Vec3 average_velocity;
	int num_boids = 0;
	
	for (auto & other_boid : other_boids)
	{
		if (&other_boid == &boid)
			continue;
		
		const Vec3 delta = boid.position - other_boid.position;
		const float distance = delta.CalcSize();
		
		if (distance <= perception)
		{
			average_velocity += other_boid.velocity;
			num_boids++;
		}
	}
	
	if (num_boids > 0)
	{
		average_velocity /= num_boids;
		
		const Vec3 desired_velocity = average_velocity.CalcNormalized() * kSteeringSpeed;
		
		const Vec3 velocity_delta = desired_velocity - boid.velocity;
		
		const Vec3 acceleration = velocity_delta;
		
		boid.acceleration += acceleration;
	}
}

static void separation(Boid & boid, const BoidsArray & other_boids, const float dt)
{
	const float perception = 80.f;
	
	Vec3 acceleration;
	
	for (auto & other_boid : other_boids)
	{
		if (&other_boid == &boid)
			continue;
		
		const Vec3 delta = boid.position - other_boid.position;
		const float distance = delta.CalcSize();
		
		if (distance <= perception)
		{
			const Vec3 direction = delta.CalcNormalized();
			
			Vec3 local_acceleration = direction / distance * kSeparationForce;
			
			limitVec3(local_acceleration, kMaxForce);
			
			acceleration += local_acceleration;
		}
	}
	
	boid.acceleration += acceleration;
}

static void cohesion(Boid & boid, const BoidsArray & other_boids, const float dt)
{
	const float perception = 80.f;
	
	Vec3 average_position;
	int num_boids = 0;
	
	for (auto & other_boid : other_boids)
	{
		if (&other_boid == &boid)
			continue;
		
		const Vec3 delta = boid.position - other_boid.position;
		const float distance = delta.CalcSize();
		
		if (distance <= perception)
		{
			average_position += other_boid.position;
			num_boids++;
		}
	}
	
	if (num_boids > 0)
	{
		average_position /= num_boids;
		
		const Vec3 position_delta = average_position - boid.position;
		
		const Vec3 acceleration = position_delta.CalcNormalized() * kMaxForce;
		
		boid.acceleration += acceleration;
	}
}

static bool do_alignment = true;
static bool do_separation = true;
static bool do_cohesion = true;

static void flock(Boid & boid, const BoidsArray & other_boids, const float dt)
{
	if (do_alignment)
		alignment(boid, other_boids, dt);
	
	if (do_separation)
		separation(boid, other_boids, dt);
	
	if (do_cohesion)
		cohesion(boid, other_boids, dt);
	
	boid.acceleration[0] += 1000.f * octave_noise_4d(4, .5f, .002f, boid.position[0], boid.position[1], boid.position[2], framework.time);
	boid.acceleration[1] +=  400.f * octave_noise_4d(4, .5f, .002f, boid.position[0], boid.position[1], boid.position[2], framework.time / 1.234f);
	boid.acceleration[2] += 1000.f * octave_noise_4d(4, .5f, .002f, boid.position[0], boid.position[1], boid.position[2], framework.time / 2.345f);
	boid.acceleration -= boid.position * .2f;

	boid.velocity *= powf(.8f, dt);
}

static void edges(Boid & boid, const float worldSize)
{
	// wrap around
	
#if 0
	boid.position = boid.position.Max(Vec3(-worldSize)).Min(Vec3(+worldSize));
#elif 0
	if (boid.position[0] < -worldSize) boid.position[0] += worldSize * 2.f;
	if (boid.position[1] < -worldSize) boid.position[1] += worldSize * 2.f;
	if (boid.position[2] < -worldSize) boid.position[2] += worldSize * 2.f;
	
	if (boid.position[0] > +worldSize) boid.position[0] -= worldSize * 2.f;
	if (boid.position[1] > +worldSize) boid.position[1] -= worldSize * 2.f;
	if (boid.position[2] > +worldSize) boid.position[2] -= worldSize * 2.f;
#endif
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.vrMode = true;
	framework.enableVrMovement = true;
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 800))
		return -1;

	Window * guiWindow = new Window("Gui", 600, 300);
	
	BoidsArray boids;
	boids.resize(200);
	
	const float worldSize = 300.f;
	
	for (auto & boid : boids)
	{
		if (true)
		{
			boid.position.Set(
				random(-worldSize, +worldSize),
				random(-worldSize, +worldSize),
				random(-worldSize, +worldSize));
			boid.velocity = randomVec3() * random(20.f, 40.f);
		}
		else
		{
			boid.velocity = randomVec3() * 100.f;
		}
	}
	
	FrameworkImGuiContext guiCtx;
	guiCtx.init();
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		framework.tickVirtualDesktop(
			vrPointer[0].getTransform(framework.vrOrigin),
			vrPointer[0].hasTransform,
			(vrPointer[0].isDown(VrButton_Trigger) << 0) |
			(vrPointer[0].isDown(VrButton_GripTrigger) << 1),
			false);
			
		const float dt = framework.timeStep;
		
		for (auto & boid : boids)
		{
			flock(boid, boids, dt);
			
			boid.tick(dt);
			
			edges(boid, worldSize);
		}
		
		pushWindow(*guiWindow);
		{
			bool inputIsCaptured = false;
			guiCtx.processBegin(dt, 800, 800, inputIsCaptured);
			{
				if (ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Checkbox("Alignment", &do_alignment);
					ImGui::Checkbox("Separation", &do_separation);
					ImGui::Checkbox("Cohesion", &do_cohesion);
					ImGui::SliderFloat("Max Steering", &kSteeringSpeed, 0.f, 400.f);
					ImGui::SliderFloat("Max Force", &kMaxForce, 0.f, 400.f);
					ImGui::SliderFloat("Separation Force", &kSeparationForce, 0.f, 1000000.f, "%.2f", ImGuiSliderFlags_Logarithmic);
				}
				ImGui::End();
			}
			guiCtx.processEnd();
			
			framework.beginDraw(0, 0, 0, 0);
			{
				guiCtx.draw();
			}
			framework.endDraw();
		}
		popWindow();
		
		for (int i = 0; i < framework.getEyeCount(); ++i)
		{
			framework.beginEye(i, colorBlack);
			{
				framework.drawVirtualDesktop();
				
				pushWireframe(true);
				gxBegin(GX_TRIANGLES);
				{
					for (auto & boid : boids)
					{
						auto x = boid.position[0];
						auto y = boid.position[1];
						auto z = boid.position[2];
						
						auto forward = boid.velocity.CalcNormalized();
						
						auto tipx = x + forward[0] * 12.f;
						auto tipz = z + forward[2] * 12.f;
						
						Vec3 side(forward[2], 0.f, -forward[0]);
						auto sidex1 = x + side[0] * 4.f;
						auto sidez1 = z + side[2] * 4.f;
						auto sidex2 = x - side[0] * 4.f;
						auto sidez2 = z - side[2] * 4.f;
						
						setColor(colorWhite);
						
						gxVertex3f(sidex1, y, sidez1);
						gxVertex3f(sidex2, y, sidez2);
						gxVertex3f(tipx, y, tipz);
					}
				}
				gxEnd();
				popWireframe();
			}
			framework.endEye();
		}
		
		framework.present();
	}
	
	guiCtx.shut();
	
	delete guiWindow;
	guiWindow = nullptr;
	
	framework.shutdown();
	
	return 0;
}
