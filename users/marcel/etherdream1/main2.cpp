#include "calibration.h"
#include "etherdream.h"
#include "framework.h"
#include "homography.h"
#include "imgui-framework.h"
#include "laserTypes.h"
#include <algorithm>
#include <map>

static const int kFrameSize = 500;
static const int kNumLasers = 4;

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
	static const int kNumPoints = kFrameSize * kNumLasers;

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

static uint16_t unormFloatToU16(float in_v)
{
	if (in_v < 0.f)
		in_v = 0.f;
	else if (in_v > 1.f)
		in_v = 1.f;
	
	return uint16_t(in_v * ((1 << 16) - 1));
}

static uint16_t snormFloatToS16(float in_v)
{
	if (in_v < -1.f)
		in_v = -1.f;
	else if (in_v > +1.f)
		in_v = +1.f;
	
	return int16_t(in_v * ((1 << 15) - 1));
}

static void convertLaserImageToEtherdream(
	const LaserPoint * points,
	const int numPoints,
	etherdream_point * out_points)
{
	for (int i = 0; i < numPoints; ++i)
	{
		auto & src = points[i];
		auto & dst = out_points[i];
		
		dst.x = snormFloatToS16(src.x);
		dst.y = snormFloatToS16(src.y);
		
		dst.r = unormFloatToU16(src.r);
		dst.g = unormFloatToU16(src.g);
		dst.b = unormFloatToU16(src.b);
		
		dst.i = uint16_t(-1);
		dst.u1 = 0; // unused
		dst.u2 = 0;
	}
}

struct LaserFrame
{
	LaserPoint points[kFrameSize];
};

struct Calibration
{
	Homography homography;
	
	void init()
	{
		homography.v00.viewPosition.Set(-1.f, -1.f);
		homography.v10.viewPosition.Set(+1.f, -1.f);
		homography.v01.viewPosition.Set(-1.f, +1.f);
		homography.v11.viewPosition.Set(+1.f, +1.f);
	}
	
	void applyTransform(LaserFrame & frame)
	{
		for (auto & point : frame.points)
		{
			// todo : do a proper transform
			
			const float canvasScale = .8f;
			
			point.x *= canvasScale;
			point.y *= canvasScale;
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
	
	void tickEditor(const float dt, bool & inputIscaptured)
	{
		auto mousePos = calculateMousePosition();
		
		logDebug("mouse pos: %.2f, %.2f", mousePos[0], mousePos[1]);
		
		calibration->homography.tickEditor(mousePos, dt, inputIscaptured);
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
		gxBegin(GL_LINE_STRIP);
		glEnable(GL_LINE_SMOOTH); // todo : remove, replace with hq lines
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		checkErrorGL();
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
		glDisable(GL_LINE_SMOOTH);
		
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
				
				const int numPoints = kFrameSize; // 30000/60 = 500 (60fps)
				const int padding = 50;
				
				etherdream_point pts[numPoints];
				
				convertLaserImageToEtherdream(frame.points, kFrameSize, pts);
				
				etherdream_point padded_pts[numPoints + padding * 2];
				
				auto first_pt = pts[0];
				auto last_pt = pts[numPoints - 1];
				
				first_pt.r =
					first_pt.g =
					first_pt.b =
					first_pt.i = 0;
				
				last_pt.r =
					last_pt.g =
					last_pt.b =
					last_pt.i = 0;
				
				for (int i = 0; i < padding; ++i)
					padded_pts[i] = first_pt;
				
				for (int i = 0; i < numPoints; ++i)
					padded_pts[padding + i] = pts[i];
				
			#if 1
				// todo : add delay lines for r, g, b
				
				for (int i = 0; i < 6; ++i)
					padded_pts[padding + i].r = 0;
			#endif
				
				for (int i = 0; i < padding; ++i)
					padded_pts[padding + numPoints + i] = last_pt;
				
			#if 1
				// todo : add delay lines for r, g, b
				
				for (int i = 0; i < 6; ++i)
					padded_pts[padding + numPoints + i].r = pts[numPoints - 1].r;
			#endif
				
			#if 0
				static int rev = 0;
				rev++;
				
				if ((rev % 1) == 0)
				{
					for (int i = 0; i < numPoints / 2; ++i)
						std::swap(pts[i], pts[numPoints - 1 - i]);
				}
			#endif
			
				const int result = etherdream_write(
					e,
					padded_pts,
					numPoints + padding * 2,
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
	Calibration calibration;
	
	CalibrationUi calibrationUi;
	
	LaserController laserController;
	
	LaserFrame frame;
	
	void init(const char * dacId)
	{
		laserController.init(dacId);
		
		calibration.init();
	
		calibrationUi.init(&calibration);
		calibrationUi.setEditorArea(100, 100, 200, 200);
	}
};

static void drawLaserFrame(const LaserFrame & frame)
{
	gxBegin(GL_LINES);
	{
		for (auto & point : frame.points)
		{
			gxColor4f(point.r, point.g, point.b, 1.f);
			gxVertex2f(point.x, point.y);
		}
	}
	gxEnd();
}

int main(int argc, char * argv[])
{
	if (!framework.init(800, 600))
		return -1;
	
	FrameworkImGuiContext guiCtx;
	guiCtx.init();
	
	const bool isEtherdreamStarted = etherdream_lib_start() == 0;
	
	LaserInstance laserInstances[kNumLasers];
	
	for (int i = 0; i < kNumLasers; ++i)
	{
		const char * dacId = i == 0 ? "ab1ae3" : "";
		
		laserInstances[i].init(dacId);
	}
	
	for (int i = 0; i < Line::kNumPoints; ++i)
		Line::initialLineXs[i] = i / float(kFrameSize);
	
	PhysicalString1D<4> string;
	string.init(1.0, 100.0);
	
	GraviticSource gravitic;
	gravitic.z = .2f;
	gravitic.force = 1.f;
	gravitic.minimumDistance = .04f;
	
	PurpleRain rain;
	
	float dropInterval = .01f;
	float dropTimer = 0.f;
	
	bool showLine = true;
	bool showLaserFrame = false;
	bool enableOutput = true;
	bool enableCalibration = true;
	
	enum CalibrationImage
	{
		kCalibrationImage_None,
		kCalibrationImage_Rectangle,
		kCalibrationImage_VScroll,
		kCalibrationImage_HScroll,
		kCalibrationImage_COUNT
	};
	
	CalibrationImage calibrationImage = kCalibrationImage_None;
	
	while (!framework.quitRequested)
	{
		SDL_Delay(10);
		
		framework.process();
		
		bool inputIscaptured = false;
		
		for (int i = 0; i < kNumLasers; ++i)
		{
			auto & laserInstance = laserInstances[i];
			
			laserInstance.calibrationUi.setEditorArea(20 + i * 100, 20, 80, 80);
			
			laserInstance.calibrationUi.tickEditor(framework.timeStep, inputIscaptured);
		}
		
		guiCtx.processBegin(framework.timeStep, 800, 600, inputIscaptured);
		{
			if (ImGui::Begin("Controls"))
			{
				ImGui::Text("Visibility");
				ImGui::Checkbox("Show line", &showLine);
				ImGui::Checkbox("Show laser frame", &showLaserFrame);
				ImGui::Checkbox("Enable laser output", &enableOutput);
				
				ImGui::Text("Calibration");
				ImGui::Checkbox("Enable correction", &enableCalibration);
				{
					int itemIndex = calibrationImage;
					const char * items[kCalibrationImage_COUNT] =
					{
						"None",
						"Rectangle",
						"V-Scroll",
						"H-Scroll"
					};
					ImGui::Combo("Calibration image", &itemIndex, items, kCalibrationImage_COUNT);
					calibrationImage = (CalibrationImage)itemIndex;
				}
				
				{
					float mass = string.mass;
					float tension = string.tension;
					ImGui::SliderFloat("String mass", &mass, 0.f, 10.f, "%.4f", 2.f);
					ImGui::SliderFloat("String tension", &tension, 0.f, 1000.f, "%.4f", 2.f);
					string.mass = mass;
					string.tension = tension;
				}
				ImGui::SliderFloat("Rain drop interval", &dropInterval, 0.001f, 2.f, "%.4f", 2.f);
				ImGui::SliderFloat("Gravitic force", &gravitic.force, 0.f, 10.f, "%.4f", 2.f);
				ImGui::SliderFloat("Gravitic minimum distance", &gravitic.minimumDistance, 0.f, 1.f, "%.4f", 2.f);
				ImGui::SliderFloat("Gravitic z position", &gravitic.z, -1.f, +1.f, "%.4f", 2.f);
				
				for (auto & laserInstance : laserInstances)
				{
					ImGui::PushID(&laserInstance);
					{
						ImGui::Text("Laser Instance");
						
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
							ImGui::SliderFloat2(name, (float*)&vertices[i]->viewPosition, -1.f, +1.f, "%.6f", .2f);
						}
					}
					ImGui::PopID();
				}
			}
			ImGui::End();
		}
		guiCtx.processEnd();
		
		const float dt = keyboard.isDown(SDLK_SPACE) ? 0.f : framework.timeStep;
		
		dropTimer += dt;
		
		while (dropTimer >= dropInterval && dropInterval != 0.f)
		{
			dropTimer -= dropInterval;
			
			rain.addRainDrop(random(0.f, 1.f), 6.f);
		}
		
		rain.tick(dt);
		
		//
		
		if (inputIscaptured == false && mouse.wentDown(BUTTON_LEFT))
		{
			inputIscaptured = true;
			
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
		
		for (int i = 0; i < kNumLasers; ++i)
		{
			auto & laserInstance = laserInstances[i];
			
			// create laser frame from line
	
			LaserFrame & frame = laserInstance.frame;
			
			const int offset = i * kFrameSize;
			
			for (int i = 0; i < kFrameSize; ++i)
			{
				auto & p_src = line.points[offset + i];
				auto & p_dst = frame.points[i];
				
				p_dst.x = + p_src.x * 2.f - 1.f;
				p_dst.y = - p_src.y;
				
				p_dst.r = 1.f;
				p_dst.g = 0.f;
				p_dst.b = 0.f;
			}
	
			//
			
			if (calibrationImage == kCalibrationImage_Rectangle)
				drawCalibrationImage_rectangle(frame.points, kFrameSize);
			else if (calibrationImage == kCalibrationImage_VScroll)
				drawCalibrationImage_line_vscroll(frame.points, kFrameSize, framework.time * .3f);
			else if (calibrationImage == kCalibrationImage_HScroll)
				drawCalibrationImage_line_hscroll(frame.points, kFrameSize, framework.time * .4f);
			else
				Assert(calibrationImage == kCalibrationImage_None);
			
			if (enableCalibration)
			{
				laserInstance.calibration.applyTransform(laserInstance.frame);
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
			}
			
			if (showLaserFrame)
			{
				auto & laserInstance = laserInstances[0];
				auto & frame = laserInstance.frame;
				
				pushBlend(BLEND_ADD);
				gxPushMatrix();
				{
					gxTranslatef(400, 300, 0);
					gxScalef(200, 200, 1);
					
					gxBegin(GL_LINE_LOOP);
					{
						for (int i = 0; i < kFrameSize; ++i)
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
				}
				gxPopMatrix();
				popBlend();
			}
			
		#if 0
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
		#endif
		
			guiCtx.draw();
			
			for (auto & laserInstance : laserInstances)
			{
				laserInstance.calibrationUi.drawEditorPreview();
				
				laserInstance.calibrationUi.drawEditor();
			}
		}
		framework.endDraw();
	}
	
	guiCtx.shut();
	
	framework.shutdown();
	
	return 0;
}
