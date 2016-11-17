#include "Calc.h"
#include "framework.h"
#include "video.h"
#include <list>

#define ENABLE_MEDIA_FOUNDATION 0
#define ENABLE_VIDEO_FOR_WINDOWS 0
#define ENABLE_FACE_DETECTION 1

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

#if ENABLE_FACE_DETECTION
	#include "image.h"
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

	float baseOffset;

	Camera()
		: position()
		, rotation()
		, baseOffset(0.f)
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

		calculateTransform(0.f, mat);

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

		if (keyboard.isDown(SDLK_a))
			baseOffset -= 10.f * dt;
		if (keyboard.isDown(SDLK_s))
			baseOffset += 10.f * dt;
	}

	void calculateTransform(const float eyeOffset, Mat4x4 & matrix) const
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

		matrix = Mat4x4(true).Translate(position).RotateY(rotation[1]).RotateX(rotation[0]).Translate(baseOffset + eyeOffset, 0.f, 0.f);
	}
};

struct Scene
{
	Camera camera;

	Shader modelShader;
	std::list<Model*> models;

	ParticleSystem ps;

	MediaPlayer * mp;

	Scene()
		: camera()
		, modelShader("model-lit")
		, models()
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
	void draw(Surface * surface, const float eye) const;
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

	mp->tick(mp->context);

	//mp->presentTime += framework.timeStep * .25f;
	mp->presentTime += framework.timeStep;
}

void Scene::draw(Surface * surface, const float eye) const
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
		camera.calculateTransform(68.f/10.f/2.f * eye, matC);
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
							ps.draw();
						}
						clearShader();

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

int main(int argc, char * argv[])
{
	//changeDirectory("data");

#if !defined(DEBUG) && 1
	framework.fullscreen = true;
	framework.exclusiveFullscreen = false;
	framework.useClosestDisplayMode = true;
#endif

	framework.enableDepthBuffer = true;

	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
#if ENABLE_VIDEO_FOR_WINDOWS
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
				sizeof(szDeviceVersion)))
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
			const HWND window = capCreateCaptureWindow("Capture Window", WS_VISIBLE | WS_POPUP | WS_CHILD, 0, 0, 800, 600, 0, 0);

			bool result = capDriverConnect(window, deviceIndex);

			if (result)
			{
				MSG message;

				while (GetMessage(&message, 0, 0, 0))
				{
					TranslateMessage(&message);
					DispatchMessage(&message);

					capGrabFrame(window);
				}
			}
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

#if ENABLE_FACE_DETECTION
		ImageData * face = loadImage("face.jpg");
		int eraseY = 0;

		while (!keyboard.isDown(SDLK_SPACE))
		{
			framework.process();

			//

			int64_t totalValue = 0;
			int64_t totalX = 0;
			int64_t totalY = 0;

			for (int y = 0; y < face->sy; ++y)
			{
				ImageData::Pixel * __restrict line = face->getLine(y);

				for (int x = 0; x < face->sx; ++x)
				{
					const int64_t lumi = line[x].r + line[x].g + line[x].b;

					totalValue += lumi;
					totalX += x * lumi;
					totalY += y * lumi;
				}

				if (y == eraseY)
					memset(line, 0, sizeof(ImageData::Pixel) * face->sx);
			}

			eraseY++;

			const int weightedX = totalX / totalValue;
			const int weightedY = totalY / totalValue;

			logDebug("weighted pos: %d, %d", weightedX, weightedY);

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
				fillCircle(weightedX, weightedY, 10.f, 20);
			}
			framework.endDraw();
		}
#endif

		mouse.showCursor(false);
		mouse.setRelative(true);

		Surface * surface = new Surface(GFX_SX, GFX_SY, false);
		Surface * surfaceL = new Surface(GFX_SX, GFX_SY, false, true);
		Surface * surfaceR = new Surface(GFX_SX, GFX_SY, false, true);

		Pisu pisu;
		
		scene = new Scene();

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

				scene->draw(surfaceL, -1.f);
				scene->draw(surfaceR, +1.f);

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

		delete scene;
		scene = nullptr;

		delete surfaceL;
		surfaceL = nullptr;

		delete surfaceR;
		surfaceR = nullptr;

		delete surface;
		surface = nullptr;
		
		framework.shutdown();
	}

	return 0;
}
