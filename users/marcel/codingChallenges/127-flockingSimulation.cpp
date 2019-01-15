#include "dataTypes.h"
#include "framework.h"
#include "imgui-framework.h"

/*
Coding Challenge #124: Flocking Simulation
https://www.youtube.com/watch?v=mhjuuHl6qHM
*/

static float kSteeringSpeed = 100.f;
static float kSeparationForce = 100000.f;
static float kMaxForce = 100.f;

static Vec2 randomVec2()
{
	const float angle = random(0.f, float(M_PI) * 2.f);
	const float x = cosf(angle);
	const float y = sinf(angle);
	
	return Vec2(x, y);
}

static void limitVec2(Vec2 & vec, const float maxMagnitude)
{
	const float mag = vec.CalcSize();
	
	if (mag > maxMagnitude)
	{
		vec = vec / mag * maxMagnitude;
	}
}


struct Boid
{
	Vec2 position;
	Vec2 velocity;
	Vec2 acceleration;
	
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
	
	Vec2 average_velocity;
	int num_boids = 0;
	
	for (auto & other_boid : other_boids)
	{
		if (&other_boid == &boid)
			continue;
		
		const Vec2 delta = boid.position - other_boid.position;
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
		
		const Vec2 desired_velocity = average_velocity.CalcNormalized() * kSteeringSpeed;
		
		Vec2 velocity_delta = desired_velocity - boid.velocity;
		
		const Vec2 acceleration = velocity_delta;
		
		boid.acceleration += acceleration;
	}
}

static void separation(Boid & boid, const BoidsArray & other_boids, const float dt)
{
	const float perception = 80.f;
	
	Vec2 acceleration;
	
	for (auto & other_boid : other_boids)
	{
		if (&other_boid == &boid)
			continue;
		
		const Vec2 delta = boid.position - other_boid.position;
		const float distance = delta.CalcSize();
		
		if (distance <= perception)
		{
			const Vec2 direction = delta.CalcNormalized();
			
			Vec2 local_acceleration = direction / distance * kSeparationForce;
			
			limitVec2(local_acceleration, kMaxForce);
			
			acceleration += local_acceleration;
		}
	}
	
	boid.acceleration += acceleration;
}

static void cohesion(Boid & boid, const BoidsArray & other_boids, const float dt)
{
	const float perception = 80.f;
	
	Vec2 average_position;
	int num_boids = 0;
	
	for (auto & other_boid : other_boids)
	{
		if (&other_boid == &boid)
			continue;
		
		const Vec2 delta = boid.position - other_boid.position;
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
		
		const Vec2 position_delta = average_position - boid.position;
		
		const Vec2 acceleration = position_delta.CalcNormalized() * kMaxForce;
		
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
}

static void edges(Boid & boid)
{
#if 0
	// constrain
	
	boid.position[0] = clamp(boid.position[0], -400.f, +400.f);
	boid.position[1] = clamp(boid.position[1], -400.f, +400.f);
#else
	// wrap around
	
	if (boid.position[0] < -400.f)
		boid.position[0] = +400.f;
	else if (boid.position[0] > +400.f)
		boid.position[0] = -400.f;
	
	if (boid.position[1] < -400.f)
		boid.position[1] = +400.f;
	else if (boid.position[1] > +400.f)
		boid.position[1] = -400.f;
#endif
}

int main(int argc, char * argv[])
{
	if (!framework.init(800, 800))
		return -1;

	BoidsArray boids;
	boids.resize(200);
	
	for (auto & boid : boids)
	{
		if (true)
		{
			boid.position.Set(random(-400.f, +400.f), random(-400.f, +400.f));
			boid.velocity = randomVec2() * random(20.f, 40.f);
		}
		else
		{
			boid.velocity = randomVec2() * 100.f;
		}
	}
	
	FrameworkImGuiContext guiCtx;
	guiCtx.init();
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		const float dt = framework.timeStep;
		
		for (auto & boid : boids)
		{
			flock(boid, boids, dt);
			
			boid.tick(dt);
			
			edges(boid);
		}
		
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
				ImGui::SliderFloat("Separation Force", &kSeparationForce, 0.f, 1000000.f, "%.2f", 4.f);
			}
			ImGui::End();
		}
		guiCtx.processEnd();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxPushMatrix();
			gxTranslatef(400, 400, 0);
			
			hqBegin(HQ_STROKED_TRIANGLES);
			{
				for (auto & boid : boids)
				{
					auto x = boid.position[0];
					auto y = boid.position[1];
					
					auto forward = boid.velocity.CalcNormalized();
					
					auto tipx = x + forward[0] * 12.f;
					auto tipy = y + forward[1] * 12.f;
					
					Vec2 side(forward[1], -forward[0]);
					auto sidex1 = x + side[0] * 4.f;
					auto sidey1 = y + side[1] * 4.f;
					auto sidex2 = x - side[0] * 4.f;
					auto sidey2 = y - side[1] * 4.f;
					
					setColor(colorWhite);
					hqStrokeTriangle(
						sidex1,
						sidey1,
						sidex2,
						sidey2,
						tipx,
						tipy,
						2.f);
				}
			}
			hqEnd();
			
			gxPopMatrix();
			
			guiCtx.draw();
		}
		framework.endDraw();
	}
	
	guiCtx.shut();
	
	return 0;
}
