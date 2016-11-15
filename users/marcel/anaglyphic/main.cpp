#include "Calc.h"
#include "framework.h"
#include <list>

#define GFX_SX 1920
#define GFX_SY 1080

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

struct Camera
{
	Vec3 position;
	Vec3 rotation;

	void tick(const float dt)
	{
		rotation[0] -= mouse.dy / 100.f;
		rotation[1] -= mouse.dx / 100.f;

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

		const float speed = 200.f;

		position += direction * speed * dt;
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

		matrix = Mat4x4(true).Translate(position).RotateY(rotation[1]).RotateX(rotation[0]).Translate(eyeOffset, 0.f, 0.f);
	}
};

struct Scene
{
	Camera camera;

	Shader modelShader;
	std::list<Model*> models;

	Scene()
		: camera()
		, modelShader("model-lit")
		, models()
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
				model->overrideShader = &modelShader;
				model->animRootMotionEnabled = false;

				models.push_back(model);
			}
		}
	}

	~Scene()
	{
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

	for (Model * model : models)
	{
		model->axis = Vec3(0.f, -1.f, 0.f);
		model->angle += -.1f * dt;

		model->angle += mouse.dx / 1000.f;

		if (!model->animIsActive)
		{
			const auto animList = model->getAnimList();

			model->startAnim(animList[rand() % animList.size()].c_str());
		}
	}
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

			gxMatrixMode(GL_MODELVIEW);
			gxPushMatrix();
			{
				gxLoadMatrixf(matC.m_v);

				setBlend(BLEND_OPAQUE);
				setColor(colorWhite);

				setShader("engine/BasicSkinned");
				{
					//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					checkErrorGL();

					for (Model * model : models)
					{
						gxPushMatrix();
						{
							model->draw();

							clearShader();

							Mat4x4 mat;
							model->calculateTransform(mat);
							gxMultMatrixf(mat.m_v);

							gxBegin(GL_QUADS);
							{
								const float s = 100.f;

								gxColor4f(.5f, .5f, .5f, 1.f);
								gxVertex3f(-s, 0.f, -s);
								gxVertex3f(+s, 0.f, -s);
								gxVertex3f(+s, 0.f, +s);
								gxVertex3f(-s, 0.f, +s);
							}
							gxEnd();
						}
						gxPopMatrix();
					}

					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					checkErrorGL();
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
}

int main(int argc, char * argv[])
{
	//changeDirectory("data");

	framework.enableDepthBuffer = true;

	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		mouse.showCursor(false);
		mouse.setRelative(true);

		Surface * surface = new Surface(GFX_SX, GFX_SY, true);
		Surface * surfaceL = new Surface(GFX_SX, GFX_SY, true, true);
		Surface * surfaceR = new Surface(GFX_SX, GFX_SY, true, true);

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

					setBlend(BLEND_OPAQUE);
					pushSurface(surface);
					{
						Shader shader("anaglyph.ps", "effect.vs", "anaglyph.ps");
						shader.setTexture("colormapL", 0, colormapL, true, false);
						shader.setTexture("colormapR", 1, colormapR, true, false);
						setShader(shader);
						{
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
