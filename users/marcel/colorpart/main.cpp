#include "Calc.h"
#include "framework.h"
#include "image.h"
#include "srt.h"
#include "video.h"

static int GFX_SX = 0;
static int GFX_SY = 0;

#define NUM_PARTICLES 100000

static Surface * surface = nullptr;
static Surface * overlay = nullptr;
static ImageData * image = nullptr;
static float particlePickup = 0.f;
static float gravityX = 0.f;
static float gravityY = 0.f;

static MediaPlayer * mediaPlayer = nullptr;

struct Particle
{
	float px;
	float py;
	mutable float lastDrawPx;
	mutable float lastDrawPy;
	float vx;
	float vy;
	Color color;
	bool fixedColor;

	void takeColor(const float amount)
	{
		if (fixedColor)
		{
			color.set(2.f, .5f, 2.f, 1.f);
			return;
		}

		const int ix = int(px);
		const int iy = int(py);

		if (ix >= 0 && ix < image->sx && iy >= 0 && iy < image->sy)
		{
			const ImageData::Pixel & pixel = image->getLine(image->sy - 1 - iy)[ix];

			const Color imageColor = Color(pixel.r, pixel.g, pixel.b, pixel.a);

			color = color.interp(imageColor, amount);
		}
	}
};

struct ParticleSystem
{
	Particle particles[NUM_PARTICLES];

	static void applyGravitySource(Particle & p, const float dt, const float gx, const float gy, const float gs)
	{
		const float dx = gx - p.px;
		const float dy = gy - p.py;
		const float dsSq = dx * dx + dy * dy + 1.f;

		p.vx += dx / dsSq * gs * dt;
		p.vy += dy / dsSq * gs * dt;
	}

	static void applyBounce(float & p, float & v, const float min, const float max)
	{
		if (p < min)
		{
			p = min;
			v *= -1.f;
		}
		else if (p > max)
		{
			p = max;
			v *= -1.f;
		}
	}

	void tick(const float dt)
	{
		const float vfPerSecond = .1f;
		const float vf = std::powf(1.f - vfPerSecond, dt);

		for (auto & p : particles)
		{
			if (mouse.isDown(BUTTON_LEFT))
			{
				applyGravitySource(p, dt, mouse.x, mouse.y, 20000.f);
			}

			applyGravitySource(p, dt, gravityX, gravityY, 20000.f);

			if (true)
			{
				// velocity falloff

				p.vx *= vf;
				p.vy *= vf;
			}

			if (false)
			{
				// maintain fixed speed

				const float vs = std::sqrtf(p.vx * p.vx + p.vy * p.vy);
				p.vx /= vs;
				p.vy /= vs;
				p.vx *= 100.f;
				p.vy *= 100.f;
			}

			p.px += p.vx * dt;
			p.py += p.vy * dt;

			if (true)
			{
				// bounce

				applyBounce(p.px, p.vx, 0.f, image->sx);
				applyBounce(p.py, p.vy, 0.f, image->sy);
			}

			p.takeColor(particlePickup);
		}
	}

	void draw() const
	{
		setBlend(BLEND_OPAQUE);

		gxBegin(GL_LINES);
		{
			for (auto & p : particles)
			{
				gxColor4f(p.color.r, p.color.g, p.color.b, p.color.a);

				gxVertex2f(p.px,         p.py        );
				gxVertex2f(p.lastDrawPx, p.lastDrawPy);

				p.lastDrawPx = p.px;
				p.lastDrawPy = p.py;
			}
		}
		gxEnd();
	}
};

static void randomizeParticles(ParticleSystem & ps, const float cx, const float cy, const float speed)
{
	for (auto & p : ps.particles)
	{
		p.px = random(0.f, float(GFX_SX));
		p.py = random(0.f, float(GFX_SY));
		p.px = cx;
		p.py = cy;
		p.lastDrawPx = p.px;
		p.lastDrawPy = p.py;
		p.fixedColor = (rand() % 50) == 0;

		const float angle = random(0.f, Calc::m2PI);
		p.vx = std::cosf(angle) * speed;
		p.vy = std::sinf(angle) * speed;

		p.takeColor(1.f);
	}
}

static void blurSurface(Surface * surface, const float blurAmount)
{
	for (int i = 0; i < 2; ++i)
	{
		const char * shaderName = (i % 2) == 0 ? "guassian-h.ps" : "guassian-v.ps";

		Shader shader(shaderName, "effect.vs", shaderName);
		shader.setTexture("colormap", 0, surface->getTexture(), true, true);
		shader.setImmediate("amount", blurAmount);

		surface->postprocess(shader);
	}
}

static const int NUMTILES_X = 16;
static const int NUMTILES_Y = 10;

static void makeTileTransform(const int x, const int y, Mat4x4 & result)
{
	const float px = (x + .5f) / NUMTILES_X * GFX_SX;
	const float py = (y + .5f) / NUMTILES_Y * GFX_SY;
	const float sx = GFX_SX / NUMTILES_X / 2.f;
	const float sy = GFX_SY / NUMTILES_Y / 2.f;
	//const float scale = .9f;
	const float scale = 1.2f;
	const float angle = Calc::DegToRad(std::sinf(framework.time) * 20.f);

	result = Mat4x4(true).Translate(px, py, 0.f).Scale(sx * scale, sy * scale, 1.f).RotateZ(angle);
}

static void drawTileBG(const int x, const int y)
{
#if 0
	const float u1 = (x + .5f) / NUMTILES_X;
	const float v1 = (y + .5f) / NUMTILES_Y;
	const float u2 = (x + .5f) / NUMTILES_X;
	const float v2 = (y + .5f) / NUMTILES_Y;
#else
	const float u1 = (x + 0.f) / NUMTILES_X;
	const float v1 = (y + 0.f) / NUMTILES_Y;
	const float u2 = (x + 1.f) / NUMTILES_X;
	const float v2 = (y + 1.f) / NUMTILES_Y;
#endif

	Mat4x4 mat(false);

	makeTileTransform(x, y, mat);

	const Vec2 p1 = mat * Vec2(-1.f, -1.f);
	const Vec2 p2 = mat * Vec2(+1.f, -1.f);
	const Vec2 p3 = mat * Vec2(+1.f, +1.f);
	const Vec2 p4 = mat * Vec2(-1.f, +1.f);
	
	gxTexCoord2f(u1, v1); gxVertex2f(p1[0], p1[1]);
	gxTexCoord2f(u2, v1); gxVertex2f(p2[0], p2[1]);
	gxTexCoord2f(u2, v2); gxVertex2f(p3[0], p3[1]);
	gxTexCoord2f(u1, v2); gxVertex2f(p4[0], p4[1]);
}

static void drawTileFG(const int x, const int y)
{
	Mat4x4 mat(false);

	makeTileTransform(x, y, mat);

	const Vec2 p1 = mat * Vec2(-1.f, -1.f);
	const Vec2 p2 = mat * Vec2(+1.f, -1.f);
	const Vec2 p3 = mat * Vec2(+1.f, +1.f);
	const Vec2 p4 = mat * Vec2(-1.f, +1.f);

	gxVertex2f(p1[0], p1[1]); gxVertex2f(p2[0], p2[1]);
	gxVertex2f(p2[0], p2[1]); gxVertex2f(p3[0], p3[1]);
	gxVertex2f(p3[0], p3[1]); gxVertex2f(p4[0], p4[1]);
	gxVertex2f(p4[0], p4[1]); gxVertex2f(p1[0], p1[1]);
}

int main(int argc, char * argv[])
{
	changeDirectory("data");

	Srt srt;
	
	loadSrt("roar.srt", srt);

	//image = loadImage("andreea.png");
	image = loadImage("2.jpg");

	GFX_SX = image->sx;
	GFX_SY = image->sy;

	framework.windowBorder = false;

	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		mediaPlayer = new MediaPlayer();

		mediaPlayer->openAsync("roar.mpg", false);

		//Music("song.ogg").play(true);

		surface = new Surface(image->sx, image->sy, true);
		overlay = new Surface(image->sx, image->sy, true);

		surface->clear(0, 0, 0, 0);
		overlay->clear(0, 0, 0, 0);

		ParticleSystem * ps = new ParticleSystem();

		float randomizeTimer = 0.f;

		double time = 0.0;

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			const float realDt = framework.timeStep;
			const float dt = Calc::Mid(framework.timeStep, 0.f, 1.f / 60.f);

			time += realDt;

			if (mediaPlayer->isActive(mediaPlayer->context))
			{
				mediaPlayer->presentTime = time;
				mediaPlayer->tick(mediaPlayer->context);
			}

			if (randomizeTimer >= 0.f)
			{
				randomizeTimer -= dt;

				if (randomizeTimer <= 0.f)
				{
					//randomizeTimer = 3.f;

					const float cx = random(0.f, float(GFX_SX));
					const float cy = random(0.f, float(GFX_SY));

					randomizeParticles(*ps, cx, cy, 60.f);

					for (auto & p : ps->particles)
					{
						p.vx += random(-20.f, +20.f);
						p.vy += random(-20.f, +20.f);
					}
				}
			}

			const float particlePickupPerSec = std::powf((std::sinf(framework.time / 3.210f) + 1.f) / 2.f, 8.f);
			//const float particlePickupPerSec = std::powf(mouse.x / float(GFX_SX), 8.f);

			particlePickup = 1.f - std::powf(particlePickupPerSec, dt);

			gravityX = std::cosf(framework.time / 2.345f) * 200.f + GFX_SX/2.f;
			gravityY = std::sinf(framework.time / 1.234f) * 100.f + GFX_SY/2.f;

			ps->tick(dt);

			framework.beginDraw(0, 0, 0, 0);
			{
				setBlend(BLEND_OPAQUE);
				const float blurAmount = (std::sinf(framework.time / 2.f) + 1.f) / 2.f;
				blurSurface(surface, blurAmount);

				pushSurface(surface);
				{
					setBlend(BLEND_ALPHA);
					const float falloffPerSecond = (std::sinf(framework.time / 4.f) + 1.f) / 2.f;
					const float falloff = 1.f - std::powf(falloffPerSecond, dt);
					setColorf(0.f, 0.f, 0.f, falloff);
					drawRect(0, 0, surface->getWidth(), surface->getHeight());

					ps->draw();

					//setColor(colorWhite);
					//fillCircle(gravityX, gravityY, 5.f, 16);
				}
				popSurface();

				gxSetTexture(surface->getTexture());
				{
					setBlend(BLEND_OPAQUE);
					setColor(colorWhite);
					drawRect(0, 0, surface->getWidth(), surface->getHeight());
				}
				gxSetTexture(0);

				//blurSurface(overlay, 1.f);

				pushSurface(overlay);
				{
					setBlend(BLEND_MUL);
					setColorf(.99f, .98f, .97f, .99f);
					drawRect(0, 0, overlay->getWidth(), overlay->getHeight());

					setBlend(BLEND_ALPHA);
					setFont("BeautifulEveryTime.ttf");
					setColor(colorWhite);

					const SrtFrame * srtFrame = srt.findFrameByTime(mediaPlayer->presentTime);

					if (srtFrame != nullptr)
					{
						for (size_t i = 0; i < srtFrame->lines.size(); ++i)
						{
							//drawText(GFX_SX/2, GFX_SY*3/4, 48, 0.f, 0.f, "%s", srtFrame->text.c_str());
							drawText(GFX_SX/2, GFX_SY*3/4 + i * 50, 48, 0.f, 0.f, "%s", srtFrame->lines[i].c_str());
						}
					}

					//drawText(GFX_SX/2, GFX_SY*3/4, 48, 0.f, 0.f, "Happy Birthday!");
				}
				popSurface();

				gxSetTexture(overlay->getTexture());
				{
					setBlend(BLEND_ALPHA);
					setColor(colorWhite);
					drawRect(0, 0, overlay->getWidth(), overlay->getHeight());
				}
				gxSetTexture(0);

				if (mediaPlayer->isActive(mediaPlayer->context))
				{
					const GLuint texture = mediaPlayer->getTexture();
					if (texture != 0)
					{
						gxSetTexture(texture);
						{
							setBlend(BLEND_ADD);
							{
								setColor(255, 255, 255, 127);
								gxBegin(GL_QUADS);
								{
									for (int tileX = 0; tileX < NUMTILES_X; ++tileX)
									{
										for (int tileY = 0; tileY < NUMTILES_Y; ++tileY)
										{
											drawTileBG(tileX, tileY);
										}
									}
								}
								gxEnd();
							}
							setBlend(BLEND_ALPHA);
						}
						gxSetTexture(0);

						setBlend(BLEND_ADD);
						{
							setColor(255, 255, 255, 255, 127);
							glEnable(GL_LINE_SMOOTH);
							glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
							gxBegin(GL_LINES);
							{
								for (int tileX = 0; tileX < NUMTILES_X; ++tileX)
								{
									for (int tileY = 0; tileY < NUMTILES_Y; ++tileY)
									{
										drawTileFG(tileX, tileY);
									}
								}
							}
							gxEnd();
							glDisable(GL_LINE_SMOOTH);
						}
						setBlend(BLEND_ALPHA);

						gxSetTexture(texture);
						{
							setBlend(BLEND_ADD);
							{
								setColor(255, 255, 255, 255, 127);
								//drawRect(0, GFX_SY, GFX_SX, 0);
							}
							setBlend(BLEND_ALPHA);
						}
						gxSetTexture(0);
					}
				}
			}
			framework.endDraw();
		}

		delete overlay;
		overlay = nullptr;

		delete surface;
		surface = nullptr;

		delete image;
		image = nullptr;

		delete mediaPlayer;
		mediaPlayer = nullptr;

		framework.shutdown();
	}

	return 0;
}
