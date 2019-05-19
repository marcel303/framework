#include "audiooutput/AudioOutput_PortAudio.h"
#include "audiostream/AudioStream.h"
#include "Calc.h"
#include "framework.h"
#include "image.h"
#include "srt.h"
#include "video.h"

#define DO_VIDEO 1
#define DO_LYRICS 1

static int GFX_SX = 0;
static int GFX_SY = 0;

#define GFX_SCALE 1
#define GFX_SX_SCALED (GFX_SX * GFX_SCALE)
#define GFX_SY_SCALED (GFX_SY * GFX_SCALE)

#define NUM_PARTICLES 100000

static Surface * finalSurface = nullptr;
static Surface * particleSurface = nullptr;
static Surface * textOverlay = nullptr;
static Surface * videoOverlay = nullptr;
static ImageData * image = nullptr;
static float particlePickup = 0.f;
static float gravityX = 0.f;
static float gravityY = 0.f;

static MediaPlayer * mediaPlayer = nullptr;

static int mouseX = 0;
static int mouseY = 0;

const static float colormul = 1.f / 255.f;

struct Particle
{
	float px;
	float py;
	mutable float lastDrawPx;
	mutable float lastDrawPy;
	float vx;
	float vy;
	float r;
	float cr, cg, cb, ca;
	bool fixedColor;

	void takeColor(const float amount)
	{
#if 1
		if (fixedColor)
		{
			cr = 1.5f;
			cg = .5f;
			cb = 2.f;
			ca = 1.f;
			return;
		}
#endif

		const int ix = int(px);
		const int iy = int(py);

		if (ix >= 0 && ix < image->sx && iy >= 0 && iy < image->sy)
		{
			const ImageData::Pixel & pixel = image->getLine(iy)[ix];

			const float amount1 = 1.f - amount;
			const float amount2 =       amount * colormul;

			cr = cr * amount1 + pixel.r * amount2;
			cg = cg * amount1 + pixel.g * amount2;
			cb = cb * amount1 + pixel.b * amount2;
			ca = ca * amount1 + pixel.a * amount2;
		}

		if (fixedColor)
		{
			const float r1 = r;
			const float r2 = 1.f - r1;
			cr = cr * r1 + 2.f * r2;
			cg = cg * r1 + 2.f * r2;
			cb = cb * r1 + .5f * r2;
			ca = ca * r1 + 1.f * r2;
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

		const float g = 1.f / dsSq;

		p.vx += dx * g * gs * dt;
		p.vy += dy * g * gs * dt;

		const float r = 1.f - g * 1000.f;
		if (r < p.r)
			p.r = r < 0.f ? 0.f : r;
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
		const float vf = powf(1.f - vfPerSecond, dt);

		for (auto & p : particles)
		{
			if (mouse.isDown(BUTTON_LEFT))
			{
				applyGravitySource(p, dt, mouseX, mouseY, 20000.f);
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

				const float vs = sqrtf(p.vx * p.vx + p.vy * p.vy);
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

			p.r += dt * .5f;

			if (p.r > 1.f)
				p.r = 1.f;
		}
	}

	void draw() const
	{
		gxBegin(GX_LINES);
		{
			for (auto & p : particles)
			{
				gxColor4f(p.cr, p.cg, p.cb, p.ca);

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
		p.vx = cosf(angle) * speed;
		p.vy = sinf(angle) * speed;

		p.r = 1.f;

		p.takeColor(1.f);
	}
}

static void blurSurface(Surface * src, Surface * dst, const float blurAmount)
{
	pushBlend(BLEND_OPAQUE);

	for (int i = 0; i < 2; ++i)
	{
		const char * shaderName = (i % 2) == 0 ? "guassian-h.ps" : "guassian-v.ps";

		Shader shader(shaderName, "effect.vs", shaderName);
		setShader(shader);
		shader.setTexture("colormap", 0, src->getTexture(), true, true);
		shader.setImmediate("amount", blurAmount);

		if (dst == src)
		{
			src->swapBuffers();
		}

		pushSurface(dst);
		{
			setShader(shader);
			{
				drawRect(0.f, 0.f, dst->getWidth(), dst->getHeight());
			}
			clearShader();
		}
		popSurface();
	}
	
	popBlend();
}

static void blurSurface(Surface * surface, const float blurAmount)
{
	blurSurface(surface, surface, blurAmount);
}

static const int NUMTILES_X = 16*2/3;
static const int NUMTILES_Y = 9*2/3;

struct GridCell
{
	GridCell()
		: transform(false)
		, mouseTimer(0.f)
		, mouseTimerRcp(0.f)
		, wasInside(false)
		, scaleTimer(0.f)
		, scaleTimerRcp(0.f)
	{
	}

	Mat4x4 transform;

	float mouseTimer;
	float mouseTimerRcp;

	bool wasInside;
	float scaleTimer;
	float scaleTimerRcp;
};

struct Grid
{
	GridCell cells[NUMTILES_X][NUMTILES_Y];

	int lastMouseX;
	int lastMouseY;

	Grid()
		: lastMouseX(-1)
		, lastMouseY(-1)
	{
	}

	void makeTileTransform(const int x, const int y, const bool forDraw, Mat4x4 & result)
	{
		const GridCell & cell = cells[x][y];

		const float px = (x + .5f) / NUMTILES_X * GFX_SX;
		const float py = (y + .5f) / NUMTILES_Y * GFX_SY;
		const float sx = GFX_SX / NUMTILES_X / 2.f;
		const float sy = GFX_SY / NUMTILES_Y / 2.f;
	#if 1
		const float h = hypotf(mouseX - px, mouseY - py);
		const float scale = 1.2f / (h / 400.f + 1.f);
		//const float angle = Calc::DegToRad(sinf(framework.time) * 20.f);
		const float angle = Calc::Lerp(0.f, Calc::DegToRad(sinf(framework.time) * 20.f), h / 400.f);
	#else
		const float scale = 1.f;
		const float angle = 0.f;
	#endif

		const float scaleX = !forDraw ? 1.f : cosf((1.f - cell.scaleTimer * cell.scaleTimerRcp) * Calc::mPI2);
		const float scaleY = !forDraw ? 1.f : 1.f;

		result = Mat4x4(true).Translate(px, py, 0.f).Scale(sx * scale, sy * scale, 1.f).Scale(scaleX, scaleY, 1.f).RotateZ(angle);
	}

	void tick(const float dt)
	{
		const bool mouseMove =
			mouseX != lastMouseX ||
			mouseY != lastMouseY;

		if (mouseMove)
		{
			lastMouseX = mouseX;
			lastMouseY = mouseY;
		}

		for (int x = 0; x < NUMTILES_X; ++x)
		{
			for (int y = 0; y < NUMTILES_Y; ++y)
			{
				GridCell & cell = cells[x][y];

				makeTileTransform(x, y, false, cell.transform);

				cell.mouseTimer -= dt;
				if (cell.mouseTimer < 0.f)
					cell.mouseTimer = 0.f;

				cell.scaleTimer -= dt;
				if (cell.scaleTimer < 0.f)
					cell.scaleTimer = 0.f;

				// mouse

				const Mat4x4 invTransform = cell.transform.Invert();
				const Vec2 localMouse = invTransform.Mul4(Vec2(mouseX, mouseY));

				bool isInside =
					localMouse[0] >= -1.f &&
					localMouse[0] <= +1.f &&
					localMouse[1] >= -1.f &&
					localMouse[1] <= +1.f;

				if (isInside != cell.wasInside)
				{
					cell.wasInside = isInside;

					if (isInside && cell.scaleTimer == 0.f)
					{
						cell.scaleTimer = 2.f;
						cell.scaleTimerRcp = 1.f / cell.scaleTimer;
					}
				}

				const bool specialMode = mouse.isDown(BUTTON_LEFT);

				if (specialMode)
				{
					cell.scaleTimer = 1.f;
					cell.scaleTimerRcp = 1.f / cell.scaleTimer;
				}

				if (mouseMove || specialMode)
				{
					const float distance = localMouse.CalcSize();
					const float maxTime = 2.f;
					const float radius = mouse.isDown(BUTTON_LEFT) ? 10.f : 4.5f;
					const float time = maxTime * (1.f - distance / radius);

					if (time > cell.mouseTimer)
					{
						cell.mouseTimer = time;
						cell.mouseTimerRcp = 1.f / maxTime;
					}
				}

				makeTileTransform(x, y, true, cell.transform);
			}
		}
	}

	void drawBG() const
	{
		for (int x = 0; x < NUMTILES_X; ++x)
		{
			for (int y = 0; y < NUMTILES_Y; ++y)
			{
				const GridCell & cell = cells[x][y];

				//const float ca = 1.f - cell.mouseTimer * cell.mouseTimerRcp;
				const float ca = Calc::Mid(cell.mouseTimer * cell.mouseTimerRcp * 1.5f, 0.f, 1.f);

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

				const Mat4x4 & mat = cells[x][y].transform;

				const Vec2 p1 = mat * Vec2(-1.f, -1.f);
				const Vec2 p2 = mat * Vec2(+1.f, -1.f);
				const Vec2 p3 = mat * Vec2(+1.f, +1.f);
				const Vec2 p4 = mat * Vec2(-1.f, +1.f);

				gxColor4f(1.f, 1.f, 1.f, ca);

				gxTexCoord2f(u1, v1); gxVertex2f(p1[0], p1[1]);
				gxTexCoord2f(u2, v1); gxVertex2f(p2[0], p2[1]);
				gxTexCoord2f(u2, v2); gxVertex2f(p3[0], p3[1]);
				gxTexCoord2f(u1, v2); gxVertex2f(p4[0], p4[1]);
			}
		}
	}

	void drawFG() const
	{
		for (int x = 0; x < NUMTILES_X; ++x)
		{
			for (int y = 0; y < NUMTILES_Y; ++y)
			{
				const GridCell & cell = cells[x][y];

				//const float ca = 1.f - cell.mouseTimer * cell.mouseTimerRcp;
				const float ca = Calc::Mid(cell.mouseTimer * cell.mouseTimerRcp * 1.5f, 0.f, 1.f);

				const Mat4x4 & mat = cell.transform;

				const Vec2 p1 = mat * Vec2(-1.f, -1.f);
				const Vec2 p2 = mat * Vec2(+1.f, -1.f);
				const Vec2 p3 = mat * Vec2(+1.f, +1.f);
				const Vec2 p4 = mat * Vec2(-1.f, +1.f);

				gxColor4f(1.f, 1.f, 1.f, ca);

				const float stroke = 1.3f;
				
				hqLine(p1[0], p1[1], stroke, p2[0], p2[1], stroke);
				hqLine(p2[0], p2[1], stroke, p3[0], p3[1], stroke);
				hqLine(p3[0], p3[1], stroke, p4[0], p4[1], stroke);
				hqLine(p4[0], p4[1], stroke, p1[0], p1[1], stroke);
			}
		}
	}
};

static int imageIndex = -1;

static void randomizeImage()
{
	const char * filenames[] =
	{
		"2.jpg",
		"3.jpg"
	};
	const int numFilenames = sizeof(filenames) / sizeof(filenames[0]);

	int newImageIndex;
	do
	{
		newImageIndex = rand() % numFilenames;
	} while (imageIndex == newImageIndex && numFilenames > 1);
	
	delete image;
	image = nullptr;

	image = loadImage(filenames[newImageIndex]);
	imageIndex = newImageIndex;
}

int main(int argc, char * argv[])
{
	changeDirectory("/Users/thecat/Google Drive/Projects/colorpart");
	
	Srt srt;
	
	loadSrt("roar.srt", srt);

	image = loadImage("2.jpg");
	
	if (image == nullptr)
		return -1;

	GFX_SX = image->sx;
	GFX_SY = image->sy;

	framework.windowBorder = false;

#if 0
	framework.fullscreen = true;
	framework.useClosestDisplayMode = true;
	framework.exclusiveFullscreen = false;
#endif

	if (framework.init(GFX_SX_SCALED, GFX_SY_SCALED))
	{
		mediaPlayer = new MediaPlayer();

	#if DO_VIDEO
		MediaPlayer::OpenParams params;
		params.filename = "roar.mpg";
		params.enableAudioStream = true;
		mediaPlayer->openAsync(params);

		AudioOutput_PortAudio * audioOutput = nullptr;
	#endif

		//Music("song.ogg").play(true);

		finalSurface = new Surface(GFX_SX, GFX_SY, true);
		particleSurface = new Surface(GFX_SX, GFX_SY, true);
		textOverlay = new Surface(GFX_SX, GFX_SY, true);
		videoOverlay = new Surface(GFX_SX, GFX_SY, true);

		finalSurface->clear(0, 0, 0, 0);
		particleSurface->clear(0, 0, 0, 0);
		textOverlay->clear(0, 0, 0, 0);
		videoOverlay->clear(0, 0, 0, 0);

		ParticleSystem * ps = new ParticleSystem();

		float randomizeTimer = 0.f;

		Grid grid;

		double time = 0.0;

		while (!framework.quitRequested)
		{
			framework.process();

			mouseX = mouse.x / GFX_SCALE;
			mouseY = mouse.y / GFX_SCALE;

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			const float realDt = framework.timeStep;
			const float dt = Calc::Mid(framework.timeStep, 0.f, 1.f / 60.f);

			time += realDt;

			if (mediaPlayer->isActive(mediaPlayer->context))
			{
				mediaPlayer->presentTime = mediaPlayer->audioTime;
				
				mediaPlayer->tick(mediaPlayer->context, true);

			#if DO_VIDEO
				if (audioOutput == nullptr)
				{
					int channelCount = 0;
					int sampleRate = 0;

					if (mediaPlayer->getAudioProperties(channelCount, sampleRate))
					{
						audioOutput = new AudioOutput_PortAudio();
						
						audioOutput->Initialize(channelCount, sampleRate, 256);
						audioOutput->Play(mediaPlayer);
					}
				}
				
				if (audioOutput != nullptr)
				{
					audioOutput->Update();
				}
			#endif
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

			if (keyboard.wentDown(SDLK_SPACE))
			{
				randomizeImage();
			}

			const float particlePickupPerSec = powf((sinf(framework.time / 3.210f) + 1.f) / 2.f, 8.f);
			//const float particlePickupPerSec = powf(mouseX / float(GFX_SX), 8.f);

			particlePickup = 1.f - powf(particlePickupPerSec, dt);

			gravityX = cosf(framework.time / 2.345f) * 200.f + GFX_SX/2.f;
			gravityY = sinf(framework.time / 1.234f) * 100.f + GFX_SY/2.f;

			ps->tick(dt);

			grid.tick(dt);

			framework.beginDraw(0, 0, 0, 0);
			{
				finalSurface->clear(0, 0, 0, 0);

				pushSurface(finalSurface);
				{
					const float blurAmount = (sinf(framework.time / 2.f) + 1.f) / 2.f;
					blurSurface(particleSurface, blurAmount);

					pushSurface(particleSurface);
					{
						const float falloffPerSecond = (sinf(framework.time / 4.f) + 1.f) / 2.f;
						const float falloff = 1.f - powf(falloffPerSecond, dt);
						setColorf(0.f, 0.f, 0.f, falloff);
						drawRect(0, 0, particleSurface->getWidth(), particleSurface->getHeight());

						pushBlend(BLEND_OPAQUE);
						ps->draw();
						popBlend();
						
						pushBlend(BLEND_ALPHA);
						hqBegin(HQ_STROKED_CIRCLES);
						setColor(colorWhite);
						hqStrokeCircle(gravityX, gravityY, 5.f, 2.f);
						hqEnd();
						popBlend();
					}
					popSurface();

					if (false)
					{
						Shader shader("kaleido.ps", "effect.vs", "kaleido.ps");
						setShader(shader);
						shader.setTexture("colormap", 0, particleSurface->getTexture(), true, false);
						shader.setImmediate("param1", 0);
						shader.setImmediate("param2", 10);
						shader.setImmediate("param3", 0);
						shader.setImmediate("param4", 0);
						shader.setImmediate("time", time * 5.f);
						shader.setImmediate("alpha", (sinf(framework.time * .1f) + 1.f)/2.f/4.f);
						setShader(shader);
						{
							pushBlend(BLEND_OPAQUE);
							drawRect(0, 0, finalSurface->getWidth(), finalSurface->getHeight());
							popBlend();
						}
						clearShader();
					}
					else
					{
						gxSetTexture(particleSurface->getTexture());
						{
							pushBlend(BLEND_OPAQUE);
							setColor(colorWhite);
							drawRect(0, 0, finalSurface->getWidth(), finalSurface->getHeight());
							popBlend();
						}
						gxSetTexture(0);
					}

				#if DO_VIDEO
					blurSurface(videoOverlay, 1.f);

					pushSurface(videoOverlay);
					{
						pushBlend(BLEND_MUL);
						setColorf(.99f, .98f, .97f, .95f);
						drawRect(0, 0, textOverlay->getWidth(), textOverlay->getHeight());
						popBlend();
						
						if (mediaPlayer->isActive(mediaPlayer->context))
						{
							const GxTextureId texture = mediaPlayer->getTexture();
							
							if (texture != 0)
							{
								gxSetTexture(texture);
								{
									pushBlend(BLEND_ALPHA);
									{
										gxBegin(GX_QUADS);
										{
											grid.drawBG();
										}
										gxEnd();
									}
									popBlend();
								}
								gxSetTexture(0);

								pushBlend(BLEND_ALPHA);
								{
									setColor(255, 255, 255, 255, 63);
									hqBegin(HQ_LINES);
									{
										grid.drawFG();
									}
									hqEnd();
								}
								popBlend();
							}
						}
					}
					popSurface();

					gxSetTexture(videoOverlay->getTexture());
					{
						pushBlend(BLEND_ALPHA);
						setColor(colorWhite);
						drawRect(0, 0, finalSurface->getWidth(), finalSurface->getHeight());
						popBlend();
					}
					gxSetTexture(0);
				#endif

					//

				#if DO_LYRICS
					blurSurface(textOverlay, 1.f);

					pushSurface(textOverlay);
					{
						pushBlend(BLEND_MUL);
						setColorf(.99f, .98f, .97f, .95f);
						drawRect(0, 0, textOverlay->getWidth(), textOverlay->getHeight());
						popBlend();

						const SrtFrame * srtFrame = srt.findFrameByTime(mediaPlayer->audioTime);

						if (srtFrame != nullptr)
						{
							pushBlend(BLEND_ALPHA);
							setFont("BeautifulEveryTime.ttf");
							setColor(colorWhite);
							
							for (size_t i = 0; i < srtFrame->lines.size(); ++i)
							{
								drawText(GFX_SX/2, GFX_SY*3/4 + i * 50, 48, 0.f, 0.f, "%s", srtFrame->lines[i].c_str());
							}
							
							popBlend();
						}
					}
					popSurface();

					gxSetTexture(textOverlay->getTexture());
					{
						pushBlend(BLEND_ALPHA);
						setColor(colorWhite);
						drawRect(0, 0, finalSurface->getWidth(), finalSurface->getHeight());
						popBlend();
					}
					gxSetTexture(0);
				#endif
				}
				popSurface();

				gxSetTexture(finalSurface->getTexture());
				{
					setBlend(BLEND_OPAQUE);
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX_SCALED, GFX_SY_SCALED);
				}
				gxSetTexture(0);
			}
			framework.endDraw();
		}

		delete videoOverlay;
		videoOverlay = nullptr;

		delete textOverlay;
		textOverlay = nullptr;

		delete particleSurface;
		particleSurface = nullptr;

		delete finalSurface;
		finalSurface = nullptr;

		delete image;
		image = nullptr;

	#if DO_VIDEO
		delete audioOutput;
		audioOutput = nullptr;
	#endif

		delete mediaPlayer;
		mediaPlayer = nullptr;

		framework.shutdown();
	}

	return 0;
}
