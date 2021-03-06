#include "calibration.h"
#include "edreamUtils.h"
#include "etherdream.h"
#include "framework.h"
#include "homography.h"
#include "imgui-framework.h"
#include "laserTypes.h"
#include "masking.h"
#include "noiseModulator.h"
#include <algorithm>
#include <map>
#include <sstream>

const int VIEW_SX = 1200;
const int VIEW_SY = 800;

static const int kFrameSize = 500; // 30000/60 = 500 (60fps)
static const int kFramePadding = 50;
static const int kNumLasers = 4;
static const int kLineSize = kFrameSize - kFramePadding*2;

// etherdream DACs

struct DacInfo
{
	int connectTimer = 0;
	
	bool connected = false;
};

std::map<int, DacInfo> s_dacInfos;

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
	struct Point
	{
		float x;
		float y;
	};
	
	std::vector<Point> points;
	
	void init(const int numPoints, const float x1, const float x2)
	{
		Assert(numPoints >= 2);
		
		points.resize(numPoints);
		
		for (int i = 0; i < numPoints; ++i)
		{
			auto & point = points[i];
			
			const float t = i / float(numPoints - 1);
			const float x = x1 * (1.f - t) + x2 * t;
			
			point.x = x;
			point.y = 0.f;
		}
	}
	
	void newFrame()
	{
		for (auto & point : points)
		{
			point.y = 0.f;
		}
	}
};

// physically simulated line with a templated down scaling factor for CPU-usage reduction

template <int kNumPoints>
struct PhysicalString1DSim
{
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
	
	int getNumPoints() const
	{
		return kNumPoints;
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
	
	void drawOntoLine(Line & line, const float x1, const float x2, const float amplitude, const float exponential)
	{
		auto begin = std::lower_bound(
			line.points.begin(),
			line.points.end(),
			x1,
			[](const Line::Point & p, const float x) -> bool { return p.x < x; });
		
		auto end = std::upper_bound(
			line.points.begin(),
			line.points.end(),
			x2,
			[](const float x, const Line::Point & p) -> bool { return x < p.x; });
		
		for (auto i = begin; i < end; ++i)
		{
			Line::Point & dst = *i;
			
			// x = x1 + (x2 - x1) * t
			// t = (x - x1) / (x2 - x1)
			
			const float t = (dst.x - x1) / (x2 - x1);
			Assert(t >= 0.f && t <= 1.f);
			
			const float src_index = t * (kNumPoints - 1);
			const int src_index1 = (int)floorf(src_index);
			const int src_index2 = src_index1 + 1;
			const float src_t = src_index - src_index1;
			
			if (src_index1 >= 0 && src_index2 < kNumPoints)
			{
				const float y1 = positions[src_index1];
				const float y2 = positions[src_index2];
				
				const float y = y1 * (1.f - src_t) + y2 * src_t;
				
				//const float curve = powf((1.f - cosf(t * float(M_PI) * 2.f)) * .5f, exponential);
				float c = (t - .5f) * 2.f;
				c = fabsf(c);
				c = 1.f - c;
				c = fminf(1.f, c * exponential);
				c = (1.f - cosf(c * float(M_PI))) * .5f;
				//c = powf(c, exponential);
				//const float curve = (1.f - cosf(powf(t, exponential) * float(M_PI) * 2.f)) * .5f;
				const float curve = c;
				
				dst.y += y * curve * amplitude;
			}
		}
	}
};

template <int kNumPoints>
struct PhysicalString1D : PhysicalString1DSim<kNumPoints>
{
	typedef PhysicalString1DSim<kNumPoints> Base;
	
	float x1 = 0.f;
	float x2 = 1.f;
	
	float amplitude = 1.f;
	
	void init(const double in_mass, const double in_tension, const float in_amplitude, const float in_x1, const float in_x2)
	{
		Base::init(in_mass, in_tension);
		
		x1 = in_x1;
		x2 = in_x2;

		amplitude = in_amplitude;
	}
	
	float getXForPointIndex(const int pointIndex) const
	{
		const float t = pointIndex / float(kNumPoints);
		
		return x1 * (1.f - t) + x2 * t;
	}
	
	void drawOntoLine(Line & line, const float exponential)
	{
		Base::drawOntoLine(line, x1, x2, amplitude, exponential);
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
		//const float falloff = 1.f / distance;
		
		const float magnitude = force * falloff;
		
		return Vec2(
			dx / distance * magnitude,
			dy / distance * magnitude);
	}
};

#include "Noise.h"

struct NoiseSweepModifier
{
	bool isActive = false;
	float time = 0.f;
	
	float x1 = 0.f;
	float x2 = 0.f;
	float noiseFrequency = 1.f;
	
	float strength = 1.f;
	float duration = 1.f;
	
	float previousTime = 0.f;
	
	void begin(const float in_x1, const float in_x2)
	{
		isActive = true;
		time = 0.f;
		previousTime = 0.f;
		
		x1 = in_x1;
		x2 = in_x2;
	}
	
	void tick(const float dt)
	{
		if (time >= duration)
			isActive = false;
		previousTime = time;
		
		time += dt;
	}
	
	static float isInside(const float x, const float in_x1, const float in_x2)
	{
		const float x1 = fminf(in_x1, in_x2);
		const float x2 = fmaxf(in_x1, in_x2);
		
		return x >= x1 && x <= x2;
	}
	
	static float distanceBetween(const float x1, const float x2)
	{
		return fabsf(x2 - x1);
	}
	
	float calculateForce(const float in_x) const
	{
		if (isActive == false)
			return 0.f;
		else
		{
			const float tPrev = saturate(previousTime / duration);
			const float tCurr = saturate(time / duration);
			
			const float xPrev = x1 * (1.f - tPrev) + x2 * tPrev;
			const float xCurr = x1 * (1.f - tCurr) + x2 * tCurr;
			
			const float dx = isInside(in_x, xPrev, xCurr)
				? 0.f
				: fminf(distanceBetween(in_x, xPrev), distanceBetween(in_x, xCurr));
			
			const float falloff = 1.f / fmaxf(dx * dx, .01f);
			
			//const float valueAtX = random<float>(-1.f, +1.f);
			//const float valueAtX = sinf(in_x * 2.f * float(M_PI) * 10.f);
			const float valueAtX = octave_noise_2d(4, .5f, 1.f, in_x * noiseFrequency, framework.time);
			
			return valueAtX * falloff * strength / sqrtf(duration);
		}
	}
};

struct LaserFrame
{
	LaserPoint points[kFrameSize];
};

struct Calibration
{
	Homography homography;
	
	void init()
	{
		homography.v00.viewPosition.Set(0.f, 0.f);
		homography.v10.viewPosition.Set(1.f, 0.f);
		homography.v01.viewPosition.Set(0.f, 1.f);
		homography.v11.viewPosition.Set(1.f, 1.f);
	}
	
	void applyTransform(LaserFrame & frame)
	{
		float m[9];
		homography.calcHomographyMatrix(m);
		homography.calcInverseMatrix(m, m);
		
		Mat4x4 m4(true);
		for (int x = 0; x < 3; ++x)
			for (int y = 0; y < 3; ++y)
				m4(x, y) = m[x + y * 3];
		
		for (int i = 0; i < kFrameSize; ++i)
		{
			auto & point = frame.points[i];
			
			auto transformed = m4.Mul3(Vec3(point.x, point.y, 1.f));
			
			Assert(transformed[2] > 0.f);
			point.x = transformed[0] / transformed[2];
			point.y = transformed[1] / transformed[2];
		}
	}
};

struct CalibrationUi
{
	Calibration * calibration = nullptr;
	
	int x = 0;
	int y = 0;
	int sx = 1;
	int sy = 1;
	
	void init(Calibration * in_calibration)
	{
		calibration = in_calibration;
	}
	
	void setEditorArea(const int in_x, const int in_y, const int in_sx, const int in_sy)
	{
		x = in_x;
		y = in_y;
		sx = in_sx;
		sy = in_sy;
	}
	
	Vec2 calculateMousePosition() const
	{
		return Vec2(
			-1.f + (mouse.x - x) / float(sx / 2.f),
			-1.f + (mouse.y - y) / float(sy / 2.f));
	}
	
	void tickEditor(const float sensitivity, const float dt, bool & inputIscaptured)
	{
		auto mousePos = calculateMousePosition();
		
		//logDebug("mouse pos: %.2f, %.2f", mousePos[0], mousePos[1]);
		
		calibration->homography.tickEditor(mousePos, sensitivity, dt, inputIscaptured);
	}
	
	void cancelEditing()
	{
		calibration->homography.cancelEditing();
	}
	
	void drawEditorPreview() const
	{
		gxPushMatrix();
		gxTranslatef(x, y, 0.f);
		gxScalef(sx / 2.f, sy / 2.f, 1.f);
		gxTranslatef(1.f, 1.f, 0.f);
		
		float m[9];
		calibration->homography.calcHomographyMatrix(m);
		
		Mat4x4 m4(true);
		for (int x = 0; x < 3; ++x)
			for (int y = 0; y < 3; ++y)
				m4(x, y) = m[x + y * 3];
		
		for (int x = 0; x < 10; ++x)
		{
			for (int y = 0; y < 10; ++y)
			{
				const float x1 = (x + 0) / 10.f;
				const float y1 = (y + 0) / 10.f;
				const float x2 = (x + 1) / 10.f;
				const float y2 = (y + 1) / 10.f;
				
				Vec3 p1 = m4.Mul3(Vec3(x1, y1, 1.f));
				Vec3 p2 = m4.Mul3(Vec3(x2, y2, 1.f));
				
				if (p1[2] > 0.f && p2[2] > 0.f)
				{
					p1 /= p1[2];
					p2 /= p2[2];
					
					setColorf(x1, y1, x2);
					drawRect(p1[0], p1[1], p2[0], p2[1]);
				}
			}
		}
		
		setColor(255, 255, 255, 127);
		gxBegin(GX_LINE_STRIP);
		pushLineSmooth(true);
		for (int i = 0; i < 1000; ++i)
		{
			const float angle = i / 1000.f * 2.f * float(M_PI);
			const float x = .5f + cosf(angle) / 3.f;
			const float y = .5f + sinf(angle) / 3.f;
			
			Vec3 p = m4.Mul3(Vec3(x, y, 1.f));
			
			p /= p[2];
			
			gxVertex2f(p[0], p[1]);
		}
		gxEnd();
		popLineSmooth();
		
		gxPopMatrix();
	}
	
	void drawEditor() const
	{
		gxPushMatrix();
		{
			gxTranslatef(x, y, 0.f);
			gxScalef(sx / 2.f, sy / 2.f, 1.f);
			gxTranslatef(1.f, 1.f, 0.f);
			
			calibration->homography.drawEditor();
		}
		gxPopMatrix();
	}
};

struct LaserController
{
	unsigned long dacId = 0;
	
	bool isDetected = false;
	
	void init(const char * in_dacId)
	{
		dacId = strtoll(in_dacId, nullptr, 16);
	}
	
	void send(const LaserFrame & frame)
	{
		DacInfo * dacInfo = nullptr;
		
		etherdream * e = nullptr;
		
		// attemp to find our DAC
		
		{
			const int numDACs = etherdream_dac_count();
			//logDebug("num DACs: %d", numDACs);
			
			for (int i = 0; i < numDACs; ++i)
			{
				etherdream * e_itr = etherdream_get(i);
				
				if (etherdream_get_id(e_itr) == dacId)
				{
					e = e_itr;
					
					dacInfo = &s_dacInfos[i];
					
					isDetected = true;
				}
			}
		}
		
		if (e != nullptr)
		{
			if (dacInfo->connected == false)
			{
				if (dacInfo->connectTimer == 0)
				{
					dacInfo->connectTimer = 1000;
					
					if (etherdream_connect(e) == 0)
					{
						dacInfo->connected = true;
					}
				}
			}
			
			if (etherdream_is_ready(e))
			{
				//logDebug("DAC is ready. index: %d", i);
				
				//
				
				const int pps = 30000;
				const int repeatCount = 1;
				
				etherdream_point pts[kFrameSize];
				
				convertLaserImageToEtherdream(frame.points, kFrameSize, pts);
			
				const int result = etherdream_write(
					e,
					pts,
					kFrameSize,
					pps,
					repeatCount);
				
				if (result != 0)
				{
					logDebug("etherdream_write failed with code %d", result);
				}
			}
		}
	}
};

struct LaserInstance
{
	std::string name;
	
	Calibration calibration;
	
	CalibrationUi calibrationUi;
	
	LaserController laserController;
	
	LaserFrame frame;
	
	int lineSegmentIndex = 0;
	
	void init(const char * in_dacId, const int in_lineSegmentIndex)
	{
		name = in_dacId;
		
		laserController.init(in_dacId);
		
		lineSegmentIndex = in_lineSegmentIndex;
		
		calibration.init();
	
		calibrationUi.init(&calibration);
		calibrationUi.setEditorArea(100, 100, 200, 200);
	}
};

static void drawLineToLaserPoints(const Line::Point * points, const int numPoints, const float xOffset, LaserPoint * laserPoints)
{
	const float src_r = 1.f;
	const float src_g = 1.f;
	const float src_b = 1.f;
	
	for (int i = 0; i < numPoints; ++i)
	{
		auto & p_src = points[i];
		auto & p_dst = laserPoints[i];
		
		p_dst.x = + (p_src.x + xOffset) * 2.f - 1.f;
		p_dst.y = + p_src.y;
		
		p_dst.r = src_r;
		p_dst.g = src_g;
		p_dst.b = src_b;
	}
}

static void addPaddingForLaserFrame(LaserFrame & frame)
{
	LaserPoint first_pt = frame.points[kFramePadding];
	first_pt.r = first_pt.g = first_pt.b = 0.f;
	
	LaserPoint last_pt = frame.points[kFrameSize - 1 - kFramePadding];
	last_pt.r = last_pt.g = last_pt.b = 0.f;
	
	auto p_dst = frame.points;
	
	for (int i = 0; i < kFramePadding; ++i)
		p_dst[i] = first_pt;
	
#if 1
	for (int i = 0; i < 6; ++i)
		p_dst[i].r = 0;
#endif

	p_dst = frame.points + kFrameSize - kFramePadding;

	for (int i = 0; i < kFramePadding; ++i)
		p_dst[i] = last_pt;

#if 1
	for (int i = 0; i < 6; ++i)
		p_dst[i].r = frame.points[kFrameSize - 1 - kFramePadding].r;
#endif
}

static void testDistanceBasedSampling()
{
	while (!framework.quitRequested)
	{
		framework.process();
		
		Vec2 points[10];
		
		for (int i = 0; i < 10; ++i)
		{
			points[i][0] = (i - 5) * 20;
			points[i][1] = sinf(framework.time + i / 2.f) * 80.f;
		}
		
		float totalDistances[10];
		
		float totalDistance = 0.f;
		
		totalDistances[0] = 0.f;
		
		for (int i = 1; i < 10; ++i)
		{
			const Vec2 & p1 = points[i - 1];
			const Vec2 & p2 = points[i - 0];
			
			totalDistance += (p2 - p1).CalcSize();
			
			totalDistances[i] = totalDistance;
		}
		
		//printf("total distance: %.2f\n", totalDistance);
		
		const int numResampledPoints = 50;
		
		Vec2 resampledPoints[100];
		
		for (int i = 0; i < numResampledPoints; ++i)
		{
			const float distance = i / float(numResampledPoints - 1) * totalDistance;
			
			float * itr = std::upper_bound(totalDistances, totalDistances + 10, distance);
			
			const int index = itr - totalDistances;
			
			if (index == 0)
			{
				resampledPoints[i] = points[0];
			}
			else if (index == 10)
			{
				resampledPoints[i] = points[10 - 1];
			}
			else
			{
				const Vec2 & p1 = points[index - 1];
				const Vec2 & p2 = points[index - 0];
				
				const float distance1 = totalDistances[index - 1];
				const float distance2 = totalDistances[index - 0];
				
				const float t = (distance - distance1) / (distance2 - distance1);
				
				Assert(t >= 0.f && t <= 1.f);
				
				const Vec2 p = p1 * (1.f - t) + p2 * t;
				
				//printf("point: %.2f, %.2f\n", p[0], p[1]);
				
				resampledPoints[i] = p;
			}
		}
		
		//printf("done\n");
		
		framework.beginDraw(0, 0, 0, 0);
		gxTranslatef(VIEW_SX/2, VIEW_SY/2, 0);
		gxScalef(3, 3, 1);
		
		setColor(colorRed);
		hqBegin(HQ_FILLED_CIRCLES);
		for (int i = 0; i < 10; ++i)
		{
			hqFillCircle(points[i][0], points[i][1], 3.f);
		}
		hqEnd();
		
		setColor(colorWhite);
		hqBegin(HQ_FILLED_CIRCLES);
		for (int i = 0; i < numResampledPoints; ++i)
		{
			hqFillCircle(resampledPoints[i][0], resampledPoints[i][1], 1.f);
		}
		hqEnd();
		
		framework.endDraw();
	}
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	//testDistanceBasedSampling();
	
	FrameworkImGuiContext guiCtx;
	guiCtx.init();
	
	etherdream_lib_start();
	
	LaserInstance laserInstances[kNumLasers];
	
	for (int i = 0; i < kNumLasers; ++i)
	{
		std::string name;
		
		if (i == 0)
			name = "ab1ae3";
		else if (i == 1)
			name = "593834";
		else
		{
			std::ostringstream s;
			s << "dac " << (i + 1);
			name = s.str();
		}
		
		laserInstances[i].init(name.c_str(), i);
	}
	
	static const int kNumStrings = 3;
	
	PhysicalString1D<kLineSize * kNumLasers> strings[kNumStrings];
	strings[0].init(1.0, 100.0, 0.2f, 0.0f, 1.0f);
	strings[1].init(1.0, 100.0, 0.2f, 2.0f, 3.0f);
	strings[2].init(4.0, 100.0, 1.0, 0.0f, kNumLasers);
	
	GraviticSource gravitic;
	gravitic.z = .2f;
	gravitic.force = 1.f;
	gravitic.minimumDistance = .04f;
	
	NoiseSweepModifier noiseSweep;
	
	NoiseModulator noiseModulator;
	
	PurpleRain rain;
	
	bool pauseSimulation = false;
	float dropInterval = 1.f;
	float dropLife = .2f;
	float dropSize = .4f;
	float dropTimer = 0.f;
	
	bool showLine = true;
	bool showLaserFrame = true;
	bool enableMask = true;
	bool maskSmoothing = false;
	float maskValue1 = 0.f;
	float maskValue2 = 1.f;
	bool enableColorCycle = false;
	float colorCycleSpeed = .1f;
	float colorCyclePhase = 0.f;
	float colorCycleStretch = .1f;
	float laserIntensity = 0.f;
	bool enableOutput = true;
	bool outputRedOnly = true;
	bool enablePerspectiveCorrection = true;
	bool enablePinchCorrection = false;
	float pinchFactor = 0.f;
	bool rotate90 = true;
	float scaleFactor = .8f;
	float rotationAngle = 0.f;
	
	enum CalibrationPattern
	{
		kCalibrationPattern_None,
		kCalibrationPattern_Rectangle,
		kCalibrationPattern_RectanglePoints,
		kCalibrationPattern_VScroll,
		kCalibrationPattern_HScroll,
		kCalibrationPattern_COUNT
	};
	
	CalibrationPattern calibrationPattern = kCalibrationPattern_None;
	
	enum Tab
	{
		kTab_Visibility,
		kTab_Control,
		kTab_Calibration,
		kTab_Simulation
	};

	Tab tab = kTab_Visibility;
	
	int selectedLaserInstanceIndex = -1;
	
	while (!framework.quitRequested)
	{
		SDL_Delay(10);
		
		framework.process();
		
		bool inputIscaptured = false;
		
		guiCtx.processBegin(framework.timeStep, VIEW_SX, VIEW_SY, inputIscaptured);
		{
			if (ImGui::Begin("Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::PushItemWidth(100);
				{
					if (ImGui::Button("Visibility"))
						tab = kTab_Visibility;
					
					ImGui::SameLine();
					if (ImGui::Button("Control"))
						tab = kTab_Control;
					
					ImGui::SameLine();
					if (ImGui::Button("Calibration"))
						tab = kTab_Calibration;
					
					ImGui::SameLine();
					if (ImGui::Button("Simulation (debugging)"))
						tab = kTab_Simulation;
				}
				ImGui::PopItemWidth();
				
				auto doDacSelection = [&]()
				{
					const char * items[kNumLasers + 1];
					int numItems = 0;
					items[numItems++] = "(none)";
					for (int i = 0; i < kNumLasers; ++i)
						items[numItems++] = laserInstances[i].name.c_str();
					
					int selectedItem = selectedLaserInstanceIndex + 1;
					ImGui::Combo("Selected DAC", &selectedItem, items, numItems);
					selectedLaserInstanceIndex = selectedItem - 1;
				};
				
				if (tab == kTab_Visibility)
				{
					ImGui::Text("Visibility");
					ImGui::Checkbox("Show line", &showLine);
					ImGui::Checkbox("Show laser frame", &showLaserFrame);
					ImGui::Checkbox("Enable masking", &enableMask);
					ImGui::Checkbox("Enable mask smoothing", &maskSmoothing);
					ImGui::SliderFloat("Mask value 1", &maskValue1, 0.f, 1.f);
					ImGui::SliderFloat("Mask value 2", &maskValue2, 0.f, 1.f);
					ImGui::Checkbox("Enable color cycle", &enableColorCycle);
					ImGui::SliderFloat("Color cycle speed", &colorCycleSpeed, 0.f, 4.f, "%.3f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Color cycle stretch", &colorCycleStretch, 0.f, 1.f, "%.3f", ImGuiSliderFlags_Logarithmic);
					doDacSelection();
					ImGui::Checkbox("Enable laser output", &enableOutput);
					ImGui::Checkbox("Output red only", &outputRedOnly);
					ImGui::SliderFloat("Laser intensity", &laserIntensity, 0.f, 1.f);
				}
				else if (tab == kTab_Control)
				{
					doDacSelection();
					
					if (selectedLaserInstanceIndex != -1)
					{
						auto & laserInstance = laserInstances[selectedLaserInstanceIndex];
					
						ImGui::RadioButton("Detected", laserInstance.laserController.isDetected);
						
						ImGui::SliderInt("Line segment", &laserInstance.lineSegmentIndex, 0, kNumLasers - 1);
					}
				}
				else if (tab == kTab_Simulation)
				{
					ImGui::Checkbox("Pause simulation", &pauseSimulation);
					
					ImGui::SliderFloat("Rain drop interval", &dropInterval, 0.001f, 2.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Rain drop life", &dropLife, 0.001f, 2.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Rain drop size", &dropSize, 0.001f, 2.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					
					for (int i = 0; i < kNumStrings; ++i)
					{
						ImGui::PushID(i);
						{
							ImGui::Text("String %d", i + 1);
							
							float mass = strings[i].mass;
							float tension = strings[i].tension;
							ImGui::SliderFloat("String mass", &mass, 0.f, 10.f, "%.4f", ImGuiSliderFlags_Logarithmic);
							ImGui::SliderFloat("String tension", &tension, 0.f, 100000.f, "%.4f", ImGuiSliderFlags_Logarithmic);
							strings[i].mass = mass;
							strings[i].tension = tension;
						}
						ImGui::PopID();
					}
					ImGui::SliderFloat("Gravitic force", &gravitic.force, 0.f, 10.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Gravitic minimum distance", &gravitic.minimumDistance, 0.f, 1.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Gravitic z position", &gravitic.z, -1.f, +1.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					
					if (ImGui::Button("Start noise sweep"))
						noiseSweep.begin(-1.f, +5.f);
					ImGui::SliderFloat("Noise sweep strength", &noiseSweep.strength, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Noise sweep duration", &noiseSweep.duration, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Noise sweep noise frequency", &noiseSweep.noiseFrequency, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					
					ImGui::Separator();
					ImGui::SliderFloat("Noise modulator strength", &noiseModulator.strength, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Noise modulator follow factor", &noiseModulator.followFactor, 0.f, 1.f, "%.4f");
					ImGui::SliderFloat("Noise modulator falloff", &noiseModulator.falloff_strength, 0.f, 200.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Noise modulator spatial frequency", &noiseModulator.noiseFrequency_spat, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat("Noise modulator time frequency", &noiseModulator.noiseFrequency_time, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic);
				}
				else if (tab == kTab_Calibration)
				{
					ImGui::Text("Canvas setup (global)");
					ImGui::Checkbox("Rotate canvas 90 degrees", &rotate90);
					ImGui::SliderFloat("Canvas scale factor", &scaleFactor, 0.2f, 1.f);
					ImGui::SliderFloat("Canvas rotation (for testing)", &rotationAngle, 0.f, 360.f);
				
					ImGui::NewLine();
					ImGui::Checkbox("Enable perspective correction", &enablePerspectiveCorrection);
					ImGui::Checkbox("Enable pinch correction", &enablePinchCorrection);
					ImGui::InputFloat("Pinch amount", &pinchFactor, -1.f, 1.f);
					
					ImGui::Separator();
					
					doDacSelection();
					
					if (selectedLaserInstanceIndex != -1)
					{
						{
							int itemIndex = calibrationPattern;
							const char * items[kCalibrationPattern_COUNT] =
							{
								"None",
								"Rectangle",
								"Rectangle points",
								"V-Scroll",
								"H-Scroll"
							};
							ImGui::Combo("Calibration pattern", &itemIndex, items, kCalibrationPattern_COUNT);
							calibrationPattern = (CalibrationPattern)itemIndex;
						}
					
						auto & laserInstance = laserInstances[selectedLaserInstanceIndex];
						
						ImGui::Text("Calibration points");
						ImGui::PushID(laserInstance.name.c_str());
						ImGui::PushItemWidth(400);
						{
							for (int i = 0; i < 4; ++i)
							{
								Homography::Vertex * vertices[4] =
								{
									&laserInstance.calibration.homography.v00,
									&laserInstance.calibration.homography.v01,
									&laserInstance.calibration.homography.v10,
									&laserInstance.calibration.homography.v11
								};
								
								char name[32];
								sprintf(name, "v%d", i + 1);
								ImGui::SliderFloat2(name, (float*)&vertices[i]->viewPosition, -.5f, +.5f, "%.6f", ImGuiSliderFlags_Logarithmic);
							}
						}
						ImGui::PopItemWidth();
						ImGui::PopID();
					}
				}
			}
			ImGui::End();
		}
		guiCtx.processEnd();
		
		for (int i = 0; i < kNumLasers; ++i)
		{
			auto & laserInstance = laserInstances[i];
			
			if (tab == kTab_Calibration && selectedLaserInstanceIndex == i)
			{
				laserInstance.calibrationUi.setEditorArea(50, 50, 700, 700);
				
				const float sensitivity = (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT)) ? .2f : .02f;
				
				laserInstance.calibrationUi.tickEditor(sensitivity, framework.timeStep, inputIscaptured);
			}
			else
			{
				laserInstance.calibrationUi.cancelEditing();
			}
		}
		
		const float dt = pauseSimulation ? 0.f : keyboard.isDown(SDLK_SPACE) ? 0.f : fminf(1.f / 30.f, framework.timeStep);
		
		dropTimer += dt;
		
		while (dropTimer >= dropInterval && dropInterval != 0.f)
		{
			dropTimer -= dropInterval;
			
			rain.addRainDrop(random(0.f, 1.f), dropLife);
		}
		
		if (enableColorCycle)
		{
			colorCyclePhase = fmodf(colorCyclePhase + dt * colorCycleSpeed, 1.f);
		}
		
		rain.tick(dt);
		
		//
		
		if (inputIscaptured == false && mouse.wentDown(BUTTON_LEFT))
		{
			inputIscaptured = true;
			
			const int offset = rand() % strings[0].getNumPoints();
			
			const int radius = 200;
			
			for (int i = -radius; i <= +radius; ++i)
			{
				const int position = offset + i - radius;;
				
				if (position >= 0 && position < strings[0].getNumPoints())
				{
					const float t = i / float(radius);
					
					float value = powf(cosf(t / 2.f * float(M_PI)), 2.f);
					
					const float sign = (rand() % 2) ? -1.f : +1.f;
					
					value *= powf(random(0.f, +1.f), 2.f) * sign;
					
					strings[0].positions[position] += value * .2f;
				}
			}
		}
		
		//
		
		Line line;
		line.init(kLineSize * kNumLasers, 0.f, kNumLasers);
		
		// update gravitic source location
		
		gravitic.x = mouse.x / 200.f;
		gravitic.y = (mouse.y - 200.f) / 200.f;
		
		logDebug("gravitic location: %.2f, %.2f", gravitic.x, gravitic.y);
		
		// apply gravitic source
		
		for (auto & string : strings)
		{
			for (int i = 0; i < string.getNumPoints(); ++i)
			{
				const float x = string.getXForPointIndex(i);
				const float y = string.positions[i];
			
				const Vec2 force = gravitic.calculateForce(x, y);
				//const Vec2 force = gravitic.calculateForce(x, 0.f);
				
				string.forces[i] += force[1];
			}
		}
		
		{
			// apply noise sweep
			
			const int numSteps = 1;
			
			for (int i = 0; i < numSteps; ++i)
			{
				noiseSweep.tick(dt / numSteps);
				
				for (auto & string : strings)
				{
					for (int i = 0; i < string.getNumPoints(); ++i)
					{
						const float x = string.getXForPointIndex(i);
					
						const float force = noiseSweep.calculateForce(x);
						
						string.forces[i] += force / numSteps;
					}
				}
			}
		}
		
		{
			// apply noise modulator
			
			const int numSteps = 1;
			
			for (int i = 0; i < numSteps; ++i)
			{
				noiseModulator.tick(gravitic.x, dt / numSteps);
				
				for (auto & string : strings)
				{
					for (int i = 0; i < string.getNumPoints(); ++i)
					{
						const float x = string.getXForPointIndex(i);
						const float y = string.positions[i];
					
						const float force = noiseModulator.calculateForce(x, y);
						
						// distance falloff
						const float dx = x - gravitic.x;
						const float dy = y - gravitic.y;
						const float dz = 0.f - gravitic.z;
						
						float minimumDistance = 1e-3f;
						const float minimumDistanceSq = minimumDistance * minimumDistance;
						const float distanceSq = fmaxf(minimumDistanceSq, dx * dx + dy * dy + dz * dz);

						const float falloff = 1.f / distanceSq;
						
						string.forces[i] += force * falloff / numSteps;
					}
				}
			}
		}
		
		// simulate
		
		for (auto & string : strings)
		{
			for (int i = 0; i < 10; ++i)
				string.tick(dt / 10.0);
		}
		
		// compose line
		
		line.newFrame();
		
		for (auto & string : strings)
		{
			string.drawOntoLine(line, 2.f);
		}
		
		// generate mask
		
		Mask mask;

		for (auto & rainDrop : rain.rainDrops)
		{
			if (rainDrop.life > 0.f)
			{
				const float size = dropSize * rainDrop.life;
				
				const float begin = rainDrop.position - size/2.f;
				const float end = rainDrop.position + size/2.f;
				
				mask.addSegment(begin, end);
				
				//drawLine(begin * 400.f, 500.f, end * 400.f, 500.f);
			}
		}

		mask.finalize();
		
		//
		
		for (int i = 0; i < kNumLasers; ++i)
		{
			auto & laserInstance = laserInstances[i];
			
			// create laser frame from line
			
			LaserFrame & frame = laserInstance.frame;
			
			if (laserInstance.lineSegmentIndex >= 0 && laserInstance.lineSegmentIndex < kNumLasers)
			{
				const int offset = laserInstance.lineSegmentIndex * kLineSize;
				
				auto & firstPoint = line.points[offset];
				
				drawLineToLaserPoints(&firstPoint, kLineSize, -firstPoint.x, frame.points + kFramePadding);
			}
			else
			{
				memset(&frame, 0, sizeof(frame));
			}
			
			// apply color cycle
			
			if (enableColorCycle)
			{
				for (auto & point : frame.points)
				{
					const float angle = colorCyclePhase + point.x * colorCycleStretch;
					
					auto color = Color::fromHSL(angle, 1.f, .5f);
					
					point.r = color.r;
					point.g = color.g;
					point.b = color.b;
				}
			}
			
			// apply mask to frame
		
			if (enableMask)
			{
				auto points = frame.points + kFramePadding;
				
				// determine begin and end for line segment
				
				int point_index = 0;
				
				int segment_index = 0;
				
				while (segment_index < mask.numMergedSegments && mask.mergedSegments[segment_index].end < 0.f)
					++segment_index;
				
				while (segment_index < mask.numMergedSegments && mask.mergedSegments[segment_index].begin < 1.f)
				{
					auto & segment = mask.mergedSegments[segment_index];
					
					while (point_index < kLineSize && (points[point_index].x + 1.f)/2.f /* fixme # hack */ < segment.begin)
					{
						points[point_index].r *= maskValue1;
						points[point_index].g *= maskValue1;
						points[point_index].b *= maskValue1;
						
						point_index++;
					}
					
				#if 1
					if (maskSmoothing && point_index < kLineSize && point_index > 0)
					{
						const float x = segment.begin * 2.f - 1.f;
						
						const float x1 = points[point_index - 1].x;
						const float x2 = points[point_index - 0].x;
						
						const float y1 = points[point_index - 1].y;
						const float y2 = points[point_index - 0].y;
						
						// x1 + (x2 - x1) * t = x
						// t = (x - x1) / (x2 - x1)
						
						const float t = (x - x1) / (x2 - x1);
						Assert(t >= 0.f && t <= 1.f);
						
						points[point_index - 1].x = x1 + (x2 - x1) * t;
						points[point_index - 1].y = y1 + (y2 - y1) * t;
						
					// fixme # wrong color
						points[point_index - 1].r = 1.f;
						points[point_index - 1].g = 1.f;
						points[point_index - 1].b = 1.f;
					}
				#else
					const int begin_index = point_index > 0 ? point_index - 1 : point_index;
				#endif
					
					while (point_index < kLineSize && (points[point_index].x + 1.f)/2.f /* fixme # hack */ <= segment.end)
					{
						points[point_index].r *= maskValue2;
						points[point_index].g *= maskValue2;
						points[point_index].b *= maskValue2;
						
						point_index++;
					}
					
				#if 1
					if (maskSmoothing && point_index < kLineSize && point_index > 0)
					{
						const float x = segment.end * 2.f - 1.f;
						
						const float x1 = points[point_index - 1].x;
						const float x2 = points[point_index - 0].x;
						
						const float y1 = points[point_index - 1].y;
						const float y2 = points[point_index - 0].y;
						
						// x1 + (x2 - x1) * t = x
						// t = (x - x1) / (x2 - x1)
						
						const float t = (x - x1) / (x2 - x1);
						Assert(t >= 0.f && t <= 1.f);
						
						points[point_index - 0].x = x1 + (x2 - x1) * t;
						points[point_index - 0].y = y1 + (y2 - y1) * t;
						
					// fixme # wrong color
						points[point_index - 0].r = 1.f;
						points[point_index - 0].g = 1.f;
						points[point_index - 0].b = 1.f;
						
						point_index++;
					}
				#else
					if (point_index < kLineSize)
						point_index++;
					
					const int end_index = point_index;
				#endif
				
				#if 0
					if (begin_index < kLineSize)
					{
						const int end_index_clamped = end_index <= kLineSize ? end_index : kLineSize;
					
						const int num_points = end_index_clamped - begin_index;
						
						if (num_points >= 2)
						{
							const float x1 = fmaxf(segment.begin * 2.f - 1.f, points[0].x);
							const float x2 = fminf(segment.end * 2.f - 1.f, points[kLineSize - 1].x);
							
							for (int i = begin_index; i < end_index_clamped; ++i)
							{
								Assert(i >= 0 && i < kLineSize);
								
								const float t = (i - begin_index) / float(num_points - 1);
								Assert(t >= 0.f && t <= 1.f);
								
								points[i].x = x1 + (x2 - x1) * t;
							}
						}
						else
						{
							logDebug("no interp");
						}
					}
				#endif
					
					++segment_index;
				}
				
				while (point_index < kLineSize)
				{
					points[point_index].r *= maskValue1;
					points[point_index].g *= maskValue1;
					points[point_index].b *= maskValue1;
					
					point_index++;
				}
			}
			
			addPaddingForLaserFrame(frame);
	
			//
			
			if (calibrationPattern == kCalibrationPattern_Rectangle)
				drawCalibrationPattern_rectangle(frame.points, kFrameSize);
			else if (calibrationPattern == kCalibrationPattern_RectanglePoints)
				drawCalibrationPattern_rectanglePoints(frame.points, kFrameSize);
			else if (calibrationPattern == kCalibrationPattern_VScroll)
				drawCalibrationPattern_line_vscroll(frame.points, kFrameSize, framework.time * .3f);
			else if (calibrationPattern == kCalibrationPattern_HScroll)
				drawCalibrationPattern_line_hscroll(frame.points, kFrameSize, framework.time * .4f);
			else
				Assert(calibrationPattern == kCalibrationPattern_None);
			
			if (rotationAngle != 0.f)
			{
				Mat4x4 mat;
				mat.MakeRotationZ(rotationAngle * float(M_PI) / 180.f);
				
				for (auto & point : laserInstance.frame.points)
				{
					auto p = mat.Mul(Vec2(point.x, point.y));
					point.x = p[0];
					point.y = p[1];
				}
			}
			
			if (rotate90)
			{
				for (auto & point : laserInstance.frame.points)
					std::swap(point.x, point.y);
			}
			
			if (enablePerspectiveCorrection)
			{
				laserInstance.calibration.applyTransform(laserInstance.frame);
			}
			
			if (scaleFactor != 1.f)
			{
				for (auto & point : laserInstance.frame.points)
				{
					point.x *= scaleFactor;
					point.y *= scaleFactor;
				}
			}
			
			for (auto & point : frame.points)
			{
				point.r *= laserIntensity;
				point.g *= laserIntensity;
				point.b *= laserIntensity;
			}
			
			if (outputRedOnly)
			{
				for (auto & point : frame.points)
				{
					point.g = 0.f;
					point.b = 0.f;
				}
			}
			
			if (enableOutput)
			{
				laserInstance.laserController.send(laserInstance.frame);
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			if (showLine)
			{
				gxPushMatrix();
				{
					gxTranslatef(0, 200, 0);
					gxScalef(200, 200, 1);
					
					setColor(colorWhite);
					gxBegin(GX_LINES);
					{
						for (int i = 0; i < line.points.size() - 1; ++i)
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
			}
			
			if (showLaserFrame && selectedLaserInstanceIndex != -1)
			{
				auto & laserInstance = laserInstances[selectedLaserInstanceIndex];
				auto & frame = laserInstance.frame;
				
				pushBlend(BLEND_ADD);
				gxPushMatrix();
				{
					gxTranslatef(VIEW_SX/2, VIEW_SY/2, 0);
					gxScalef(300, 300, 1);
					
					gxBegin(GX_LINES);
					{
						for (int i = 0; i < kFrameSize - 1; ++i) // todo # closed
						{
							auto & point1 = frame.points[(i + 0) % kFrameSize];
							auto & point2 = frame.points[(i + 1) % kFrameSize];
							
							gxColor4f(point1.r, point1.g, point1.b, 1.f);
							gxVertex2f(point1.x, point1.y);
							
							gxColor4f(point2.r, point2.g, point2.b, 1.f);
							gxVertex2f(point2.x, point2.y);
						}
					}
					gxEnd();
					
					hqBegin(HQ_FILLED_CIRCLES, true);
					{
						for (auto & point : frame.points)
						{
							setColorf(point.r, point.g, point.b);
							hqFillCircle(point.x, point.y, 4.f);
						}
					}
					hqEnd();
				}
				gxPopMatrix();
				popBlend();
			}
			
		#if 1
			pushBlend(BLEND_ADD);
			{
				setColor(200, 150, 100);
				
				gxBegin(GX_LINES);
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
		#endif
		
			for (int i = 0; i < kNumLasers; ++i)
			{
				if (tab == kTab_Calibration && selectedLaserInstanceIndex == i)
				{
					auto & laserInstance = laserInstances[i];
					
					laserInstance.calibrationUi.drawEditorPreview();
					
					laserInstance.calibrationUi.drawEditor();
				}
			}
			
			guiCtx.draw();
		}
		framework.endDraw();
	}
	
	guiCtx.shut();
	
	framework.shutdown();
	
	return 0;
}
