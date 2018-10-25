#include "Calc.h"
#include "framework.h"
#include "Timer.h"
#include "video.h"
#include <cmath>
#include <list>

#include <array>

#define ENABLE_VIDEOIN 0

#define GFX_SX 1920
#define GFX_SY 1080

#if ENABLE_VIDEOIN
	#include "videoin.h"
#endif

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
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		
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
		
		glDisable(GL_LINE_SMOOTH);
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
		pushBlend(BLEND_OPAQUE);
		setColor(colorWhite);
		for (int i = 0; i < 10; ++i)
		{
			drawCircle(0.f, 0.f, 25.f, 40);
			//gxTranslatef(0.f, 0.f, 5.f);
			gxRotatef(5.f, 1.f, 0.f, 0.f);
			gxRotatef(5.f, 0.f, 1.f, 0.f);
			gxRotatef(5.f, 0.f, 0.f, 1.f);
		}
		
		popBlend();
	}
	gxPopMatrix();

	Shader shader("extrusion");
	shader.setImmediate("mode", 0);
	shader.setImmediate("lightPosition", lightPosition[0], lightPosition[1], lightPosition[2]);
	shader.setImmediate("lightDirection", lightDirection[0], lightDirection[1], lightDirection[2]);
	shader.setTexture("source", 0, texture, true, true);
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
	Camera3d controllerCamera;

	Camera()
		: controllerCamera()
	{
		controllerCamera.maxForwardSpeed = 200.f;
		controllerCamera.maxStrafeSpeed = 200.f;
	}

	void tick(const float dt)
	{
		controllerCamera.tick(dt, true);
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

		matrix = Mat4x4(true)
			.Translate(controllerCamera.position)
			.Translate(eyeX, eyeY, 0.f)
			.RotateY(controllerCamera.yaw * M_PI / 180.f)
			.RotateX(controllerCamera.pitch * M_PI / 180.f)
			.Translate(eyeOffset, 0.f, 0.f).Scale(1, -1, 1);
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
		camera.controllerCamera.position = Vec3(0.f, 170.f, -180.f);

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
		mp->openAsync("../colorpart/roar.mpg", MP::kOutputMode_RGBA);
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

	mp->tick(mp->context, true);
	
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
						pushBlend(BLEND_OPAQUE);
						setColor(colorWhite);
						model->draw();
						popBlend();

						clearShader();

						if (true)
						{
							Mat4x4 mat;
							model->calculateTransform(mat);
							gxMultMatrixf(mat.m_v);

							pushBlend(BLEND_OPAQUE);
							setColor(colorWhite);
							gxSetTexture(getTexture("tile2.jpg"));
							gxBegin(GL_QUADS);
							{
								const float s = 70.f;

								gxTexCoord2f(0.f, 0.f); gxVertex3f(-s, 0.f, -s);
								gxTexCoord2f(1.f, 0.f); gxVertex3f(+s, 0.f, -s);
								gxTexCoord2f(1.f, 1.f); gxVertex3f(+s, 0.f, +s);
								gxTexCoord2f(0.f, 1.f); gxVertex3f(-s, 0.f, +s);
							}
							gxEnd();
							gxSetTexture(0);
							popBlend();
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

							pushBlend(BLEND_OPAQUE);
							Shader shader("waves");
							shader.setImmediate("time", framework.time);
							shader.setImmediate("mode", wireMode ? 1 : 0);
							shader.setTexture("source", 0, getTexture("tile2.jpg"), true, true);
							setShader(shader);
							{
								setColor(colorWhite);
								drawGrid(40, 40);
							}
							clearShader();
							popBlend();

							glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
							checkErrorGL();
						}
						gxPopMatrix();
					}

					//

					{
						gxPushMatrix();
						{
							const float scaleX = 1600.f/2.f;
							const float scaleY = 1000.f/2.f;
							const float scaleZ = 50.f;
							
							gxTranslatef(0.f, 250.f, -400.f);
							gxRotatef(std::sinf(framework.time * .123f) * 30.f, 0.f, 1.f, 0.f);
							gxScalef(scaleX, scaleY, scaleZ);

							const GLuint texture = mp->getTexture();

							if (texture != 0)
							{
								const int numX = int(std::round(scaleX / 60.f));
								const int numY = int(std::round(scaleY / 60.f));
								
								pushBlend(BLEND_OPAQUE);
								drawExtrusion(numX, numY, texture);
								popBlend();
							}
						}
						gxPopMatrix();
					}

					//

					{
						glDepthMask(false);
						
						setColor(127, 127, 127);
						Shader shader("particles");
						shader.setTexture("source", 0, getTexture("particle.jpg"), true, true);
						setShader(shader);
						{
							pushBlend(BLEND_ADD);
							ps.draw();
							popBlend();
						}
						clearShader();

						glDepthMask(true);
					}

					//

					{
						glDepthMask(false);

						pushBlend(BLEND_ADD);
						setColor(127, 127, 127);
						vm.draw();
						popBlend();

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
	}
	popSurface();

	if (false)
	{
		Shader shader("chromo.ps", "effect.vs", "chromo.ps");
		shader.setTexture("colormap", 0, surface->getTexture(), true, true);
		pushBlend(BLEND_OPAQUE);
		surface->postprocess(shader);
		popBlend();
	}
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

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

	if (framework.init(GFX_SX, GFX_SY))
	{
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

		mouse.showCursor(false);
		mouse.setRelative(true);

		Surface * surface = new Surface(GFX_SX, GFX_SY, false);
		Surface * surfaceL = new Surface(GFX_SX, GFX_SY, false, true);
		Surface * surfaceR = new Surface(GFX_SX, GFX_SY, false, true);

		scene = new Scene();

	#if ENABLE_VIDEOIN
		GLuint videoTexture = 0;
	#endif

		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			const float dt = framework.timeStep;

			scene->tick(dt);

			// todo : colour switch background or line style after x amount of time

			framework.beginDraw(0, 0, 0, 0);
			{
				const float eyeX = 0.f;
				const float eyeY = 0.f;

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
							pushBlend(BLEND_OPAQUE);
							drawRect(0.f, 0.f, GFX_SX, GFX_SY);
							popBlend();
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
					}
					popSurface();
				}
			#endif

				gxSetTexture(surface->getTexture());
				{
					pushBlend(BLEND_OPAQUE);
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
					popBlend();
				}
				gxSetTexture(0);
			}
			framework.endDraw();
		}

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
