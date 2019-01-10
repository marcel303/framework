#include "etherdream.h"
#include "framework.h"
#include "imgui-framework.h"
#include <algorithm>
#include <map>

// etherdream DACs

struct DacInfo
{
	int connectTimer = 0;
	
	bool connected = false;
};

std::map<int, DacInfo> s_dacInfos;

// masking support. merges many masking regions with different begin/end points into a sorted array of non-overlapping regions,
// ready to be applied to a laser scanning line

struct MaskSegment
{
	float begin;
	float end;
};

struct Mask
{
	static const int kMaxSegments = 200;
	
	float begin[kMaxSegments];
	float end[kMaxSegments];
	int numSegments = 0;
	
	MaskSegment mergedSegments[kMaxSegments];
	int numMergedSegments = 0;
	
	void addSegment(const float in_begin, const float in_end)
	{
		if (numSegments < kMaxSegments)
		{
			begin[numSegments] = in_begin;
			end[numSegments] = in_end;
			numSegments++;
		}
		else
		{
			logDebug("the number of segments exceeds the maximum number of segments!");
		}
	}
	
	void finalize()
	{
		// sort begin and end positions of segments
		
		std::sort(begin, begin + numSegments);
		std::sort(end, end + numSegments);
		
		// iterate over begin and end points, keeping track of the number of overlapping segments. whenever there's
		// a transition from zero to one, it means a new segment begins. likewise, if there's and transition from one
		// to zero, it means a segment ends
		
		int overlap = 0;
		
		int begin_index = 0;
		int end_index = 0;
		
		float segment_begin;
		
		while (end_index != numSegments)
		{
			while (begin_index != numSegments && begin[begin_index] <= end[end_index])
			{
				overlap++;
				
				if (overlap == 1)
				{
					// this is an actual begin!
					
					segment_begin = begin[begin_index];
				}
				
				begin_index++;
			}
			
			while (end_index != numSegments && (begin_index == numSegments || (end[end_index] < begin[begin_index])))
			{
				overlap--;
				
				if (overlap == 0)
				{
					// this is an actual end!
					
					const float segment_end = end[end_index];
					
					Assert(numMergedSegments < kMaxSegments);
					if (numMergedSegments < kMaxSegments)
					{
						mergedSegments[numMergedSegments].begin = segment_begin;
						mergedSegments[numMergedSegments].end = segment_end;
						numMergedSegments++;
					}
					else
					{
						logDebug("the number of merged segments exceeds the maximum number of segments!");
					}
				}
				
				end_index++;
				
				Assert(end_index <= begin_index);
			}
		}
		
		Assert(begin_index == numSegments);
		Assert(end_index == numSegments);
	}
};

// basic particle system used to test masking support

struct PurpleRain
{
	static const int kMaxRainDrops = 100;
	
	struct RainDrop
	{
		float position;
		float life;
		float lifeRcp;
	};
	
	RainDrop rainDrops[kMaxRainDrops];
	int nextRainDropIndex;
	
	PurpleRain()
	{
		memset(this, 0, sizeof(*this));
	}

	void addRainDrop(const float position, const float life)
	{
		auto & rainDrop = rainDrops[nextRainDropIndex];
		
		nextRainDropIndex++;
		if (nextRainDropIndex == kMaxRainDrops)
			nextRainDropIndex = 0;
		
		rainDrop.position = position;
		rainDrop.life = 1.f;
		rainDrop.lifeRcp = 1.f / life;
	}
	
	void tick(const float dt)
	{
		for (auto & rainDrop : rainDrops)
		{
			if (rainDrop.life > 0.f)
			{
				rainDrop.life = fmaxf(0.f, rainDrop.life - rainDrop.lifeRcp * dt);
			}
		}
	}
};

// line sent to laser DAC

struct Line
{
	static const int kNumPointsPerLaser = 500;
	static const int kNumLasers = 4;

	static const int kNumPoints = kNumPointsPerLaser * kNumLasers;

	static float initialLineXs[kNumPoints];

	struct Point
	{
		float x;
		float y;
	};
	
	Point points[kNumPoints];
	
	void init()
	{
		for (int i = 0; i < kNumPoints; ++i)
		{
			auto & point = points[i];
			
			point.x = initialLineXs[i];
			point.y = 0.f;
		}
	}
	
	void add(const Line & line)
	{
		for (int i = 0; i < kNumPoints; ++i)
		{
		// fixme : actually add
			points[i].x = line.points[i].x;// - initialLineXs[i];
			points[i].y = line.points[i].y;
		}
	}
};

float Line::initialLineXs[kNumPoints];

// physically simulated line with a templated down scaling factor for CPU-usage reduction

template <int DownScale>
struct PhysicalString1D
{
	static const int kNumPoints = Line::kNumPoints / DownScale;
	
	double positions[kNumPoints];
	double velocities[kNumPoints];
	double forces[kNumPoints];
	
	double mass = 1.0;
	double tension = 1.0;
	
	void init(const double in_mass, const double in_tension)
	{
		memset(positions, 0, sizeof(positions));
		memset(velocities, 0, sizeof(velocities));
		memset(forces, 0, sizeof(forces));
		
		mass = in_mass;
		tension = in_tension;
	}
	
	float getX(const int index) const
	{
		return Line::initialLineXs[index * DownScale];
	}
	
	void tick(const float dt)
	{
		const double retain = pow(0.5, dt);
		
		for (int i = 0; i < kNumPoints; ++i)
		{
			forces[i] /= mass;
			
			const double deltaPrev = positions[i - 1 >=            0 ? i - 1 : i];
			const double deltaCurr = positions[i                                ];
			const double deltaNext = positions[i + 1 <= kNumPoints-1 ? i + 1 : i];
			
			const double N = 2.02;
			
			const double delta = (deltaCurr * N - deltaPrev - deltaNext) / N;
			
			const double force = - delta * tension;
			
			forces[i] += force;
		}
		
		forces[0] = 0.0;
		forces[kNumPoints - 1] = 0.0;

		for (int i = 0; i < kNumPoints; ++i)
		{
			velocities[i] += forces[i] * dt;
			
			positions[i] += velocities[i] * dt;
			
			velocities[i] *= retain;
			
			positions[i] *= retain;
		}
		
		memset(forces, 0, sizeof(forces));
	}
	
	void toLine(Line & line)
	{
		for (int i = 0; i < Line::kNumPoints; ++i)
		{
			line.points[i].x = Line::initialLineXs[i];
			line.points[i].y = positions[i / DownScale];
		}
	}
};

template <int DownScale>
struct PhysicalString2D
{
	static const int kNumPoints = Line::kNumPoints / DownScale;
	
	double positions[kNumPoints][2];
	double velocities[kNumPoints][2];
	double forces[kNumPoints][2];
	
	double mass = 1.0;
	double tension = 1.0;
	
	void init()
	{
		memset(positions, 0, sizeof(positions));
		memset(velocities, 0, sizeof(velocities));
		memset(forces, 0, sizeof(forces));
		
		for (int i = 0; i < kNumPoints; ++i)
		{
			positions[i][0] = Line::initialLineXs[i * DownScale];
		}
	}
	
	void tick(const float dt)
	{
		const double retain = pow(0.5, dt);
		
		for (int i = 0; i < kNumPoints; ++i)
		{
			forces[i][0] /= mass;
			forces[i][1] /= mass;
			
			for (int d = 0; d < 2; ++d)
			{
				const double deltaPrev = positions[d][i - 1 >=            0 ? i - 1 : i][d];
				const double deltaCurr = positions[d][i                                ][d];
				const double deltaNext = positions[d][i + 1 <= kNumPoints-1 ? i + 1 : i][d];
				
				const double N = 2.0;
				
				const double delta = (deltaCurr * N - deltaPrev - deltaNext) / N;
				
				const double force = - delta * tension;
				
				forces[i][d] += force;
		
				forces[0][d] = 0.0;
				forces[kNumPoints - 1][d] = 0.0;
				
				velocities[i][d] += forces[i][d] * dt;
				
				positions[i][d] += velocities[i][d] * dt;
				
				velocities[i][d] *= retain;
				
				if (d == 0)
					positions[i][d] = lerp((double)Line::initialLineXs[i * DownScale], positions[i][d], retain);
				else
					positions[i][d] *= retain;
			}
		}
		
		memset(forces, 0, sizeof(forces));
	}
	
	void toLine(Line & line)
	{
		for (int i = 0; i < Line::kNumPoints; ++i)
		{
			line.points[i].x = positions[i / DownScale][0];
			line.points[i].y = positions[i / DownScale][1];
		}
	}
};

struct GraviticSource
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	
	float force = 1.f;
	float minimumDistance = 1e-3f;
	
	Vec2 calculateForce(const float in_x, const float in_y) const
	{
		const float dx = x - in_x;
		const float dy = y - in_y;
		const float dz = z;
		const float minimumDistanceSq = minimumDistance * minimumDistance;
		const float distanceSq = fmaxf(minimumDistanceSq, dx * dx + dy * dy + dz * dz);
		const float distance = sqrtf(distanceSq);
		
		const float falloff = 1.f / distanceSq;
		
		const float magnitude = force * falloff;
		
		return Vec2(
			dx / distance * magnitude,
			dy / distance * magnitude);
	}
};

int main(int argc, char * argv[])
{
	if (!framework.init(800, 600))
		return -1;
	
	const bool isEtherdreamStarted = etherdream_lib_start() == 0;
	
	for (int i = 0; i < Line::kNumPoints; ++i)
		Line::initialLineXs[i] = i / float(Line::kNumPointsPerLaser);
	
	PhysicalString1D<1> string;
	string.init(1.0, 100.0);
	
	GraviticSource gravitic;
	
	PurpleRain rain;
	
	const float dropInterval = .01f;
	float dropTimer = 0.f;
	
	while (!framework.quitRequested)
	{
		SDL_Delay(10);
		
		framework.process();
		
		const float dt = keyboard.isDown(SDLK_SPACE) ? 0.f : framework.timeStep;
		
		dropTimer += dt;
		
		while (dropTimer >= dropInterval)
		{
			dropTimer -= dropInterval;
			
			rain.addRainDrop(random(0.f, 1.f), 6.f);
		}
		
		rain.tick(dt);
		
		//
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			const int offset = rand() % string.kNumPoints;
			
			const int radius = 200;
			
			for (int i = -radius; i <= +radius; ++i)
			{
				const int position = offset + i - radius;;
				
				if (position >= 0 && position < string.kNumPoints)
				{
					const float t = i / float(radius);
					
					float value = powf(cosf(t / 2.f * float(M_PI)), 2.f);
					
					const float sign = (rand() % 2) ? -1.f : +1.f;
					
					value *= powf(random(0.f, +1.f), 2.f) * sign;
					
					string.positions[position] += value * .2f;
				}
			}
		}
		
		//
		
		Line line;
		line.init();
		
		gravitic.x = mouse.x / 200.f;
		gravitic.y = (mouse.y - 200.f) / 200.f;
		gravitic.z = .2f;
		gravitic.force = 4.f;
		//gravitic.force = -10.f;
		gravitic.minimumDistance = .04f;
		
		//gravitic.force = 200.f;
		//gravitic.minimumDistance = .5f;
		
		for (int i = 0; i < string.kNumPoints; ++i)
		{
			//const float x = line.points[i * Line::kNumPoints / string.kNumPoints].x;
			const float x = string.getX(i);
			const float y = string.positions[i];
		
			const Vec2 force = gravitic.calculateForce(x, y);
			//const Vec2 force = gravitic.calculateForce(x, 0.f);
			
			string.forces[i] += force[1];
		}
		
		for (int i = 0; i < 10; ++i)
			string.tick(dt / 10.0);
		
		Line stringLine;
		string.toLine(stringLine);
		line.add(stringLine);
		
		if (isEtherdreamStarted)
		{
			const int numDACs = etherdream_dac_count();
			//logDebug("num DACs: %d", numDACs);
			
			for (int i = 0; i < numDACs; ++i)
			{
				etherdream * e = etherdream_get(i);
				
				DacInfo & dacInfo = s_dacInfos[i];
				
				if (dacInfo.connected == false)
				{
					if (dacInfo.connectTimer == 0)
					{
						dacInfo.connectTimer = 1000;
						
						if (etherdream_connect(e) == 0)
						{
							dacInfo.connected = true;
						}
					}
				}
				
				if (etherdream_is_ready(e))
				{
					//logDebug("DAC is ready. index: %d", i);
					
					const int pps = 30000;
					const int repeatCount = 1;
					
					const int numPoints = line.kNumPointsPerLaser; // 30000/60 = 500 (60fps)
					
					etherdream_point pts[numPoints];
					
					/*
					struct etherdream_point {
						int16_t x;
						int16_t y;
						uint16_t r;
						uint16_t g;
						uint16_t b;
						uint16_t i;
						uint16_t u1;
						uint16_t u2;
					};
					*/
					
					const float intensity = 1.f;
					
					for (int i = 0; i < numPoints; ++i)
					{
						auto & p_src = line.points[i];
						auto & p_dst = pts[i];
						
						p_dst.x = (int)roundf(p_src.x * 16000.f);
						p_dst.y = (int)roundf(p_src.y * 16000.f);
						
						p_dst.r = (intensity * ((1 << 16) - 1));
						p_dst.g = (intensity * ((1 << 16) - 1));
						p_dst.b = (intensity * ((1 << 16) - 1));
						p_dst.i = uint16_t(-1);
						p_dst.u1 = 0; // unused
						p_dst.u2 = 0;
					}
					
					const int result = etherdream_write(
						e,
						pts,
                     	500,
                     	pps,
                     	repeatCount);
					
					if (result != 0)
					{
						logDebug("etherdream_write failed with code %d", result);
					}
				}
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxPushMatrix();
			{
				gxTranslatef(0, 200, 0);
				gxScalef(200, 200, 1);
				
				setColor(colorWhite);
				gxBegin(GL_LINES);
				{
					for (int i = 0; i < Line::kNumPoints - 1; ++i)
					{
						auto & point1 = line.points[i + 0];
						auto & point2 = line.points[i + 1];
						
						gxVertex2f(point1.x, point1.y);
						gxVertex2f(point2.x, point2.y);
					}
				}
				gxEnd();
				
				drawCircle(gravitic.x, gravitic.y, .1f, 10);
			}
			gxPopMatrix();
			
			pushBlend(BLEND_ADD);
			{
				Mask mask;
				
				setColor(200, 150, 100);
				
				for (auto & rainDrop : rain.rainDrops)
				{
					if (rainDrop.life > 0.f)
					{
						const float size = .02f * rainDrop.life;
						
						const float begin = rainDrop.position - size/2.f;
						const float end = rainDrop.position + size/2.f;
						
						mask.addSegment(begin, end);
						
						drawLine(begin * 400.f, 500.f, end * 400.f, 500.f);
					}
				}
				
				mask.finalize();
				
				gxBegin(GL_LINES);
				{
					for (int i = 0; i < mask.numMergedSegments; ++i)
					{
						auto & segment = mask.mergedSegments[i];
						gxVertex2f(segment.begin * 400.f, 400.f);
						gxVertex2f(segment.end * 400.f, 400.f);
					}
				}
				gxEnd();
			}
			popBlend();
		}
		framework.endDraw();
	}
	
	return 0;
}
