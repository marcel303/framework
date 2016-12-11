#include "Calc.h"
#include "framework.h"
#include "Timer.h"
#include "video.h"
#include <list>

#include <array>

#define ENABLE_MEDIA_FOUNDATION 0
#define ENABLE_VIDEO_FOR_WINDOWS 0
#define ENABLE_VIDEOIN 0
#define ENABLE_FACE_DETECTION 0
#define ENABLE_MOTION_DETECTION 0
#define ENABLE_VIOLA_JONES 0

#define GFX_SX 1920
#define GFX_SY 1080

#if ENABLE_MEDIA_FOUNDATION
	#include <Mfapi.h>
	#include <MfIdl.h>
	#include <MfReadwrite.h>
	#pragma comment(lib, "Mfplat.lib")
	#pragma comment(lib, "Mfuuid.lib")
	#pragma comment(lib, "MfReadWrite.lib")
	#pragma comment(lib, "Mf.lib")
#endif

#if ENABLE_VIDEO_FOR_WINDOWS
	#include <Windows.h>
	#include <Vfw.h>
	#pragma comment(lib, "Vfw32.lib")
#endif

#if ENABLE_VIDEOIN
	#include "videoin.h"
#endif

#if ENABLE_FACE_DETECTION
	#include "image.h"
#endif

#if ENABLE_VIOLA_JONES
	#include "image.h"

	#include "viola-jones/src/Feature.h"
	#include "viola-jones/src/WeakClassifier.h"
	#include "viola-jones/src/StrongClassifier.h"
	#include "viola-jones/src/CascadeClassifier.h"

float * integral_image(float * grayImage, const int sx, const int sy)
{
	const int bufferSize = sx * sy;

	float * __restrict ii = new float[bufferSize];
	float * __restrict s = new float[bufferSize];

	for (int y = 0; y < sy; ++y)
	{
		for (int x = 0; x < sx; ++x)
		{
			if (x == 0)
				s[(y*sx)+x] = grayImage[(y*sx)+x];
			else
				s[(y*sx)+x] = s[(y*sx)+x-1] + grayImage[(y*sx)+x];

			if (y == 0)
				ii[(y*sx)+x] = s[(y*sx)+x];
			else
				ii[(y*sx)+x] = ii[((y-1)*sx)+x] + s[(y*sx)+x];
		}
	}

	delete[] s;

	return ii;
}

float * squared_integral_image(const float * __restrict grayImage, const int sx, const int sy)
{
	const int bufferSize = sx * sy;
	float * __restrict ii = new float[bufferSize];
	float * __restrict s = new float[bufferSize];

	for (int y = 0; y < sy; ++y)
	{
		for (int x = 0; x < sx; ++x)
		{
			if (x == 0)
				s[(y*sx)+x] = pow(grayImage[(y*sx)+x], 2);
			else
				s[(y*sx)+x] = s[(y*sx)+x-1] + pow(grayImage[(y*sx)+x], 2);

			if (y == 0)
				ii[(y*sx)+x] = s[(y*sx)+x];
			else
				ii[(y*sx)+x] = ii[((y-1)*sx)+x] + s[(y*sx)+x];
		}
	}

	delete[] s;

	return ii;
}

float evaluate_integral_rectangle(const float * __restrict ii, const int iiwidth, const int x, const int y, const int sx, const int sy)
{
	float value = ii[((y+sy-1)*iiwidth)+(x+sx-1)];

	if (x > 0)
		value -= ii[((y+sy-1)*iiwidth)+(x-1)];
	if (y > 0)
		value -= ii[(y-1)*iiwidth+(x+sx-1)];
	if (x > 0 && y > 0)
		value += ii[(y-1)*iiwidth+(x-1)];

	return value;
}

#endif

static float scrollX = 0.f;

static float calculateHeight(const float t)
{
	const float baseY = GFX_SY/2.f;
	
	float a = 1.f;
	float f = Calc::m2PI / 1000.f;
	float at = 0.f;

	float result = 0.f;

	for (int i = 0; i < 5; ++i)
	{
		result += std::sinf(t * f) * a;

		at += a;
		a /= 1.210f;
		f *= 1.321f;
	}

	result = baseY + result / at * 200.f;

	return result;
}

struct Pisu
{
	Pisu()
		: x(GFX_SX/2.f)
		, y(0.f)
		, vy(0.f)
	{
	}

	void tick(const float dt)
	{
		const float h = calculateHeight(scrollX + x);

		if (y > h)
		{
			const float dh = h - y;
			const float vm = dh / dt;

			y = h;

			if (vy > 0.f)
				vy *= -.5f;
			
			if (vy > vm)
				vy = vm;
		}

		vy += 200.f * dt;
		
		y += vy * dt;
	}

	void draw() const
	{
		setBlend(BLEND_ALPHA);
		setColor(colorWhite);
		//fillCircle(x, y, 20.f, 30);

		Sprite pisu("pisu.png");
		pisu.pivotX = pisu.getWidth()/2;
		pisu.pivotY = pisu.getHeight();
		pisu.drawEx(x, y, 0.f, .1f);
	}

	float x;
	float y;
	float vy;
};

struct ParticleSystem
{
	static const int kMaxParticles = 25000;

	float lt[kMaxParticles];
	float lr[kMaxParticles];
	float x[kMaxParticles];
	float y[kMaxParticles];
	float z[kMaxParticles];
	float vx[kMaxParticles];
	float vy[kMaxParticles];
	float vz[kMaxParticles];

	double spawnTimer;
	double spawnInterval;
	int spawnIndex;

	ParticleSystem()
	{
		memset(this, 0, sizeof(*this));
	}

	void spawn(const float life)
	{
		const float x1 = -2000.f;
		const float y1 = -2000.f * .5f;
		const float z1 = -2000.f;
		const float x2 = +2000.f;
		const float y2 = +2000.f;
		const float z2 = +2000.f;

		lt[spawnIndex] = life;
		lr[spawnIndex] = 1.f / life;
		x[spawnIndex] = random(x1, x2);
		y[spawnIndex] = random(y1, y2);
		z[spawnIndex] = random(z1, z2);
		vx[spawnIndex] = random(-10.f, +10.f);
		vy[spawnIndex] = random(-10.f, +80.f);
		vz[spawnIndex] = random(-10.f, +10.f);

		spawnIndex = (spawnIndex + 1) % kMaxParticles;
	}

	void tick(const float dt)
	{
		spawnInterval = 1.0 / 60.0 / 100.0;

		if (spawnInterval > 0.0)
		{
			spawnTimer -= dt;

			while (spawnTimer <= 0.0)
			{
				spawnTimer += spawnInterval;

				const double life = kMaxParticles * spawnInterval;

				spawn(life);
			}
		}
		else
		{
			spawnTimer = 0.0;
		}

		for (int i = 0; i < kMaxParticles; ++i)
		{
			lt[i] -= dt;

			x[i] += vx[i] * dt;
			y[i] += vy[i] * dt;
			z[i] += vz[i] * dt;

			vy[i] += -10.f * dt;
		}
	}

	void draw() const
	{
		const float kMaxSize = 4.f;

		glEnable(GL_PROGRAM_POINT_SIZE);
		checkErrorGL();

		gxBegin(GL_POINTS);
		{
			for (int i = 0; i < kMaxParticles; ++i)
			{
				if (lt[i] > 0.f)
				{
					const float l = lt[i] * lr[i];

					const float size = std::sin(l * Calc::mPI) * ((6.f - std::cos(l * Calc::mPI * 7.f)) / 5.f) * kMaxSize;

					gxTexCoord2f(size, 0.f);
					gxVertex3f(x[i], y[i], z[i]);
				}
			}
		}
		gxEnd();

		glDisable(GL_PROGRAM_POINT_SIZE);
		checkErrorGL();

#if 0
		gxBegin(GL_LINES);
		{
			for (int i = 0, n = 0; i < kMaxParticles; ++i)
			{
				if (lt[i] > 0.f)
				{
					if ((n % 10) == 0)
					{
						gxVertex3f(0.f, 0.f, 0.f);
						gxVertex3f(x[i], y[i], z[i]);
					}

					n++;
				}
			}
		}
		gxEnd();
#endif
	}
};

struct VectorMemory
{
	const static int kMaxLines = 10000;

	struct Line
	{
		float lt, lr;
		float x1, y1, z1;
		float x2, y2, z2;
	};

	Line lines[kMaxLines];

	int lineAllocIndex;

	VectorMemory()
	{
		memset(this, 0, sizeof(*this));
	}

	void addLine(
		const float x1,
		const float y1,
		const float z1,
		const float x2,
		const float y2,
		const float z2,
		const float life)
	{
		Line & line = lines[lineAllocIndex];

		lineAllocIndex = (lineAllocIndex + 1) % kMaxLines;

		line.lt = life;
		line.lr = 1.f / life;

		line.x1 = x1;
		line.y1 = y1;
		line.z1 = z1;
		line.x2 = x2;
		line.y2 = y2;
		line.z2 = z2;
	}

	void tick(const float dt)
	{
		for (int i = 0; i < kMaxLines; ++i)
		{
			lines[i].lt -= dt;
		}
	}

	void draw() const
	{
		gxBegin(GL_LINES);
		{
			for (int i = 0; i < kMaxLines; ++i)
			{
				const Line & line = lines[i];

				if (line.lt > 0.f)
				{
					const float l = line.lt * line.lr;

					gxColor4f(l, l, l, l);

					gxVertex3f(line.x1, line.y1, line.z1);
					gxVertex3f(line.x2, line.y2, line.z2);
				}
			}
		}
		gxEnd();
	}
};

static void drawGrid(const int numQuadsX, const int numQuadsY)
{
	const float stepX = 1.f / (numQuadsX + 1);
	const float stepY = 1.f / (numQuadsY + 1);

	gxBegin(GL_QUADS);
	{
		for (int x = 0; x < numQuadsX; ++x)
		{
			for (int y = 0; y < numQuadsY; ++y)
			{
				const float x1 = (x + 0) * stepX - .5f;
				const float y1 = (y + 0) * stepY - .5f;
				const float x2 = (x + 1) * stepX - .5f;
				const float y2 = (y + 1) * stepY - .5f;

				gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
				gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
				gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
				gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
			}
		}
	}
	gxEnd();
}

static void drawExtrusion(const int numX, const int numY, const GLuint texture)
{
	const float stepX = 1.f / (numX + 1);
	const float stepY = 1.f / (numY + 1);
	const float sizeX = stepX * .45f;
	const float sizeY = stepY * .45f;
	
	const float time = framework.time * .2f;
	const Vec3 lightPosition = Vec3(std::sinf(time * 1.234f) * 200.f, 200.f + std::sinf(time * 2.345f) * 200.f, std::sinf(time * 3.456f) * 50.f - 250.f);
	const Vec3 lightDirection = Vec3(std::sinf(framework.time), std::cosf(framework.time), -2.f).CalcNormalized();

	gxPushMatrix();
	{
		gxLoadIdentity();
		gxTranslatef(lightPosition[0], lightPosition[1], lightPosition[2]);
		setBlend(BLEND_OPAQUE);
		setColor(colorWhite);
		for (int i = 0; i < 10; ++i)
		{
			drawCircle(0.f, 0.f, 25.f, 40);
			//gxTranslatef(0.f, 0.f, 5.f);
			gxRotatef(5.f, 1.f, 0.f, 0.f);
			gxRotatef(5.f, 0.f, 1.f, 0.f);
			gxRotatef(5.f, 0.f, 0.f, 1.f);
		}
	}
	gxPopMatrix();

	Shader shader("extrusion");
	shader.setImmediate("mode", 0);
	shader.setImmediate("lightPosition", lightPosition[0], lightPosition[1], lightPosition[2]);
	shader.setImmediate("lightDirection", lightDirection[0], lightDirection[1], lightDirection[2]);
	shader.setTexture("texture", 0, texture, true, true);
	setShader(shader);
	{
		gxBegin(GL_QUADS);
		{
			for (int x = 0; x < numX; ++x)
			{
				for (int y = 0; y < numY; ++y)
				{
					const float xMid = (x + .5f) * stepX;
					const float yMid = (y + .5f) * stepY;

					const float x1 = xMid - sizeX;
					const float y1 = yMid - sizeY;
					const float x2 = xMid + sizeX;
					const float y2 = yMid + sizeY;

					gxTexCoord2f(xMid, yMid);
					
					// top

					gxNormal3f(0.f, 0.f, -1.f);
					gxVertex3f(x1, y1, 1.f);
					gxVertex3f(x2, y1, 1.f);
					gxVertex3f(x2, y2, 1.f);
					gxVertex3f(x1, y2, 1.f);
				}
			}
		};
		gxEnd();
	}
	clearShader();

	shader.setImmediate("mode", 1);
	setShader(shader);
	{
		gxBegin(GL_QUADS);
		{
			for (int x = 0; x < numX; ++x)
			{
				for (int y = 0; y < numY; ++y)
				{
					const float xMid = (x + .5f) * stepX;
					const float yMid = (y + .5f) * stepY;

					const float x1 = xMid - sizeX;
					const float y1 = yMid - sizeY;
					const float x2 = xMid + sizeX;
					const float y2 = yMid + sizeY;

					gxTexCoord2f(xMid, yMid);

					// x-axis top
					gxNormal3f(0.f, +1.f, 0.f);
					gxVertex3f(x1, y1, 0.f);
					gxVertex3f(x2, y1, 0.f);
					gxVertex3f(x2, y1, 1.f);
					gxVertex3f(x1, y1, 1.f);

					// x-axis bottom
					gxNormal3f(0.f, -1.f, 0.f);
					gxVertex3f(x1, y2, 0.f);
					gxVertex3f(x2, y2, 0.f);
					gxVertex3f(x2, y2, 1.f);
					gxVertex3f(x1, y2, 1.f);

					// y-axis left
					gxNormal3f(+1.f, 0.f, 0.f);
					gxVertex3f(x1, y1, 0.f);
					gxVertex3f(x1, y2, 0.f);
					gxVertex3f(x1, y2, 1.f);
					gxVertex3f(x1, y1, 1.f);

					// y-axis right
					gxNormal3f(-1.f, 0.f, 0.f);
					gxVertex3f(x2, y1, 0.f);
					gxVertex3f(x2, y2, 0.f);
					gxVertex3f(x2, y2, 1.f);
					gxVertex3f(x2, y1, 1.f);

					// bottom

					gxNormal3f(0.f, 0.f, +1.f);
					gxVertex3f(x1, y1, 0.f);
					gxVertex3f(x2, y1, 0.f);
					gxVertex3f(x2, y2, 0.f);
					gxVertex3f(x1, y2, 0.f);
				}
			}
		};
		gxEnd();
	}
}

struct Camera
{
	Vec3 position;
	Vec3 rotation;

	Camera()
		: position()
		, rotation()
	{
	}

	void tick(const float dt)
	{
		rotation[0] -= mouse.dy / 100.f;
		rotation[1] -= mouse.dx / 100.f;

		if (gamepad[0].isConnected)
		{
			rotation[0] -= gamepad[0].getAnalog(1, ANALOG_Y) * dt;
			rotation[1] -= gamepad[0].getAnalog(1, ANALOG_X) * dt;
		}

		Mat4x4 mat;

		calculateTransform(0.f, 0.f, 0.f, mat);

		const Vec3 xAxis(mat(0, 0), mat(0, 1), mat(0, 2));
		const Vec3 zAxis(mat(2, 0), mat(2, 1), mat(2, 2));

		Vec3 direction;

		if (keyboard.isDown(SDLK_UP))
			direction += zAxis;
		if (keyboard.isDown(SDLK_DOWN))
			direction -= zAxis;
		if (keyboard.isDown(SDLK_LEFT))
			direction -= xAxis;
		if (keyboard.isDown(SDLK_RIGHT))
			direction += xAxis;

		if (gamepad[0].isConnected)
		{
			direction -= zAxis * gamepad[0].getAnalog(0, ANALOG_Y);
			direction += xAxis * gamepad[0].getAnalog(0, ANALOG_X);
		}

		const float speed = 200.f;

		position += direction * speed * dt;
	}

	void calculateTransform(const float eyeOffset, const float eyeX, const float eyeY, Mat4x4 & matrix) const
	{
		// todo : use the correct eye position when we're trying to do head mounted VR
		// right now the anatomy looks like this:
		//
		//      L----O----R
		//           |
		//           |
		//           |
		//
		// where L is the left eye, R is the right eye and O is where the head rotates around the axis
		// in real life, the head and eyes rotate a little more complicated..

		matrix = Mat4x4(true).Translate(position).Translate(eyeX, eyeY, 0.f).RotateY(rotation[1]).RotateX(rotation[0]).Translate(eyeOffset, 0.f, 0.f);
	}
};

struct Scene
{
	Camera camera;

	Shader modelShader;
	std::list<Model*> models;

	ParticleSystem ps;

	VectorMemory vm;

	MediaPlayer * mp;

	Scene()
		: camera()
		, modelShader("model-lit")
		, models()
		, ps()
		, vm()
		, mp(nullptr)
	{
		camera.position = Vec3(0.f, 170.f, -180.f);

		for (int x = -2; x <= +2; ++x)
		{
			for (int z = 0; z <= +2; ++z)
			{
				Model * model = new Model("model.txt");
				model->x = x * 300.f;
				model->y = 0.f;
				model->z = z * 300.f;
				model->scale = 0.f;
				model->overrideShader = &modelShader;
				model->animRootMotionEnabled = false;

				models.push_back(model);
			}
		}

		mp = new MediaPlayer();
		mp->openAsync("../colorpart/roar.mpg", false);
		//mp->openAsync("../colorpart/haelos.mpg", false);
		mp->presentTime = 0.0;
	}

	~Scene()
	{
		delete mp;
		mp = nullptr;

		for (auto model : models)
		{
			delete model;
			model = nullptr;
		}

		models.clear();
	}

	void tick(const float dt);
	void draw(Surface * surface, const float eyeOffset, const float eyeX, const float eyeY) const;
};

static Scene * scene = nullptr;

void Scene::tick(const float dt)
{
	camera.tick(dt);

	//

	for (Model * model : models)
	{
		model->axis = Vec3(0.f, -1.f, 0.f);
		model->angle += -.1f * dt;

		model->angle += mouse.dx / 1000.f;

		if (!model->animIsActive)
		{
			model->scale -= model->animSpeed * 2.f * dt;

			if (model->scale < 0.f)
				model->scale = 0.f;

			if (model->scale == 0.f)
			{
				const auto animList = model->getAnimList();

				model->startAnim(animList[rand() % animList.size()].c_str());
				model->animSpeed = .2f;
			}
		}
		else
		{
			model->scale += model->animSpeed * 3.f * dt;

			if (model->scale > 1.f)
				model->scale = 1.f;
		}
	}

	//

	ps.tick(dt);

	//

	vm.tick(dt);
	
	static double _t = 0.0;
	const double t[2] = { _t, _t + dt };
	for (int j = 0; j < 8; ++j)
	{
		double x[2];
		double y[2];
		double z[2];

		for (int i = 0; i < 2; ++i)
		{
			double s = (j + 1) / 8.0;

			x[i] = std::sin(t[i] / 1.234 * 10.0 * s) * 100.0;
			y[i] = std::sin(t[i] / 1.345 * 1.0  * s) * 400.0 + 200.0;
			z[i] = std::sin(t[i] / 1.456 * 10.0 * s) * 100.0;
		}
		
		vm.addLine(x[0], y[0], z[0], x[1], y[1], z[1], 10.f);
	}
	_t  = t[1];
	for (int i = 0; i < 4 * 0; ++i)
	{
		vm.addLine(
			random(-1000.f, +1000.f),
			random(-1000.f, +1000.f),
			random(-1000.f, +1000.f),
			random(-1000.f, +1000.f),
			random(-1000.f, +1000.f),
			random(-1000.f, +1000.f),
			4.f);
	}

	//

	mp->tick(mp->context);

	//mp->presentTime += framework.timeStep * .25f;
	mp->presentTime += framework.timeStep;
}

void Scene::draw(Surface * surface, const float eyeOffset, const float eyeX, const float eyeY) const
{
	pushSurface(surface);
	{
		surface->clear(0, 0, 0, 0);
		surface->clearDepth(1.f);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		Mat4x4 matP;
		matP.MakePerspectiveLH(Calc::DegToRad(60.f), surface->getHeight() / float(surface->getWidth()), .1f, 10000.f);

		Mat4x4 matC(true);
		camera.calculateTransform(68.f/10.f/2.f * eyeOffset, eyeX, eyeY, matC);
		matC = matC.Invert();

		gxMatrixMode(GL_PROJECTION);
		gxPushMatrix();
		{
			gxLoadMatrixf(matP.m_v);
			gxMultMatrixf(matC.m_v);

			gxMatrixMode(GL_MODELVIEW);
			gxPushMatrix();
			{
				gxLoadIdentity();

				if (keyboard.isDown(SDLK_l))
				{
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					checkErrorGL();
				}

				for (const Model * model : models)
				{
					gxPushMatrix();
					{
						setBlend(BLEND_OPAQUE);
						setColor(colorWhite);
						model->draw();

						clearShader();

						if (true)
						{
							Mat4x4 mat;
							model->calculateTransform(mat);
							gxMultMatrixf(mat.m_v);

							setBlend(BLEND_OPAQUE);
							setColor(colorWhite);
							gxBegin(GL_QUADS);
							{
								const float s = 70.f;

								gxVertex3f(-s, 0.f, -s);
								gxVertex3f(+s, 0.f, -s);
								gxVertex3f(+s, 0.f, +s);
								gxVertex3f(-s, 0.f, +s);
							}
							gxEnd();
						}
					}
					gxPopMatrix();
				}

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				checkErrorGL();

				//

				{
					for (int i = 0; i < 2; ++i)
					{
						gxPushMatrix();
						{
							gxTranslatef(0.f, 50.f - i * 100.f, 0.f);
							gxRotatef(90.f, -1.f, 0.f, 0.f);

							float scale = 1500.f;
							if (i == 1)
								scale *= 2.f;
							gxScalef(scale, scale, scale);

							const bool wireMode = (i == 0);

							glPolygonMode(GL_FRONT_AND_BACK, wireMode ? GL_LINE : GL_FILL);
							checkErrorGL();

							setBlend(BLEND_OPAQUE);
							Shader shader("waves");
							shader.setImmediate("time", framework.time);
							shader.setImmediate("mode", wireMode ? 1 : 0);
							shader.setTexture("texture", 0, getTexture("tile2.jpg"), true, true);
							setShader(shader);
							{
								setColor(colorWhite);
								drawGrid(40, 40);
							}
							clearShader();

							glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
							checkErrorGL();
						}
						gxPopMatrix();
					}

					//

					{
						gxPushMatrix();
						{
							//gxTranslatef(0.f, 600.f, 0.f);
							gxTranslatef(0.f, 250.f, -400.f);
							//gxRotatef(-90.f, -1.f, 0.f, 0.f);

							gxRotatef(std::sinf(framework.time * .123f) * 30.f, 0.f, 1.f, 0.f);
							//gxRotatef(std::sinf(framework.time * .245f) * 30.f, 1.f, 0.f, 0.f);

							const float scaleX = 1600.f/2.f;
							const float scaleY = 1000.f/2.f;
							const float scaleZ = 50.f;
							gxScalef(scaleX, scaleY, scaleZ);

							const int numX = int(std::round(scaleX / 60.f));
							const int numY = int(std::round(scaleY / 60.f));

							setBlend(BLEND_OPAQUE);

							const GLuint texture = mp->getTexture();

							if (texture != 0)
							{
								//drawExtrusion(20, 20, getTexture("tile1.jpg"));
								drawExtrusion(numX, numY, texture);
							}
						}
						gxPopMatrix();
					}

					//

					{
						glDepthMask(false);

						setBlend(BLEND_ADD);
						setColor(127, 127, 127);
						Shader shader("particles");
						shader.setTexture("texture", 0, getTexture("particle.jpg"), true, true);
						setShader(shader);
						{
							//ps.draw();
						}
						clearShader();

						glDepthMask(true);
					}

					//

					{
						glDepthMask(false);

						setBlend(BLEND_ADD);
						setColor(127, 127, 127);
						vm.draw();

						glDepthMask(true);
					}
				}
			}
			gxMatrixMode(GL_MODELVIEW);
			gxPopMatrix();
		}
		gxMatrixMode(GL_PROJECTION);
		gxPopMatrix();

		glDisable(GL_DEPTH_TEST);

		setBlend(BLEND_ALPHA);
		setColor(colorWhite);

		//setFont("calibri.ttf");
		//drawText(50.f, 50.f, 48, +1.f, +1.f, "%s : %04.2fs", model->getAnimName(), model->animTime);
	}
	popSurface();

	if (false)
	{
		Shader shader("chromo.ps", "effect.vs", "chromo.ps");
		shader.setTexture("colormap", 0, surface->getTexture(), true, true);
		setBlend(BLEND_OPAQUE);
		surface->postprocess(shader);
	}
}

#if ENABLE_VIDEO_FOR_WINDOWS

struct VideoCapture
{
	enum HicState
	{
		kHicState_Initial,
		kHicState_Created,
		kHicState_Ready,
		kHicState_Failed
	};

	CAPSTATUS status;

	int videoSx;
	int videoSy;
	DWORD videoFormat;

	BITMAPINFOHEADER srcFormat;
	BITMAPINFOHEADER dstFormat;

	HIC hic;
	HicState hicState;

	uint8_t * decompressBuffer;
	int decompressCount;

	VideoCapture()
	{
		memset(this, 0, sizeof(*this));
	}

	~VideoCapture()
	{
		delete decompressBuffer;
		decompressBuffer = nullptr;
	}

	static int calculateStride(const int sx, const int bitDepth)
	{
		return (((sx * bitDepth) + 31) & ~31) >> 3;
	}

	void allocateDecompressBuffer()
	{
		decompressBuffer = new uint8_t[dstFormat.biSizeImage];
	}
};

static LRESULT PASCAL frameCallback(HWND window, VIDEOHDR * videoHeader)
{
	VideoCapture * self = (VideoCapture*)capGetUserData(window);

	if (window == 0)
		return FALSE;
	else
	{
		if (self->hicState == VideoCapture::kHicState_Initial)
		{
			self->hic = ICOpen(MAKEFOURCC('V','I','D','C'), self->videoFormat, ICMODE_DECOMPRESS);

			if (self->hic == 0)
			{
				self->hicState = VideoCapture::kHicState_Failed;
			}
			else
			{
				self->hicState = VideoCapture::kHicState_Created;
			}
		}

		if (self->hicState == VideoCapture::kHicState_Created)
		{
			BITMAPINFO srcFormat;

			memset(&srcFormat, 0, sizeof(srcFormat));
			if (capGetVideoFormat(window, &srcFormat, sizeof(srcFormat)) == 0)
			{
				self->hicState = VideoCapture::kHicState_Failed;
			}
			else
			{
				BITMAPINFOHEADER dstFormat;
				memset(&dstFormat, 0, sizeof(dstFormat));
				dstFormat.biSize = sizeof(dstFormat);
				dstFormat.biWidth = srcFormat.bmiHeader.biWidth;
				dstFormat.biHeight = srcFormat.bmiHeader.biHeight;
				dstFormat.biBitCount = 24;
				dstFormat.biPlanes = 1;
				dstFormat.biCompression = BI_RGB;

				const DWORD dstStride = VideoCapture::calculateStride(dstFormat.biWidth, dstFormat.biBitCount);
				dstFormat.biSizeImage = dstFormat.biHeight * dstStride;

				self->srcFormat = srcFormat.bmiHeader;
				self->dstFormat = dstFormat;

				if (ICDecompressBegin(self->hic, &self->srcFormat, &self->dstFormat) != ICERR_OK)
				{
					self->hicState = VideoCapture::kHicState_Failed;
				}
				else
				{
					self->allocateDecompressBuffer();

					self->hicState = VideoCapture::kHicState_Ready;
				}
			}
		}

		if (self->hicState == VideoCapture::kHicState_Ready)
		{
			const uint64_t time1 = g_TimerRT.TimeUS_get();

			if (ICDecompress(self->hic, ICDECOMPRESS_HURRYUP, &self->srcFormat, videoHeader->lpData, &self->dstFormat, self->decompressBuffer) == ICERR_OK)
			{
				//logDebug("decompress success! %d", self->decompressCount++);
			}

			const uint64_t time2 = g_TimerRT.TimeUS_get();

			logDebug("decompress took %fms", (time2 - time1) / 1000.f);

			//ICClose(hic);
		}

		return TRUE;
	}
}

#endif

#if ENABLE_MOTION_DETECTION
static bool detectFacePosition_MovementDelta(
	const uint8_t * __restrict curPixels,
	      uint8_t * __restrict oldPixels,
	short * __restrict deltaPixels,
	const int sx, const int sy,
	double & faceX, double & faceY)
{
	// note : assumes gray scale input images

	const int bufferSize = sx * sy;

	// compute delta image

	for (int i = 0; i < bufferSize; ++i)
	{
		deltaPixels[i] = curPixels[i] - oldPixels[i];

		oldPixels[i] = curPixels[i];
	}

	// analyze delta image

	double totalValue = 0.0;
	double totalX = 0.0;
	double totalY = 0.0;

	const double normValue = 1.0 / 255.0;

	for (int y = 0; y < sy; ++y)
	{
		const short * __restrict line = deltaPixels + (y * sx);

		for (int x = 0; x < sx; ++x)
		{
			const double delta = std::pow(std::abs(line[x]) * normValue, 2.0);

			totalValue += delta;
			totalX += x * delta;
			totalY += y * delta;
		}
	}

	if (totalValue > 0.0)
	{
		const double maxDelta = sx * sy;

		const double moveAmount = Calc::Min(1.0, totalValue / maxDelta * 100.0);
		//const double moveAmount = 1.0;

		faceX = Calc::Lerp(faceX, totalX / totalValue, moveAmount);
		faceY = Calc::Lerp(faceY, totalY / totalValue, moveAmount);
	}

	return true;
}
#endif

#if ENABLE_FACE_DETECTION
static bool detectFacePosition(const uint8_t * pixels, const int sx, const int sy, const int pixelStride, int & faceX, int & faceY)
{
	int64_t totalValue = 0;
	int64_t totalX = 0;
	int64_t totalY = 0;

	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict line = pixels + (y * sx * pixelStride);

		for (int x = 0; x < sx; ++x)
		{
			const int r = line[x * pixelStride + 0];
			const int g = line[x * pixelStride + 1];
			const int b = line[x * pixelStride + 2];

			const int lumi = r * 2 + g - b / 2;

			//const int64_t lumi = line[x * pixelStride + 0];// + line[x].g + line[x].b;

			totalValue += lumi;
			totalX += x * lumi;
			totalY += y * lumi;
		}
	}

	faceX = int(totalX / totalValue);
	faceY = int(totalY / totalValue);

	return true;
}
#endif

int main(int argc, char * argv[])
{
	//changeDirectory("data");

#if !defined(DEBUG) && 1
	framework.fullscreen = true;
	framework.exclusiveFullscreen = false;
	framework.useClosestDisplayMode = true;
#endif

#if defined(DEBUG)
	framework.enableRealTimeEditing = true;
	framework.minification = 2;
#endif

	framework.enableDepthBuffer = true;

	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
#if ENABLE_VIDEO_FOR_WINDOWS
		VideoCapture videoCapture;

		char szDeviceName[80];
		char szDeviceVersion[80];

		int deviceIndex = -1;

		for (int i = 0; i < 10; ++i)
		{
			if (capGetDriverDescription(
				i,
				szDeviceName,
				sizeof(szDeviceName),
				szDeviceVersion,
				sizeof(szDeviceVersion)) == TRUE)
			{
				logDebug("device: %d: %s", i, szDeviceName);

				if (deviceIndex < 0)
				{
					deviceIndex = i;
				}
			}
		}

		if (deviceIndex >= 0)
		{
			//const HWND window = capCreateCaptureWindow("Capture Window", (WS_VISIBLE * 1) | WS_POPUP | WS_CHILD, 0, 0, 800, 600, 0, 0);
			const HWND window = capCreateCaptureWindow("Capture Window", 0, 0, 0, 800, 600, 0, 0);

			const BOOL result = capDriverConnect(window, deviceIndex);

			if (result == TRUE)
			{
				CAPDRIVERCAPS caps;
				memset(&caps, 0, sizeof(caps));
				if (capDriverGetCaps(window, &caps, sizeof(caps)) == TRUE)
				{
					logDebug("got driver caps!");
				}

				CAPSTATUS status;
				memset(&status, 0, sizeof(status));
				if (capGetStatus(window, &status, sizeof(status)) == TRUE)
				{
					logDebug("got driver status!");
				}

				SetWindowPos(window, NULL, 0, 0,
					status.uiImageWidth, 
					status.uiImageHeight,
					SWP_NOZORDER | SWP_NOMOVE); 

				//

				if (false)
				{
					if (caps.fHasDlgVideoSource)
						capDlgVideoSource(window); 

					if (caps.fHasDlgVideoFormat) 
					{
						capDlgVideoFormat(window); 
						capGetStatus(window, &status, sizeof (CAPSTATUS));
					} 

					if (caps.fHasDlgVideoDisplay)
						capDlgVideoDisplay(window);
				}

				//

				// todo : set this pointer and check return value
				capSetUserData(window, &videoCapture);

				// todo : check return value
				capSetCallbackOnFrame(window, frameCallback);
				//capSetCallbackOnVideoStream(window, frameCallback);

				CAPTUREPARMS params;
				memset(&params, 0, sizeof(params));
				if (capCaptureGetSetup(window, &params, sizeof(params)) == TRUE)
				{
					params.dwRequestMicroSecPerFrame = 1000000/30; // 60 fps
					// todo : check return value;
					capCaptureSetSetup(window, &params, sizeof(params));
				}

				// todo : check what these do
				//capPreviewScale(window, FALSE);
				//capPreviewRate(window, 0);
				capPreview(window, FALSE);

				//capCaptureSequenceNoFile(window);

				const DWORD formatSize = capGetVideoFormatSize(window);

				if (formatSize > 0)
				{
					uint8_t * formatBuffer = new uint8_t[formatSize];

					if (capGetVideoFormat(window, formatBuffer, formatSize) == formatSize)
					{
						const BITMAPINFOHEADER & header = ((BITMAPINFO*)formatBuffer)->bmiHeader;

						videoCapture.videoSx = header.biWidth;
						videoCapture.videoSy = header.biHeight;
						videoCapture.videoFormat = header.biCompression;
					}

					delete[] formatBuffer;
					formatBuffer = nullptr;
				}

				while (!framework.quitRequested)
				{
					framework.process();

					const uint64_t time1 = g_TimerRT.TimeUS_get();
					//capGrabFrame(window);
					capGrabFrameNoStop(window);
					const uint64_t time2 = g_TimerRT.TimeUS_get();
					logDebug("capGrabFrameNoStop took %fms", (time2 - time1) / 1000.f);

					framework.beginDraw(0, 0, 0, 0);
					{
						const int x = rand() % GFX_SX;
						const int y = rand() % GFX_SY;

						fillCircle(x, y, 100, 100);
					}
					framework.endDraw();
				}
			}

			capSetCallbackOnFrame(window, nullptr);
			capDriverDisconnect(window);
			DestroyWindow(window);
		}
#endif

#if ENABLE_MEDIA_FOUNDATION
		//CoInitializeEx(0, COINIT_MULTITHREADED);
		//MFStartup(MF_VERSION);

		IMFAttributes * mfAttributes = nullptr;

		if (MFCreateAttributes(&mfAttributes, 0) == S_OK)
		{
			if (mfAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID) == S_OK)
			{
				IMFActivate ** deviceSources = nullptr;
				UINT32 deviceSourceCount = 0;

				if (MFEnumDeviceSources(mfAttributes, &deviceSources, &deviceSourceCount) == S_OK)
				{
					for (UINT32 i = 0; i < deviceSourceCount; ++i)
					{
						WCHAR * friendlyName = nullptr;
						UINT32 friendlyNameSize = 0;

						if (deviceSources[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendlyName, &friendlyNameSize) == S_OK)
						{
							logDebug("capture device: %s", friendlyName);
						}

						/*
						WCHAR * symbolicLink = nullptr;
						UINT32 symbolicLinkSize = 0;

						if (deviceSources[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symbolicLink, &symbolicLinkSize) == S_OK)
						{
							logDebug("capture device symlink: %s", symbolicLink);
						}
						*/

						IMFMediaSource * mediaSource = nullptr;

						if (deviceSources[i]->ActivateObject(IID_PPV_ARGS(&mediaSource)) == S_OK)
						{
							IMFSourceReader * sourceReader = nullptr;

							if (MFCreateSourceReaderFromMediaSource(mediaSource, mfAttributes, &sourceReader) == S_OK)
							{
								DWORD streamIndex = 0;

								BOOL isStreamSelected = FALSE;
								if (sourceReader->GetStreamSelection(streamIndex, &isStreamSelected) == S_OK)
								{
									logDebug("isStreamSelected: %d", int(isStreamSelected));
								}

								IMFMediaType * mediaType = nullptr;

								for (int mediaTypeIndex = 0; true; ++mediaTypeIndex)
								{
									if (sourceReader->GetNativeMediaType(streamIndex, mediaTypeIndex, &mediaType) == S_OK)
									{
										break;

										mediaType->Release();
										mediaType = nullptr;
									}
									else
									{
										// todo : check if error or MF_E_NO_MORE_TYPES
										break;
									}
								}

								IMFMediaType * outputMediaType = nullptr;
								if (MFCreateMediaType(&outputMediaType) == S_OK)
								{
									if (mediaType != nullptr)
									{
										mediaType->CopyAllItems(outputMediaType);
										mediaType->Release();
									}

									outputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
									//outputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB8);
									//outputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
									//outputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);
#if 0
									//outputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlaceMode);
									MFSetAttributeSize(outputMediaType, MF_MT_FRAME_SIZE, 320, 240);
									outputMediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, 0);

									UINT32 imageSize;
									MFCalculateImageSize(MFVideoFormat_RGB8, 320, 240, &imageSize);

									outputMediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, true);
									outputMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, true);
									MFSetAttributeRatio(outputMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
#endif

									//if (sourceReader->SetCurrentMediaType(streamIndex, nullptr, outputMediaType) == S_OK)
									{
										logDebug("success!");

										for (;;)
										{
											DWORD actualStreamIndex = 0;
											DWORD streamFlags = 0;
											LONGLONG timeStamp = 0;

											IMFSample * sample = nullptr;

											if (sourceReader->ReadSample(streamIndex, 0, &actualStreamIndex, &streamFlags, &timeStamp, &sample) == S_OK)
											{
												if (streamFlags & MF_SOURCE_READERF_ERROR)
												{
													logDebug("msg: error");
												}

												if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
												{
													logDebug("msg: end of stream");
												}

												if (streamFlags & MF_SOURCE_READERF_NEWSTREAM)
												{
													logDebug("msg: new stream");
												}

												if (streamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
												{
													logDebug("msg: current media type changed");
												}

												if (sample != nullptr)
												{
													logDebug("got sample!");

													sample->Release();
													sample = nullptr;
												}
											}
										}
									}
								}

								sourceReader->Release();
								sourceReader = nullptr;
							}

							mediaSource->Release();
							mediaSource = nullptr;
						}
					}

					for (UINT32 i = 0; i < deviceSourceCount; ++i)
					{
						deviceSources[i]->Release();
					}

					if (deviceSources != nullptr)
					{
						CoTaskMemFree(deviceSources);
						deviceSources = nullptr;
					}

					deviceSourceCount = 0;
				}
			}

			mfAttributes->Release();
			mfAttributes = nullptr;
		}
#endif

#if ENABLE_VIDEOIN
		videoInput * VI = new videoInput();

		const int numDevices = VI->listDevices();	
		const int deviceIndex = 0;
		
		//VI->setIdealFramerate(deviceIndex, 60);	
		//VI->setupDevice(deviceIndex, 320, 240);
		VI->setupDevice(deviceIndex, 160/2, 120/2);

		const int videoSx = VI->getWidth(deviceIndex);
		const int videoSy = VI->getHeight(deviceIndex);
		const int videoBufferSize = VI->getSize(deviceIndex);
		uint8_t * videoBuffer = new unsigned char[videoBufferSize];
#endif

	#if ENABLE_FACE_DETECTION
		ImageData * face = loadImage("face.jpg");

		while (!keyboard.isDown(SDLK_SPACE))
		{
			framework.process();

			//

			int faceX;
			int faceY;

			if (detectFacePosition((uint8_t*)face->getLine(0), face->sx, face->sy, 4, faceX, faceY))
			{
				logDebug("weighted pos: %d, %d", faceX, faceY);
			}

			//

			framework.beginDraw(0, 0, 0, 0);
			{
				gxBegin(GL_POINTS);
				{
					for (int y = 0; y < face->sy; y += 5)
					{
						const ImageData::Pixel * __restrict line = face->getLine(y);

						for (int x = 0; x < face->sx; x += 5)
						{
							gxColor3ub(line[x].r, line[x].g, line[x].b);
							gxVertex2f(x, y);
						}
					}
				}
				gxEnd();

				setColor(colorWhite);
				fillCircle(faceX, faceY, 10.f, 20);
			}
			framework.endDraw();
		}
	#endif

	#if ENABLE_VIOLA_JONES
		// load image

		ImageData * image = loadImage("face.jpg");

		// face detection over image

		struct Detection
		{
			int x;
			int y;
			int baseResolution;
		};

		std::vector<Detection> detections;

		// load cascade classifier model
		
		const char * modelfile = "haar-face.txt";
		const float strictness = 1.f;
		const float fincrement = .1f;
		const float fscale = 1.25f;

		CascadeClassifier * classifier = new CascadeClassifier(modelfile);
		classifier->strictness(strictness);

		// convert RGB image to grayscale
		float * __restrict grayImage = new float[image->sx * image->sy];
		for (int y = 0; y < image->sy; ++y)
		{
			const ImageData::Pixel * __restrict srcLine = image->getLine(y);
			float * __restrict dstLine = grayImage + y * image->sx;

			for (int x = 0; x < image->sx; ++x)
			{
				*dstLine = 
					.21f * srcLine->r +
					.71f * srcLine->g +
					.07f * srcLine->b;

				srcLine++;
				dstLine++;
			}
		}

		// calculate integral image and squared integral image

		const float * iimg = integral_image(grayImage, image->sx, image->sy);
		const float * siimg = squared_integral_image(grayImage, image->sx, image->sy);

		delete[] grayImage;
		grayImage = nullptr;

		// run face detection on multiple scales

		int fnotfound = 0;

		int base_resolution = classifier->getBaseResolution();

		while (base_resolution <= image->sx && base_resolution <= image->sy)
		{
			const int increment = Calc::Max(1, int(base_resolution * fincrement));

			// slide window over image

			for (int i = 0; (i+base_resolution) <= image->sx; i += increment)
			{
				for (int j = 0; (j+base_resolution) <= image->sy; j += increment)
				{
					// calculate mean and std. deviation for current window

					const float mean = evaluate_integral_rectangle(
						iimg, image->sx, i, j, base_resolution, base_resolution) / pow(base_resolution, 2);

					const float stdev = sqrt((evaluate_integral_rectangle(siimg, image->sx, i, j, base_resolution, base_resolution) / pow(base_resolution, 2)) - pow(mean, 2));

					// classify window (post-normalization of feature values using mean and stdev)

					if (classifier->classify(iimg, image->sx, i, j, mean, stdev) == true)
					{
						Detection detection;
						detection.x = i;
						detection.y = j;
						detection.baseResolution = base_resolution;

						detections.push_back(detection);
					}
					else
					{
						fnotfound++;
					}
				}
			}

			classifier->scale(fscale);
			base_resolution = classifier->getBaseResolution();
		}

		// Merge overlapping detections
		//merge_detections(detections);
	#endif

		mouse.showCursor(false);
		mouse.setRelative(true);

		Surface * surface = new Surface(GFX_SX, GFX_SY, false);
		Surface * surfaceL = new Surface(GFX_SX, GFX_SY, false, true);
		Surface * surfaceR = new Surface(GFX_SX, GFX_SY, false, true);

		Pisu pisu;
		
		scene = new Scene();

	#if ENABLE_VIDEOIN
		GLuint videoTexture = 0;
	#endif

	#if ENABLE_MOTION_DETECTION
		uint8_t * curPixels = new uint8_t[videoSx * videoSy];
		uint8_t * oldPixels = new uint8_t[videoSx * videoSy];
		short * deltaPixels = new short[videoSx * videoSy];
	#endif

	#if ENABLE_MOTION_DETECTION || ENABLE_FACE_DETECTION
		double faceX = 0.0;
		double faceY = 0.0;
		bool hasFacePosition = false;
	#endif

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			const float dt = framework.timeStep;

			scrollX += dt * 200.f;

			pisu.tick(dt);

			scene->tick(dt);

			// todo : colour switch background or line style after x amount of time

			framework.beginDraw(0, 0, 0, 0);
			{
			#if 0
				pushSurface(surface);
				{
					surface->clear(0, 0, 0, 0);
					const float m = .999f;
					setBlend(BLEND_MUL);
					setColorf(m, m, m, m);
					//drawRect(0, 0, surface->getWidth(), surface->getHeight());

					// draw path

					setBlend(BLEND_ALPHA);
					//setColor(colorWhite);
					setColor(Color::fromHSL(int((framework.time * .1f) * 6.f) / 6.f, 1.f, .5f));

				#if 1
					gxBegin(GL_LINES);
					{
						for (int x = 0; x < GFX_SX; ++x)
						{
							const float h1 = calculateHeight(scrollX + x + 0.f);
							const float h2 = calculateHeight(scrollX + x + 1.f);

							gxVertex2f(x + 0, h1);
							gxVertex2f(x + 1, h2);
						}
					}
					gxEnd();
				#else
					gxBegin(GL_LINES);
					{
						const float step = .01f;

						for (float t = 0.f; t < numNodes; t += step)
						{
							const Vec2F p1 = path.Interpolate(t + step * 0.f);
							const Vec2F p2 = path.Interpolate(t + step * 1.f);

							gxVertex2f(p1[0], p1[1]);
							gxVertex2f(p2[0], p2[1]);
						}
					}
					gxEnd();
			#endif

					pisu.draw();
				}
				popSurface();
			#endif

				const float faceMovementAmount = 50.f;

			#if ENABLE_FACE_DETECTION
				const float eyeX = - (faceX/videoSx - 0.5) * faceMovementAmount;
				const float eyeY = + (faceY/videoSy - 0.5) * faceMovementAmount;
			#else
				const float eyeX = 0.f;
				const float eyeY = 0.f;
			#endif

				scene->draw(surfaceL, -1.f, eyeX, eyeY);
				scene->draw(surfaceR, +1.f, eyeX, eyeY);

				if (true)
				{
					const GLuint colormapL = surfaceL->getTexture();
					const GLuint colormapR = surfaceR->getTexture();

					pushSurface(surface);
					{
						Shader shader("anaglyph.ps", "effect.vs", "anaglyph.ps");
						shader.setTexture("colormapL", 0, colormapL, true, false);
						shader.setTexture("colormapR", 1, colormapR, true, false);
						shader.setImmediate("mode",
							keyboard.isDown(SDLK_1) ? 1 :
							keyboard.isDown(SDLK_2) ? 2 :
							0);
						setShader(shader);
						{
							setBlend(BLEND_OPAQUE);
							drawRect(0.f, 0.f, GFX_SX, GFX_SY);
						}
						clearShader();
					}
					popSurface();
				}

			#if ENABLE_VIDEOIN
				if (true)
				{
					if (VI->isFrameNew(deviceIndex))
					{
						if (VI->getPixels(deviceIndex, videoBuffer, true, false))
						{
						#if ENABLE_MOTION_DETECTION
							for (int y = 0; y < videoSy; ++y)
							{
								const uint8_t * __restrict srcLine = videoBuffer + (y * videoSx * 3);
								uint8_t * __restrict curLine = curPixels + y * videoSx;

								for (int x = 0; x < videoSx; ++x)
								{
									const int r = srcLine[0];
									const int g = srcLine[1];
									const int b = srcLine[2];

									srcLine += 3;

									const int lumi = (r + g * 2 + b) >> 2;

									curLine[x] = lumi;
								}
							}

							if (detectFacePosition_MovementDelta(
								curPixels,
								oldPixels,
								deltaPixels,
								videoSx,
								videoSy,
								faceX,
								faceY))
							{
								hasFacePosition = true;
							}
						#elif ENABLE_FACE_DETECTION
							if (detectFacePosition(videoBuffer, videoSx, videoSy, 3, faceX, faceY))
							{
								hasFacePosition = true;
							}
						#endif

							//

							if (videoTexture != 0)
							{
								glDeleteTextures(1, &videoTexture);
								videoTexture = 0;
							}

							videoTexture = createTextureFromRGB8(videoBuffer, videoSx, videoSy, false, true);
						}
					}

					pushSurface(surface);
					{
						const float scaleX = GFX_SX/5.f / float(videoSx);
						const float scaleY = GFX_SY/5.f / float(videoSy);
						const float scale = std::min(scaleX, scaleY);

						gxScalef(scale, scale, scale);

						if (videoTexture != 0)
						{
							gxSetTexture(videoTexture);
							{
								drawRect(0, 0, videoSx, videoSy);
							}
							gxSetTexture(0);
						}

						if (hasFacePosition)
						{
							setColor(colorWhite);
							fillCircle(faceX, videoSy - faceY, 10 / scale, 100);
						}
					}
					popSurface();
				}
			#endif

				gxSetTexture(surface->getTexture());
				{
					setBlend(BLEND_OPAQUE);
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}
				gxSetTexture(0);
			}
			framework.endDraw();
		}

	#if ENABLE_MOTION_DETECTION
		delete[] curPixels;
		curPixels = nullptr;

		delete[] oldPixels;
		oldPixels = nullptr;

		delete[] deltaPixels;
		deltaPixels = nullptr;
	#endif

	#if ENABLE_VIDEOIN
		if (videoTexture != 0)
		{
			glDeleteTextures(1, &videoTexture);
			videoTexture = 0;
		}
	#endif

		delete scene;
		scene = nullptr;

		delete surfaceL;
		surfaceL = nullptr;

		delete surfaceR;
		surfaceR = nullptr;

		delete surface;
		surface = nullptr;

	#if ENABLE_VIDEOIN
		delete[] videoBuffer;
		videoBuffer = nullptr;

		VI->stopDevice(deviceIndex);

		delete VI;
		VI = nullptr;
	#endif
		
		framework.shutdown();
	}

	return 0;
}
