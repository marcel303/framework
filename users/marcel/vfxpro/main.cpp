#include "ip/UdpSocket.h"
#include "mediaplayer_old/MPContext.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "audiostream/AudioOutput.h"
#include "audioin.h"
#include "Calc.h"
#include "config.h"
#include "drawable.h"
#include "framework.h"
#include "Path.h"
#include "scene.h"
#include "Timer.h"
#include "tinyxml2.h"
#include "types.h"
#include "video.h"
#include "xml.h"
#include <algorithm>
#include <list>
#include <map>
#include <Windows.h>

#include "data/ShaderConstants.h"

using namespace tinyxml2;

const static int kNumScreens = 3;

#define SCREEN_SX 1024
#define SCREEN_SY 768

#define GFX_SX (SCREEN_SX * kNumScreens)
#define GFX_SY (SCREEN_SY * 1)

#define OSC_ADDRESS "127.0.0.1"
#define OSC_RECV_PORT 1121

const float eps = 1e-10f;
const float pi2 = float(M_PI) * 2.f;

static Config config;

static float virtualToScreenX(float x)
{
	return ((x / 100.f) + 1.5f) * SCREEN_SX;
}

static float virtualToScreenY(float y)
{
	return (y / 100.f + .5f) * SCREEN_SY;
}

/*

:: todo :: OSC

	-

:: todo :: configuration

	# prompt for MIDI controller at startup. or select by name in XML config file?
		+ defined in settings.xml instead
	# prompt for audio input device at startup. or select by name in XML config file?
		+ defined in settings xml instead
	+ add XML settings file
	- define scene XML representation
	- discuss with Max what would be needed for life act

:: todo :: projector output

	- add brightness control
		+ write shader which does a lookup based on the luminance of the input and transforms the input
		- add ability to change the setting
	- add border blends to hide projector seam. unless eg MadMapper already does this, it may be necessary to do it ourselvess

:: todo :: utility functions

	+ add PCM capture
	- add FFT calculation PCM data
	+ add loudness calculation PCM data

:: todo :: post processing and graphics quality

	- smooth line drawing with high AA. use a post process pass to blur the result ?

:: todo :: visuals tech 2D

	+ add a box blur shader. allow it to darken the output too

	- integrate Box2D ?

	+ add flow map shader
		+ use an input texture to warp/distort the previous frame

	- add film grain shader

:: todo :: visuals tech 3D

	- virtual camera positioning
		- allow positioning of the virtual camera based on settings XML
		- allow tweens on the virtual camera position ?

	+ compute virtual camera matrices

	- add lighting shader code

:: todo :: effects

	- particle effect : sea

	- particle effect : rain

	- particle effect : bouncy (sort of like rain, but with bounce effect + ability to control with sensor input)

	- cloth simulation

	- particle effect :: star cluster

:: todo :: particle system

	- ability to toggle particle trails

:: todo :: color controls

	- 

:: notes

	- seamless transitions between scenes

*/

struct Effect;

//

static void registerEffect(const char * name, Effect * effect);
static void unregisterEffect(Effect * effect);
static Effect * getEffect(const char * name);

struct Effect : TweenFloatCollection
{
	bool is3D; // when set to 3D, the effect is rendered using a separate virtual camera to each screen. when false, it will use simple 1:1 mapping onto screen coordinates
	Mat4x4 transform; // transformation matrix for 3D effects
	float screenX;
	float screenY;
	float scaleX;
	float scaleY;
	float z;

	Effect(const char * name)
		: is3D(false)
		, screenX(0.f)
		, screenY(0.f)
		, scaleX(1.f)
		, scaleY(1.f)
		, z(0.f)
	{
		transform.MakeIdentity();

		if (name != nullptr)
		{
			registerEffect(name, this);
		}
	}

	virtual ~Effect()
	{
		unregisterEffect(this);
	}

	Vec2 screenToLocal(Vec2Arg v) const
	{
		return Vec2(
			(v[0] - screenX) / scaleX,
			(v[1] - screenY) / scaleY);
	}

	Vec2 localToScreen(Vec2Arg v) const
	{
		return Vec2(
			v[0] * scaleX + screenX,
			v[1] * scaleY + screenY);
	}

	Vec3 worldToLocal(Vec3Arg v, const bool withTranslation) const
	{
		fassert(is3D);

		const Mat4x4 invTransform = transform.CalcInv();

		if (withTranslation)
			return invTransform.Mul4(v);
		else
			return invTransform.Mul3(v);
	}

	Vec3 localToWorld(Vec3Arg v, const bool withTranslation) const
	{
		fassert(is3D);

		if (withTranslation)
			return transform.Mul4(v);
		else
			return transform.Mul3(v);
	}

	virtual void tick(const float dt) = 0;
	virtual void draw(DrawableList & list) = 0;
	virtual void draw() = 0;
};

struct EffectDrawable : Drawable
{
	Effect * m_effect;

	EffectDrawable(Effect * effect)
		: Drawable(effect->z)
		, m_effect(effect)
	{
	}

	virtual void draw() override
	{
		gxPushMatrix();
		{
			if (m_effect->is3D)
				glMultMatrixf(m_effect->transform.m_v); // fixme : use gx call
			else
				gxTranslatef(m_effect->screenX, m_effect->screenY, 0.f);

			m_effect->draw();
		}
		gxPopMatrix();
	}
};

static std::map<std::string, Effect*> g_effectsByName;

static void registerEffect(const char * name, Effect * effect)
{
	Assert(g_effectsByName.count(name) == 0);

	g_effectsByName[name] = effect;
}

static void unregisterEffect(Effect * effect)
{
	for (auto i = g_effectsByName.begin(); i != g_effectsByName.end(); ++i)
	{
		if (i->second == effect)
		{
			g_effectsByName.erase(i);
			break;
		}
	}
}

static Effect * getEffect(const char * name)
{
	auto i = g_effectsByName.find(name);

	if (i == g_effectsByName.end())
		return nullptr;
	else
		return i->second;
}

//

struct ParticleSystem : Effect
{
	int numParticles;

	Array<int> freeList;
	int numFree;

	Array<bool> alive;
	Array<bool> autoKill;

	Array<float> x;
	Array<float> y;
	Array<float> vx;
	Array<float> vy;
	Array<float> sx;
	Array<float> sy;
	Array<float> angle;
	Array<float> vangle;
	Array<float> life;
	Array<float> lifeRcp;
	Array<bool> hasLife;

	ParticleSystem(const char * name, int numElements)
		: Effect(name)
		, numParticles(0)
		, numFree(0)
	{
		resize(numElements);
	}

	virtual ~ParticleSystem()
	{
	}

	void resize(int numElements)
	{
		numParticles = numElements;

		freeList.resize(numElements, true);
		for (int i = 0; i < numElements; ++i)
			freeList[i] = i;
		numFree = numElements;

		alive.resize(numElements, true);
		autoKill.resize(numElements, true);

		x.resize(numElements, true);
		y.resize(numElements, true);
		vx.resize(numElements, true);
		vy.resize(numElements, true);
		sx.resize(numElements, true);
		sy.resize(numElements, true);
		angle.resize(numElements, true);
		vangle.resize(numElements, true);
		life.resize(numElements, true);
		lifeRcp.resize(numElements, true);
		hasLife.resize(numElements, true);
	}

	bool alloc(const bool _autoKill, float _life, int & id)
	{
		if (numFree == 0)
		{
			id = -1;

			return false;
		}
		else
		{
			id = freeList[--numFree];

			fassert(!alive[id]);

			alive[id] = true;
			autoKill[id] = _autoKill;

			x[id] = 0.f;
			y[id] = 0.f;
			vx[id] = 0.f;
			vy[id] = 0.f;
			sx[id] = 1.f;
			sy[id] = 1.f;

			angle[id] = 0.f;
			vangle[id] = 0.f;

			if (_life == 0.f)
			{
				life[id] = 1.f;
				lifeRcp[id] = 1.f;
				hasLife[id] = false;
			}
			else
			{
				life[id] = _life;
				lifeRcp[id] = 1.f / _life;
				hasLife[id] = true;
			}

			return true;
		}
	}

	void free(const int id)
	{
		if (isValidIndex(id))
		{
			fassert(alive[id]);

			alive[id] = false;

			freeList[numFree++] = id;
		}
	}

	virtual void tick(const float dt) override
	{
		for (int i = 0; i < numParticles; ++i)
		{
			if (alive[i])
			{
				if (hasLife[i])
				{
					life[i] = life[i] - dt;
				}

				if (life[i] < 0.f)
				{
					life[i] = 0.f;

					if (autoKill[i])
					{
						free(i);

						continue;
					}
				}

				x[i] += vx[i] * dt;
				y[i] += vy[i] * dt;

				angle[i] += vangle[i] * dt;
			}
		}
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		gxBegin(GL_QUADS);
		{
			for (int i = 0; i < numParticles; ++i)
			{
				if (alive[i])
				{
					const float value = life[i] * lifeRcp[i];

					gxColor4f(1.f, 1.f, 1.f, value);

					const float s = std::sinf(angle[i]);
					const float c = std::cosf(angle[i]);

					const float sx_2 = sx[i] * .5f;
					const float sy_2 = sy[i] * .5f;

					const float s_sx_2 = s * sx_2;
					const float s_sy_2 = s * sy_2;
					const float c_sx_2 = c * sx_2;
					const float c_sy_2 = c * sy_2;

					gxTexCoord2f(0.f, 1.f); gxVertex2f(x[i] + (- c_sx_2 - s_sy_2), y[i] + (+ s_sx_2 - c_sy_2));
					gxTexCoord2f(1.f, 1.f); gxVertex2f(x[i] + (+ c_sx_2 - s_sy_2), y[i] + (- s_sx_2 - c_sy_2));
					gxTexCoord2f(1.f, 0.f); gxVertex2f(x[i] + (+ c_sx_2 + s_sy_2), y[i] + (- s_sx_2 + c_sy_2));
					gxTexCoord2f(0.f, 0.f); gxVertex2f(x[i] + (- c_sx_2 + s_sy_2), y[i] + (+ s_sx_2 + c_sy_2));
				}
			}
		}
		gxEnd();
	}
};

struct Effect_Rain : Effect
{
	ParticleSystem m_particleSystem;
	Array<float> m_particleSizes;
	EffectTimer m_spawnTimer;

	Effect_Rain(const char * name, int numRainDrops)
		: Effect(name)
		, m_particleSystem(nullptr, numRainDrops)
	{
		m_particleSizes.resize(numRainDrops, true);
	}

	virtual void tick(const float dt) override
	{
		const float gravityY = 400.f;
		//const float falloff = .9f;
		const float falloff = 1.f;
		const float falloffThisTick = powf(falloff, dt);

		const Sprite sprite("rain.png");
		const float spriteSx = sprite.getWidth();
		const float spriteSy = sprite.getHeight();

		// spawn particles

		m_spawnTimer.tick(dt);

		while (m_spawnTimer.consume((1.f + config.midiGetValue(101, 1.f) * 9.f) / 1000.f))
		{
			int id;

			if (!m_particleSystem.alloc(false, 5.f, id))
				continue;

			m_particleSystem.x[id] = rand() % GFX_SX;
			m_particleSystem.y[id] = -50.f;
			m_particleSystem.vx[id] = 0.f;
			m_particleSystem.vy[id] = 0.f;
			m_particleSystem.sx[id] = 1.f;
			m_particleSystem.sy[id] = 1.f;
			//m_particleSystem.vangle[id] = 1.f;

			m_particleSizes[id] = random(.1f, 1.f) * .25f;
		}

		// update particles

		for (int i = 0; i < m_particleSystem.numParticles; ++i)
		{
			if (!m_particleSystem.alive[i])
				continue;

			// integrate gravity

			m_particleSystem.vy[i] += gravityY * dt;

			// collision and bounce

			if (m_particleSystem.y[i] > GFX_SY)
			{
				m_particleSystem.y[i] = GFX_SY;
				m_particleSystem.vy[i] *= -.5f;
			}

			// velocity falloff

			m_particleSystem.vx[i] *= falloffThisTick;
			m_particleSystem.vy[i] *= falloffThisTick;

			// size

			const float size = m_particleSystem.life[i] * m_particleSystem.lifeRcp[i] * m_particleSizes[i];
			m_particleSystem.sx[i] = size * spriteSx;
			m_particleSystem.sy[i] = size * spriteSy * m_particleSystem.vy[i] / 100.f;

			// check if the particle is dead

			if (m_particleSystem.life[i] == 0.f)
			{
				m_particleSystem.free(i);
			}
		}

		m_particleSystem.tick(dt);
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		gxSetTexture(Sprite("rain.png").getTexture());
		{
			m_particleSystem.draw();
		}
		gxSetTexture(0);
	}
};

struct Effect_StarCluster : Effect
{
	ParticleSystem m_particleSystem;

	Effect_StarCluster(const char * name, int numStars)
		: Effect(name)
		, m_particleSystem(nullptr, numStars)
	{
		for (int i = 0; i < numStars; ++i)
		{
			int id;

			if (m_particleSystem.alloc(false, 0.f, id))
			{
				const float angle = random(0.f, pi2);
				const float radius = random(10.f, 200.f);
				//const float arcSpeed = radius / 10.f;
				const float arcSpeed = radius / 1.f;

				m_particleSystem.x[id] = cosf(angle) * radius;
				m_particleSystem.y[id] = sinf(angle) * radius;
				//m_particleSystem.vx[id] = cosf(angle + pi2/4.f) * arcSpeed;
				//m_particleSystem.vy[id] = sinf(angle + pi2/4.f) * arcSpeed;
				m_particleSystem.vx[id] = random(-arcSpeed, +arcSpeed);
				m_particleSystem.vy[id] = random(-arcSpeed, +arcSpeed);
				m_particleSystem.sx[id] = 10.f;
				m_particleSystem.sy[id] = 10.f;
			}
		}
	}

	virtual void tick(const float dt) override
	{
		// affect stars based on force from center

		for (int i = 0; i < m_particleSystem.numParticles; ++i)
		{
			if (!m_particleSystem.alive[i])
				continue;

			const float dx = m_particleSystem.x[i];
			const float dy = m_particleSystem.y[i];
			const float ds = sqrtf(dx * dx + dy * dy) + eps;

		#if 0
			const float as = 100.f;
			const float ax = - dx / ds * as;
			const float ay = - dy / ds * as;
		#else
			const float ax = - dx;
			const float ay = - dy;
		#endif

			m_particleSystem.vx[i] += ax * dt;
			m_particleSystem.vy[i] += ay * dt;

			const float size = ds / 10.f;
			m_particleSystem.sx[i] = size;
			m_particleSystem.sy[i] = size;
		}

		m_particleSystem.tick(dt);
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		gxSetTexture(Sprite("prayer.png").getTexture());
		{
			m_particleSystem.draw();
		}
		gxSetTexture(0);
	}
};

#define CLOTHPIECE_MAX_SX 16
#define CLOTHPIECE_MAX_SY 16

struct ClothPiece : Effect
{
	struct Vertex
	{
		bool isFixed;
		float x;
		float y;
		float vx;
		float vy;

		float baseX;
		float baseY;
	};

	int sx;
	int sy;
	Vertex vertices[CLOTHPIECE_MAX_SX][CLOTHPIECE_MAX_SY];

	ClothPiece(const char * name)
		: Effect(name)
	{
		sx = 0;
		sy = 0;

		memset(vertices, 0, sizeof(vertices));
	}

	void setup(int _sx, int _sy)
	{
		sx = _sx;
		sy = _sy;

		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				Vertex & v = vertices[x][y];

				//v.isFixed = (y == 0) || (y == sy - 1);
				v.isFixed = (y == 0);

				v.x = x;
				v.y = y;
				v.vx = 0.f;
				v.vy = 0.f;

				v.baseX = v.x;
				v.baseY = v.y;
			}
		}
	}

	Vertex * getVertex(int x, int y)
	{
		if (x >= 0 && x < sx && y >= 0 && y < sy)
			return &vertices[x][y];
		else
			return nullptr;
	}

	virtual void tick(const float dt) override
	{
		const float gravityX = 0.f;
		const float gravityY = keyboard.isDown(SDLK_g) ? 10.f : 0.f;

		const float springConstant = 100.f;
		const float falloff = .9f;
		//const float falloff = .3f;
		const float falloffThisTick = powf(1.f - falloff, dt);
		const float rigidity = .9f;
		const float rigidityThisTick = powf(1.f - rigidity, dt);

		// update constraints

		const int offsets[4][2] =
		{
			{ -1, 0 },
			{ +1, 0 },
			{ 0, -1 },
			{ 0, +1 }
		};

		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				Vertex & v = vertices[x][y];

				if (v.isFixed)
					continue;

				for (int i = 0; i < 4; ++i)
				{
					const Vertex * other = getVertex(x + offsets[i][0], y + offsets[i][1]);

					if (other == nullptr)
						continue;

					const float dx = other->x - v.x;
					const float dy = other->y - v.y;
					const float ds = sqrtf(dx * dx + dy * dy);

					if (ds > 1.f)
					{
						const float a = (ds - 1.f) * springConstant;

						const float ax = gravityX + dx / (ds + eps) * a;
						const float ay = gravityY + dy / (ds + eps) * a;

						v.vx += ax * dt;
						v.vy += ay * dt;
					}
				}
			}
		}

		// integrate velocity

		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				Vertex & v = vertices[x][y];

				if (v.isFixed)
					continue;

				v.x += v.vx * dt;
				v.y += v.vy * dt;

				v.x = v.x * rigidityThisTick + v.baseX * (1.f - rigidityThisTick);
				v.y = v.y * rigidityThisTick + v.baseY * (1.f - rigidityThisTick);

				v.vx *= falloffThisTick;
				v.vy *= falloffThisTick;
			}
		}
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	// todo : make the transform a part of the drawable or effect

	virtual void draw() override
	{
		gxPushMatrix();
		{
			gxScalef(40.f, 40.f, 1.f);

			//for (int i = 3; i >= 1; --i)
			for (int i = 1; i >= 1; --i)
			{
				glLineWidth(i);
				doDraw();
			}
		}
		gxPopMatrix();
	}

	void doDraw()
	{
		gxColor4f(1.f, 1.f, 1.f, .2f);

		gxBegin(GL_LINES);
		{
			for (int x = 0; x < sx - 1; ++x)
			{
				for (int y = 0; y < sy - 1; ++y)
				{
					const Vertex & v00 = vertices[x + 0][y + 0];
					const Vertex & v10 = vertices[x + 1][y + 0];
					const Vertex & v11 = vertices[x + 1][y + 1];
					const Vertex & v01 = vertices[x + 0][y + 1];

					gxVertex2f(v00.x, v00.y);
					gxVertex2f(v10.x, v10.y);

					gxVertex2f(v10.x, v10.y);
					gxVertex2f(v11.x, v11.y);

					gxVertex2f(v11.x, v11.y);
					gxVertex2f(v01.x, v01.y);

					gxVertex2f(v01.x, v01.y);
					gxVertex2f(v00.x, v00.y);
				}
			}
		}
		gxEnd();
	}
};

struct SpriteSystem : Effect
{
	static const int kMaxSprites = 128;

	struct SpriteInfo
	{
		SpriteInfo()
			: alive(false)
		{
		}

		bool alive;
		std::string filename;
		float z;
		SpriterState spriterState;
	};

	struct SpriteDrawable : Drawable
	{
		SpriteInfo * m_spriteInfo;

		SpriteDrawable(SpriteInfo * spriteInfo)
			: Drawable(spriteInfo->z)
			, m_spriteInfo(spriteInfo)
		{
		}

		virtual void draw() override
		{
			setColorf(1.f, 1.f, 1.f, 1.f);

			Spriter(m_spriteInfo->filename.c_str()).draw(m_spriteInfo->spriterState);
		}
	};

	SpriteInfo m_sprites[kMaxSprites];

	SpriteSystem(const char * name)
		: Effect(name)
	{
	}

	virtual void tick(const float dt) override
	{
		for (int i = 0; i < kMaxSprites; ++i)
		{
			SpriteInfo & s = m_sprites[i];

			if (!s.alive)
				continue;

			if (s.spriterState.updateAnim(Spriter(s.filename.c_str()), dt))
			{
				// the animation is done. clear the sprite

				s = SpriteInfo();
			}
		}
	}

	virtual void draw(DrawableList & list) override
	{
		for (int i = 0; i < kMaxSprites; ++i)
		{
			SpriteInfo & s = m_sprites[i];

			if (!s.alive)
				continue;

			new (list) SpriteDrawable(&s);
		}
	}

	virtual void draw() override
	{
		// nop
	}

	void addSprite(const char * filename, const int animIndex, const float x, const float y, const float z, const float scale)
	{
		for (int i = 0; i < kMaxSprites; ++i)
		{
			SpriteInfo & s = m_sprites[i];

			if (s.alive)
				continue;

			s.alive = true;
			s.filename = filename;
			s.spriterState.x = x;
			s.spriterState.y = y;
			s.spriterState.scale = scale;

			s.spriterState.startAnim(Spriter(filename), animIndex);

			return;
		}

		logWarning("failed to find a free sprite! cannot play %s", filename);
	}
};

struct Effect_Boxes : Effect
{
	struct Box
	{
		TweenFloat m_tx;
		TweenFloat m_ty;
		TweenFloat m_tz;

		TweenFloat m_sx;
		TweenFloat m_sy;
		TweenFloat m_sz;

		TweenFloat m_rx;
		TweenFloat m_ry;
		TweenFloat m_rz;

		int m_axis;

		bool tick(const float dt)
		{
			m_tx.tick(dt);
			m_ty.tick(dt);
			m_tz.tick(dt);

			m_sx.tick(dt);
			m_sy.tick(dt);
			m_sz.tick(dt);

			m_rx.tick(dt);
			m_ry.tick(dt);
			m_rz.tick(dt);

			return true;
		}
	};

	std::list<Box> m_boxes;

	Effect_Boxes(const char * name)
		: Effect(name)
	{
	}

	Box * addBox(
		const float tx, const float ty, const float tz,
		const float sx, const float sy, const float sz,
		const int axis)
	{
		m_boxes.push_back(Box());

		Box & b = m_boxes.back();

		b.m_tx = tx;
		b.m_ty = ty;
		b.m_tz = tz;

		b.m_sx = sx;
		b.m_sy = sy;
		b.m_sz = sz;

		b.m_axis = axis;

		return &b;
	}

	virtual void tick(const float dt) override
	{
		for (auto i = m_boxes.begin(); i != m_boxes.end(); )
		{
			Box & b = *i;

			if (!b.tick(dt))
			{
				i = m_boxes.erase(i);
			}
			else
			{
				++i;
			}
		}
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		Light lights[kMaxLights];

		lights[0].setup(kLightType_Omni, 0.f, 0.f, 0.f, .25f, .5f, 1.f, config.midiGetValue(100, 1.f) * 5.f);
		//lights[1].setup(kLightType_Omni, -1.f, 0.f, 0.f, 1.f, 1.f, .125f, config.midiGetValue(100, 1.f) * 5.f);

		Shader shader("basic_lit");
		setShader(shader);

		ShaderBuffer buffer;
		buffer.setData(lights, sizeof(lights));
		shader.setBuffer("lightsBlock", buffer);

		for (auto i = m_boxes.begin(); i != m_boxes.end(); ++i)
		{
			Box & b = *i;

			setColor(colorWhite);

			gxPushMatrix();
			{
				gxTranslatef(b.m_tx, b.m_ty, b.m_tz);

				gxRotatef(b.m_rx * Calc::rad2deg, 1.f, 0.f, 0.f);
				gxRotatef(b.m_ry * Calc::rad2deg, 0.f, 1.f, 0.f);
				gxRotatef(b.m_rz * Calc::rad2deg, 0.f, 0.f, 1.f);

				gxScalef(b.m_sx, b.m_sy, b.m_sz);

				gxBegin(GL_QUADS);
				{
					gxNormal3f( 0.f,  0.f, -1.f);
					gxVertex3f(-1.f, -1.f, -1.f);
					gxVertex3f(+1.f, -1.f, -1.f);
					gxVertex3f(+1.f, +1.f, -1.f);
					gxVertex3f(-1.f, +1.f, -1.f);

					gxNormal3f( 0.f,  0.f, +1.f);
					gxVertex3f(-1.f, -1.f, +1.f);
					gxVertex3f(+1.f, -1.f, +1.f);
					gxVertex3f(+1.f, +1.f, +1.f);
					gxVertex3f(-1.f, +1.f, +1.f);

					gxNormal3f(-1.f,  0.f,  0.f);
					gxVertex3f(-1.f, -1.f, -1.f);
					gxVertex3f(-1.f, +1.f, -1.f);
					gxVertex3f(-1.f, +1.f, +1.f);
					gxVertex3f(-1.f, -1.f, +1.f);

					gxNormal3f(+1.f,  0.f,  0.f);
					gxVertex3f(+1.f, -1.f, -1.f);
					gxVertex3f(+1.f, +1.f, -1.f);
					gxVertex3f(+1.f, +1.f, +1.f);
					gxVertex3f(+1.f, -1.f, +1.f);
				}
				gxEnd();
			}
			gxPopMatrix();
		}

		clearShader();
	}
};

struct Effect_Video : Effect
{
	std::string m_filename;
	TweenFloat m_x;
	TweenFloat m_y;
	TweenFloat m_scale;
	bool m_centered;

	MediaPlayer m_mediaPlayer;

	Effect_Video(const char * name)
		: Effect(name)
		, m_x(0.f)
		, m_y(0.f)
		, m_scale(1.f)
		, m_centered(true)
	{
		addVar("x", m_x);
		addVar("y", m_y);
		addVar("scale", m_scale);
	}

	void setup(const char * filename, float x, float y, float scale, bool centered)
	{
		m_filename = filename;
		m_x = x;
		m_y = y;
		m_scale = scale;
		m_centered = centered;

		if (!m_mediaPlayer.open(filename))
		{
			logWarning("failed to open %s", filename);
		}
	}

	virtual void tick(const float dt) override
	{
		m_x.tick(dt);
		m_y.tick(dt);
		m_scale.tick(dt);

		m_mediaPlayer.tick(dt);

		if (!m_mediaPlayer.isActive())
		{
			m_mediaPlayer.close();
		}
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		if (m_mediaPlayer.texture)
		{
			gxPushMatrix();
			{
				const int sx = m_mediaPlayer.sx;
				const int sy = m_mediaPlayer.sy;
				const float scaleX = SCREEN_SX / float(sx);
				const float scaleY = SCREEN_SY / float(sy);
				const float scale = Calc::Min(scaleX, scaleY);

				gxTranslatef(virtualToScreenX(m_x), virtualToScreenY(m_y), 0.f);
				gxScalef(m_scale, m_scale, 1.f);
				gxScalef(scale, scale, 1.f);
				if (m_centered)
					gxTranslatef(-sx/2.f, -sy/2.f, 0.f);

				setColor(colorWhite);
				m_mediaPlayer.draw();
			}
			gxPopMatrix();
		}
	}
};

//

SceneEffect::SceneEffect()
	: m_effect(nullptr)
{
}

SceneEffect::~SceneEffect()
{
	delete m_effect;
	m_effect = nullptr;
}

bool SceneEffect::load(const XMLElement * xmlEffect)
{
	m_name = stringAttrib(xmlEffect, "name", "");

	const std::string type = stringAttrib(xmlEffect, "type", "");

	if (type == "rain")
	{
		const int numRaindrops = intAttrib(xmlEffect, "num_raindrops", 0);

		if (numRaindrops == 0)
		{
			logWarning("num_raindrops is 0. skipping effect");
		}
		else
		{
			m_effect = new Effect_Rain(m_name.c_str(), numRaindrops);
		}

		return true;
	}
	else
	{
		logError("unknown effect type: %s", type.c_str());
		return false;
	}
}

SceneLayer::SceneLayer()
	: m_blendMode(kBlendMode_Add)
	, m_autoClear(true)
	, m_opacity(1.f)
{
	addVar("opacity", m_opacity);
}

SceneLayer::~SceneLayer()
{
	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		delete effect;
	}

	m_effects.clear();
}

void SceneLayer::load(const XMLElement * xmlLayer)
{
	m_name = stringAttrib(xmlLayer, "name", "");

	std::string blend = stringAttrib(xmlLayer, "blend", "add");

	m_blendMode = kBlendMode_Add;

	if (blend == "add")
		m_blendMode = kBlendMode_Add;
	else if (blend == "subtract")
		m_blendMode = kBlendMode_Subtract;
	else if (blend == "alpha")
		m_blendMode = kBlendMode_Alpha;
	else if (blend == "opaque")
		m_blendMode = kBlendMode_Opaque;
	else
		logWarning("unknown blend type: %s", blend.c_str());

	m_autoClear = boolAttrib(xmlLayer, "auto_clear", true);

	//

	for (const XMLElement * xmlEffect = xmlLayer->FirstChildElement("effect"); xmlEffect; xmlEffect = xmlEffect->NextSiblingElement("effect"))
	{
		SceneEffect * effect = new SceneEffect();

		if (!effect->load(xmlEffect))
		{
			delete effect;
			effect = nullptr;
		}
		else
		{
			m_effects.push_back(effect);
		}
	}
}

void SceneLayer::tick(const float dt)
{
	TweenFloatCollection::tick(dt);

	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		effect->m_effect->tick(dt);
	}
}

void SceneLayer::draw(DrawableList & drawableList)
{
	struct SceneLayerDrawable : Drawable
	{
		SceneLayer * m_layer;

		SceneLayerDrawable(float z, SceneLayer * layer)
			: Drawable(z)
			, m_layer(layer)
		{
		}

		virtual void draw() override
		{
			m_layer->draw();
		}
	};

	new (drawableList) SceneLayerDrawable(0.f, this);
}

void SceneLayer::draw()
{
	// todo : compose

	setColorf(1.f, 1.f, 1.f, m_opacity);
	drawRect(0, 0, GFX_SX, GFX_SY);

	DrawableList drawableList;

	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		effect->m_effect->draw(drawableList);
	}

	drawableList.sort();
	drawableList.draw();
}

SceneAction::SceneAction()
	: m_type(kActionType_None)
{
}

bool SceneAction::load(const XMLElement * xmlAction)
{
	const std::string type = xmlAction->Name();

	if (type == "tween")
	{
		m_type = kActionType_Tween;

		const std::string layer = stringAttrib(xmlAction, "layer", "");
		const std::string effect = stringAttrib(xmlAction, "effect", "");

		if (!layer.empty() + !effect.empty() > 1)
		{
			logError("more than one target type set on effect action!");
			return false;
		}

		if (!layer.empty())
		{
			m_tween.m_targetType = Tween::kTargetType_Layer;
			m_tween.m_targetName = layer;
		}
		else if (!effect.empty())
		{
			m_tween.m_targetType = Tween::kTargetType_Effect;
			m_tween.m_targetName = effect;
		}

		m_tween.m_varName = stringAttrib(xmlAction, "var", "");
		m_tween.m_tweenTo = floatAttrib(xmlAction, "to", 0.f);
		m_tween.m_tweenTime = floatAttrib(xmlAction, "time", 1.f);

		const std::string easeType = stringAttrib(xmlAction, "ease", "linear");
		m_tween.m_easeType = kEaseType_Linear;
		if (easeType == "linear")
			m_tween.m_easeType = kEaseType_Linear;
		else if (easeType == "pow_in")
			m_tween.m_easeType = kEaseType_PowIn;
		else if (easeType == "pow_out")
			m_tween.m_easeType = kEaseType_PowOut;
		else
			logError("unknown ease type: %s", easeType.c_str());
		m_tween.m_easeParam = floatAttrib(xmlAction, "ease_param", 1.f);
		m_tween.m_replaceTween = boolAttrib(xmlAction, "replace", false);

		return true;
	}
	else
	{
		logError("unknown event action: %s", type.c_str());
		return false;
	}
}

SceneEvent::SceneEvent()
{
}

SceneEvent::~SceneEvent()
{
	for (auto i = m_actions.begin(); i != m_actions.end(); ++i)
	{
		SceneAction * action = *i;

		delete action;
	}

	m_actions.clear();
}

void SceneEvent::execute(Scene & scene)
{
	for (auto i = m_actions.begin(); i != m_actions.end(); ++i)
	{
		SceneAction * action = *i;

		switch (action->m_type)
		{
		case SceneAction::kActionType_Tween:
			{
				TweenFloatCollection * varCollection = nullptr;

				switch (action->m_tween.m_targetType)
				{
				case SceneAction::Tween::kTargetType_Layer:
					{
						for (auto l = scene.m_layers.begin(); l != scene.m_layers.end(); ++l)
						{
							SceneLayer * layer = *l;

							if (layer->m_name == action->m_tween.m_targetName)
							{
								varCollection = layer;
							}
						}
					}
					break;

				case SceneAction::Tween::kTargetType_Effect:
					{
					}
					break;

				default:
					Assert(false);
					break;
				}

				if (varCollection == 0)
				{
					logWarning("couldn't find tween target by name. name=%s", action->m_tween.m_targetName.c_str());
				}
				else
				{
					TweenFloat * var = varCollection->getVar(action->m_tween.m_varName.c_str());

					if (var == 0)
					{
						logWarning("couldn't find tween value by name. name=%s", action->m_tween.m_varName.c_str());
					}
					else
					{
						var->to(
							action->m_tween.m_tweenTo,
							action->m_tween.m_tweenTime,
							action->m_tween.m_easeType,
							action->m_tween.m_easeParam);
					}
				}
			}
			break;

		default:
			Assert(false);
			break;
		}
	}
}

void SceneEvent::load(const XMLElement * xmlEvent)
{
	m_name = stringAttrib(xmlEvent, "name", "");

	for (const XMLElement * xmlAction = xmlEvent->FirstChildElement(); xmlAction; xmlAction = xmlAction->NextSiblingElement())
	{
		SceneAction * action = new SceneAction();

		if (!action->load(xmlAction))
		{
			delete action;
		}
		else
		{
			m_actions.push_back(action);
		}
	}
}

Scene::Scene()
{
}

Scene::~Scene()
{
	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		delete layer;
	}

	m_layers.clear();

	//

	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		delete event;
	}

	m_events.clear();
}

void Scene::tick(const float dt)
{
	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		layer->tick(dt);
	}
}

void Scene::draw(DrawableList & drawableList)
{
	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		layer->draw(drawableList);
	}
}

void Scene::triggerEvent(const char * name)
{
	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		if (event->m_name == name)
		{
			event->execute(*this);
		}
	}
}

bool Scene::load(const char * filename)
{
	bool result = true;

	tinyxml2::XMLDocument xmlDoc;

	if (xmlDoc.LoadFile(filename) != XML_NO_ERROR)
	{
		logError("failed to load %s", filename);

		result = false;
	}
	else
	{
		const XMLElement * xmlScene = xmlDoc.FirstChildElement("scene");

		if (xmlScene == 0)
		{
			logError("missing <scene> element");

			result = false;
		}
		else
		{
			m_name = stringAttrib(xmlScene, "name", "");

			if (m_name.empty())
			{
				logWarning("scene name not set!");
			}

			//

			const XMLElement * xmlLayers = xmlScene->FirstChildElement("layers");

			if (xmlLayers == 0)
			{
				logWarning("no layers found in scene!");
			}
			else
			{
				for (const XMLElement * xmlLayer = xmlLayers->FirstChildElement("layer"); xmlLayer; xmlLayer = xmlLayer->NextSiblingElement("layer"))
				{
					SceneLayer * layer = new SceneLayer();

					layer->load(xmlLayer);

					m_layers.push_back(layer);
				}
			}

			//

			const XMLElement * xmlEvents = xmlScene->FirstChildElement("events");

			if (xmlEvents == 0)
			{
				logDebug("scene doesn't have any events");
			}
			else
			{
				for (const XMLElement * xmlEvent = xmlEvents->FirstChildElement("event"); xmlEvent; xmlEvent = xmlEvent->NextSiblingElement("event"))
				{
					SceneEvent * event = new SceneEvent();

					event->load(xmlEvent);

					m_events.push_back(event);
				}
			}
		}

		//

		const XMLElement * xmlMidi = xmlScene->FirstChildElement("midi");

		if (xmlMidi == 0)
		{
			logDebug("scene doesn't have any MIDI mappings");
		}
		else
		{
			for (const XMLElement * xmlMidiMap = xmlMidi->FirstChildElement("map"); xmlMidiMap; xmlMidiMap = xmlMidiMap->NextSiblingElement("map"))
			{
			}
		}
	}

	return result;
}

//

static CRITICAL_SECTION s_oscMessageMtx;
static HANDLE s_oscMessageThread = INVALID_HANDLE_VALUE;

enum OscMessageType
{
	kOscMessageType_None,
	// scene :: constantly reinforced
	kOscMessageType_SetScene,
	// visual effects
	kOscMessageType_Box3D,
	kOscMessageType_Sprite,
	kOscMessageType_Video,
	// time effect
	kOscMessageType_TimeDilation,
	// sensors
	kOscMessageType_Swipe
};

struct OscMessage
{
	OscMessage()
		: type(kOscMessageType_None)
	{
		memset(param, 0, sizeof(param));
	}

	OscMessageType type;
	float param[4];
	std::string str;
};

static std::list<OscMessage> s_oscMessages;

class MyOscPacketListener : public osc::OscPacketListener
{
protected:
	virtual void ProcessMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint)
	{
		try
		{
			osc::ReceivedMessageArgumentStream args = m.ArgumentStream();

			OscMessage message;

			if (strcmp(m.AddressPattern(), "/box") == 0)
			{
				// width, angle1, angle2
				message.type = kOscMessageType_Sprite;
				const char * str;
				args >> str >> message.param[0] >> message.param[1] >> message.param[2];
				message.str = str;
			}
			else if (strcmp(m.AddressPattern(), "/sprite") == 0)
			{
				// filename, x, y, scale
				message.type = kOscMessageType_Sprite;
				const char * str;
				args >> str >> message.param[0] >> message.param[1] >> message.param[2];
				message.str = str;
			}
			else if (strcmp(m.AddressPattern(), "/video") == 0)
			{
				// filename, x, y, scale
				message.type = kOscMessageType_Video;
				const char * str;
				args >> str >> message.param[0] >> message.param[1] >> message.param[2];
				message.str = str;
			}
			else if (strcmp(m.AddressPattern(), "/timedilation") == 0)
			{
				// filename, x, y, scale
				message.type = kOscMessageType_TimeDilation;
				args >> message.param[0] >> message.param[1];
			}
			else
			{
				logWarning("unknown message type: %s", m.AddressPattern());
			}

			if (message.type != kOscMessageType_None)
			{
				EnterCriticalSection(&s_oscMessageMtx);
				{
					s_oscMessages.push_back(message);
				}
				LeaveCriticalSection(&s_oscMessageMtx);
			}
		}
		catch (osc::Exception & e)
		{
			logError("error while parsing message: %s: %s", m.AddressPattern(), e.what());
		}
	}
};

static MyOscPacketListener s_oscListener;
UdpListeningReceiveSocket * s_oscReceiveSocket = nullptr;

static DWORD WINAPI ExecuteOscThread(LPVOID pParam)
{
	s_oscReceiveSocket = new UdpListeningReceiveSocket(IpEndpointName(IpEndpointName::ANY_ADDRESS, OSC_RECV_PORT), &s_oscListener);
	s_oscReceiveSocket->Run();
	return 0;
}

struct TimeDilationEffect
{
	TimeDilationEffect()
	{
		memset(this, 0, sizeof(*this));
	}

	float duration;
	float durationRcp;
	float multiplier;
};

struct Camera
{
	Mat4x4 worldToCamera;
	Mat4x4 cameraToWorld;
	Mat4x4 cameraToView;
	float fovX;

	void setup(Vec3Arg position, Vec3 * screenCorners, int numScreenCorners, int cameraIndex)
	{
		Mat4x4 lookatMatrix;

		{
			const Vec3 d1a = screenCorners[0] - position;
			const Vec3 d2a = screenCorners[1] - position;
			const Vec3 d1 = Vec3(d1a[0], 0.f, d1a[2]).CalcNormalized();
			const Vec3 d2 = Vec3(d2a[0], 0.f, d2a[2]).CalcNormalized();
			const Vec3 d = ((d1 + d2) / 2.f).CalcNormalized();

			const Vec3 upVector(0.f, 1.f, 0.f);

			lookatMatrix.MakeLookat(position, position + d, upVector);
		}

		Mat4x4 invLookatMatrix = lookatMatrix.CalcInv();

		Vec3 * screenCornersInCameraSpace = (Vec3*)alloca(numScreenCorners * sizeof(Vec3));

		for (int i = 0; i < numScreenCorners; ++i)
			screenCornersInCameraSpace[i] = lookatMatrix * screenCorners[i];

		float fovX;

		{
			const Vec3 d1a = screenCornersInCameraSpace[0];
			const Vec3 d2a = screenCornersInCameraSpace[1];
			const Vec3 d1 = Vec3(d1a[0], 0.f, d1a[2]).CalcNormalized();
			const Vec3 d2 = Vec3(d2a[0], 0.f, d2a[2]).CalcNormalized();
			const float dot = d1 * d2;
			fovX = acosf(dot);
		}

		float fovY;

		{
			const Vec3 d1a = screenCornersInCameraSpace[cameraIndex == 0 ? 1 : 0];
			const Vec3 d2a = screenCornersInCameraSpace[cameraIndex == 0 ? 2 : 3];
			const Vec3 d1 = Vec3(0.f, d1a[1], d1a[2]).CalcNormalized();
			const Vec3 d2 = Vec3(0.f, d2a[1], d2a[2]).CalcNormalized();
			const float dot = d1 * d2;
			fovY = acosf(dot);
		}

		// calculate horizontal and vertical fov and setup projection matrix

		const float aspect = cosf(fovX) / cosf(fovY);

		Mat4x4 projection;
		projection.MakePerspectiveLH(fovY, aspect, .001f, 10.f);

		//

		cameraToWorld = invLookatMatrix;
		worldToCamera = lookatMatrix;
		cameraToView = projection;
	}

	void beginView(int c, int & sx, int & sy) const
	{
		gxMatrixMode(GL_PROJECTION);
		gxPushMatrix();
		gxLoadMatrixf(cameraToView.m_v);
		glMultMatrixf(worldToCamera.m_v);

		gxMatrixMode(GL_MODELVIEW);
		gxPushMatrix();
		const int x1 = virtualToScreenX(-150 + (c + 0) * 100);
		const int y1 = virtualToScreenY(-50);
		const int x2 = virtualToScreenX(-150 + (c + 1) * 100);
		const int y2 = virtualToScreenY(+50);

		sx = x2 - x1;
		sy = y2 - y1;

		glViewport(
			x1 / framework.minification,
			y1 / framework.minification,
			sx / framework.minification,
			sy / framework.minification);
	}

	void endView() const
	{
		gxMatrixMode(GL_PROJECTION);
		gxPopMatrix();
		gxMatrixMode(GL_MODELVIEW);
		gxPopMatrix();

		// dirty hack to restore viewport

		pushSurface(nullptr);
		popSurface();
	}
};

static void drawTestObjects()
{
	for (int k = 0; k < 3; ++k)
	{
		gxPushMatrix();
		{
			const float dx = sinf(framework.time / (k + 4.f));
			const float dy = sinf(framework.time / (k + 8.f)) / 2.f;
			const float dz = cosf(framework.time / (k + 4.f) * 2.f) * 2.f;

			gxTranslatef(
				dx + (k - 1) / 1.1f,
				dy + .5f + (k - 1) / 4.f, dz + 2.f);
			gxScalef(.5f, .5f, .5f);
			gxRotatef(framework.time / (k + 1.123f) * Calc::rad2deg, 1.f, 0.f, 0.f);
			gxRotatef(framework.time / (k + 1.234f) * Calc::rad2deg, 0.f, 1.f, 0.f);
			gxRotatef(framework.time / (k + 1.345f) * Calc::rad2deg, 0.f, 0.f, 1.f);

			gxBegin(GL_TRIANGLES);
			{
				gxColor4f(1.f, 0.f, 0.f, 1.f); gxVertex3f(-1.f,  0.f,  0.f); gxVertex3f(+1.f,  0.f,  0.f); gxVertex3f(+1.f, 0.3f, 0.2f);
				gxColor4f(0.f, 1.f, 0.f, 1.f); gxVertex3f( 0.f, -1.f,  0.f); gxVertex3f( 0.f, +1.f,  0.f); gxVertex3f(0.4f, +1.f, 0.4f);
				gxColor4f(0.f, 0.f, 1.f, 1.f); gxVertex3f( 0.f,  0.f, -1.f); gxVertex3f( 0.f,  0.f, +1.f); gxVertex3f(0.1f, 0.2f, +1.f);
			}
			gxEnd();
		}
		gxPopMatrix();
	}
}

static void drawGroundPlane(const float y)
{
	gxColor4f(.2f, .2f, .2f, 1.f);
	gxBegin(GL_QUADS);
	{
		gxVertex3f(-100.f, y, -100.f);
		gxVertex3f(+100.f, y, -100.f);
		gxVertex3f(+100.f, y, +100.f);
		gxVertex3f(-100.f, y, +100.f);
	}
	gxEnd();
}

static void drawCamera(const Camera & camera, const float alpha)
{
	// draw local axis

	gxMatrixMode(GL_MODELVIEW);
	gxPushMatrix();
	{
		glMatrixMultfEXT(GL_MODELVIEW, camera.cameraToWorld.m_v);

		gxPushMatrix();
		{
			gxScalef(.2f, .2f, .2f);
			gxBegin(GL_LINES);
			{
				gxColor4f(1.f, 0.f, 0.f, alpha); gxVertex3f(0.f, 0.f, 0.f); gxVertex3f(1.f, 0.f, 0.f);
				gxColor4f(0.f, 1.f, 0.f, alpha); gxVertex3f(0.f, 0.f, 0.f); gxVertex3f(0.f, 1.f, 0.f);
				gxColor4f(0.f, 0.f, 1.f, alpha); gxVertex3f(0.f, 0.f, 0.f); gxVertex3f(0.f, 0.f, 1.f);
			}
			gxEnd();
		}
		gxPopMatrix();

		const Mat4x4 invView = camera.cameraToView.CalcInv();
		
		const Vec3 p[5] =
		{
			invView * Vec3(-1.f, -1.f, 0.f),
			invView * Vec3(+1.f, -1.f, 0.f),
			invView * Vec3(+1.f, +1.f, 0.f),
			invView * Vec3(-1.f, +1.f, 0.f),
			invView * Vec3( 0.f,  0.f, 0.f)
		};

		gxPushMatrix();
		{
			gxScalef(1.f, 1.f, 1.f);
			gxBegin(GL_LINES);
			{
				for (int i = 0; i < 4; ++i)
				{
					gxColor4f(1.f, 1.f, 1.f, alpha);
					gxVertex3f(0.f, 0.f, 0.f);
					gxVertex3f(p[i][0], p[i][1], p[i][2]);
				}
			}
			gxEnd();
		}
		gxPopMatrix();
	}
	gxMatrixMode(GL_MODELVIEW);
	gxPopMatrix();
}

static void drawScreen(const Vec3 * screenPoints, GLuint surfaceTexture, int screenId)
{
	const bool translucent = true;

	if (translucent)
	{
		//gxColor4f(1.f, 1.f, 1.f, 1.f);
		//glDisable(GL_DEPTH_TEST);

		//setBlend(BLEND_ADD);
	}
	else
	{
		setBlend(BLEND_OPAQUE);
	}

	setColor(colorWhite);
	gxSetTexture(surfaceTexture);
	{
		gxBegin(GL_QUADS);
		{
			gxTexCoord2f(1.f / 3.f * (screenId + 0), 0.f); gxVertex3f(screenPoints[0][0], screenPoints[0][1], screenPoints[0][2]);
			gxTexCoord2f(1.f / 3.f * (screenId + 1), 0.f); gxVertex3f(screenPoints[1][0], screenPoints[1][1], screenPoints[1][2]);
			gxTexCoord2f(1.f / 3.f * (screenId + 1), 1.f); gxVertex3f(screenPoints[2][0], screenPoints[2][1], screenPoints[2][2]);
			gxTexCoord2f(1.f / 3.f * (screenId + 0), 1.f); gxVertex3f(screenPoints[3][0], screenPoints[3][1], screenPoints[3][2]);
		}
		gxEnd();
	}
	gxSetTexture(0);

	//glEnable(GL_DEPTH_TEST);
	//setBlend(BLEND_ALPHA);

	setColor(colorWhite);
	gxBegin(GL_LINE_LOOP);
	{
		for (int i = 0; i < 4; ++i)
			gxVertex3fv(&screenPoints[i][0]);
	}
	gxEnd();
}

int main(int argc, char * argv[])
{
	changeDirectory("data");

	if (!config.load("settings.xml"))
	{
		logError("failed to load settings.xml");
		return -1;
	}

	const int numMidiDevices = midiInGetNumDevs();

	if (numMidiDevices > 0)
	{
		printf("MIDI devices:\n");

		for (int i = 0; i < numMidiDevices; ++i)
		{
			MIDIINCAPS caps;
			memset(&caps, 0, sizeof(caps));

			if (midiInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
			{
				printf("device_index=%d, driver_version=%03d.%03d, name=%s\n",
					i,
					(caps.vDriverVersion & 0xff00) >> 8,
					(caps.vDriverVersion & 0x00ff) >> 0,
					caps.szPname);
			}
		}
	}

#if 0
	TweenFloat tween(10.f);
	tween.to(0.f, 1.f);
	tween.to(5.f, 1.f);
	tween.to(0.f, 1.f);
	tween.to(100.f, 1.f);

	while (!tween.isDone())
	{
		printf("tween value: %g\n", (float)tween);

		tween.tick(1.f / 9.f);
	}

	printf("tween value: %g\n", (float)tween);
#endif

	AudioIn audioIn;

	float audioInProvideTime = 0.f;
	int audioInHistoryMaxSize = 0;
	int audioInHistorySize = 0;
	short * audioInHistory = nullptr;

	if (config.audioIn.enabled)
	{
		if (!audioIn.init(config.audioIn.deviceIndex, config.audioIn.numChannels, config.audioIn.sampleRate, config.audioIn.bufferLength))
		{
			logError("failed to initialise audio in!");
		}
		else
		{
			audioInHistoryMaxSize = config.audioIn.numChannels * config.audioIn.bufferLength;
			audioInHistory = new short[audioInHistoryMaxSize];
		}
	}

	// initialise OSC

	InitializeCriticalSectionAndSpinCount(&s_oscMessageMtx, 256);

	s_oscMessageThread = CreateThread(NULL, 64 * 1024, ExecuteOscThread, NULL, CREATE_SUSPENDED, NULL);
	ResumeThread(s_oscMessageThread);

	// initialise framework

	framework.fullscreen = false;
	framework.windowBorder = false;
	framework.enableDepthBuffer = true;
	framework.minification = 2;
	framework.enableMidi = true;
	framework.midiDeviceIndex = config.midi.deviceIndex;

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		changeDirectory("C:/gg-code-hg/users/marcel/vfxpro/data");
		framework.fillCachesWithPath(".", true);
		changeDirectory("C:/Temp/cmake/vfxpro/data");
	#ifndef DEBUG
		framework.fillCachesWithPath(".", true);
	#endif

		std::list<TimeDilationEffect> timeDilationEffects;

		bool clearScreen = true;
		bool debugDraw = false;
		bool cameraControl = false;
		bool postProcess = false;

		bool drawRain = true;
		bool drawStarCluster = true;
		bool drawCloth = false;
		bool drawSprites = true;
		bool drawBoxes = true;
		bool drawVideo = true;
		bool drawPCM = true;

		bool drawHelp = true;
		bool drawScreenIds = false;
		bool drawProjectorSetup = false;
		bool drawActiveEffects = false;

		Scene * scene = new Scene();
		scene->load("scene.xml");

		scene->triggerEvent("fade_rain");

		Effect_Rain rain("rain", 10000);

		Effect_StarCluster starCluster("stars", 100);
		starCluster.screenX = virtualToScreenX(0);
		starCluster.screenY = virtualToScreenY(0);

		ClothPiece clothPiece("cloth");
		clothPiece.setup(CLOTHPIECE_MAX_SX, CLOTHPIECE_MAX_SY);

		SpriteSystem spriteSystem("sprites");

		Effect_Boxes boxes("boxes");
		for (int b = 0; b < 2; ++b)
		{
			const float boxScale = 2.5f;
			Effect_Boxes::Box * box = boxes.addBox(0.f, 0.f, 0.f, boxScale, boxScale / 8.f, boxScale, 0);
			const float boxTimeStep = 1.f;
			for (int i = 0; i < 1000; ++i)
			{
				if ((rand() % 3) <= 1)
				{
					box->m_rx.to(random(-2.f, +2.f), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
					box->m_ry.to(random(-2.f, +2.f), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
					//box->m_rz.to(random(-2.f, +2.f), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
				}
				else
				{
					box->m_rx.to(box->m_rx.getFinalValue(), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
					box->m_ry.to(box->m_ry.getFinalValue(), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
					//box->m_rz.to(box->m_rz.getFinalValue(), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
				}

				if ((rand() % 4) <= 0)
					box->m_sy.to(random(0.f, boxScale / 4.f), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
				else
					box->m_sy.to(box->m_sy.getFinalValue(), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
			}
		}

		Effect_Video video("video");
		video.setup("doa.avi", 0.f, 0.f, 1.f, true);

		video.tweenTo("scale", 3.f, 10.f, kEaseType_PowIn, 2.f);

		Surface surface(GFX_SX, GFX_SY);

	#if 0
		Shader basicLitShader("basic_lit");
		setShader(basicLitShader);
		clearShader();
		exit(0);
	#endif

		Shader jitterShader("jitter");
		Shader boxblurShader("boxblur");
		Shader luminanceShader("luminance");
		Shader flowmapShader("flowmap");
		Shader distortionBarsShader("distortion_bars");
		Shader fxaaShader("fxaa");

		Vec3 cameraPosition(0.f, .5f, -1.f);
		Vec3 cameraRotation(0.f, 0.f, 0.f);
		Mat4x4 cameraMatrix;
		cameraMatrix.MakeIdentity();

		int activeCamera = kNumScreens;

		float time = 0.f;
		float timeReal = 0.f;

		bool stop = false;

		while (!stop)
		{
			// process

			framework.process();

			// todo : process audio input

			int * samplesThisFrame = nullptr;
			int numSamplesThisFrame = 0;

			float loudnessThisFrame = 0.f;

			if (config.audioIn.enabled)
			{
				audioInHistorySize = audioInHistorySize;

				while (audioIn.provide(audioInHistory, audioInHistorySize))
				{
					//logDebug("got audio data! numSamples=%04d", audioInHistorySize);

					audioInHistorySize /= config.audioIn.numChannels;
					audioInProvideTime = framework.time;
				}

				numSamplesThisFrame = Calc::Min(config.audioIn.sampleRate / 60, audioInHistorySize);
				Assert(audioInHistorySize >= numSamplesThisFrame);

				// todo : secure this code

				samplesThisFrame = new int[numSamplesThisFrame];

				int offset = (framework.time - audioInProvideTime) * config.audioIn.sampleRate;
				Assert(offset >= 0);

				if (offset + numSamplesThisFrame > audioInHistorySize)
					offset = audioInHistorySize - numSamplesThisFrame;

				//logDebug("offset = %04d/%04d (numSamplesThisFrame=%04d)", offset, audioInHistorySize, numSamplesThisFrame);

				offset *= config.audioIn.numChannels;

				const short * __restrict src = audioInHistory + offset;
				      int   * __restrict dst = samplesThisFrame;

				for (int i = 0; i < numSamplesThisFrame; ++i)
				{
					int value = 0;

					for (int c = 0; c < config.audioIn.numChannels; ++c)
					{
						value += *src++;
					}

					*dst++ = value;

					Assert(src <= audioInHistory + audioInHistorySize * config.audioIn.numChannels);
					Assert(dst <= samplesThisFrame + numSamplesThisFrame);
				}

				// calculate loudness

				int total = 0;
				for (int i = 0; i < numSamplesThisFrame; ++i)
					total += samplesThisFrame[i];
				loudnessThisFrame = sqrtf(total / float(numSamplesThisFrame) / float(1 << 15));
			}

			// todo : process OSC input

			// todo : process direct MIDI input

			// input

			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;

			if (keyboard.wentDown(SDLK_r))
			{
				framework.reloadCaches();
				framework.reloadCachesOnActivate = true;
			}

			if (keyboard.wentDown(SDLK_a))
			{
				spriteSystem.addSprite("Diamond.scml", 0, rand() % GFX_SX, rand() % GFX_SY, 0.f, 1.f);
			}

			if (keyboard.wentDown(SDLK_c))
				clearScreen = !clearScreen;
			if (keyboard.wentDown(SDLK_d))
				debugDraw = !debugDraw;
			if (keyboard.wentDown(SDLK_RSHIFT))
				cameraControl = !cameraControl;
			if (keyboard.wentDown(SDLK_p) || config.midiWentDown(64))
				postProcess = !postProcess;

			if (keyboard.wentDown(SDLK_1))
				drawRain = !drawRain;
			if (keyboard.wentDown(SDLK_2))
				drawStarCluster = !drawStarCluster;
			if (keyboard.wentDown(SDLK_3))
				drawCloth = !drawCloth;
			if (keyboard.wentDown(SDLK_4))
				drawSprites = !drawSprites;
			if (keyboard.wentDown(SDLK_5))
				drawBoxes = !drawBoxes;
			if (keyboard.wentDown(SDLK_6))
				drawVideo = !drawVideo;
			if (keyboard.wentDown(SDLK_7))
				drawPCM = !drawPCM;

			if (keyboard.wentDown(SDLK_F1))
				drawHelp = !drawHelp;
			if (keyboard.wentDown(SDLK_i))
				drawScreenIds = !drawScreenIds;
			if (keyboard.wentDown(SDLK_s))
				drawProjectorSetup = !drawProjectorSetup;
			if (keyboard.wentDown(SDLK_e))
				drawActiveEffects = !drawActiveEffects;

			if (keyboard.wentDown(SDLK_o))
			{
				cameraPosition = Vec3(0.f, .5f, -1.f);
				cameraRotation.SetZero();
			}

			SDL_SetRelativeMouseMode(cameraControl ? SDL_TRUE : SDL_FALSE);

			const float dtReal = Calc::Min(1.f / 30.f, framework.timeStep) * config.midiGetValue(100, 1.f);

			Mat4x4 cameraPositionMatrix;
			Mat4x4 cameraRotationMatrix;

			if (cameraControl && drawProjectorSetup)
			{
				cameraRotation[0] -= mouse.dy / 100.f;
				cameraRotation[1] -= mouse.dx / 100.f;

				Vec3 speed;

				if (keyboard.isDown(SDLK_RIGHT))
					speed[0] += 1.f;
				if (keyboard.isDown(SDLK_LEFT))
					speed[0] -= 1.f;
				if (keyboard.isDown(SDLK_UP))
					speed[2] += 1.f;
				if (keyboard.isDown(SDLK_DOWN))
					speed[2] -= 1.f;

				cameraPosition += cameraMatrix.CalcInv().Mul3(speed) * dtReal;

				if (keyboard.wentDown(SDLK_END))
					activeCamera = (activeCamera + 1) % (kNumScreens + 1);
			}

			{
				Mat4x4 rotX;
				Mat4x4 rotY;
				rotX.MakeRotationX(cameraRotation[0]);
				rotY.MakeRotationY(cameraRotation[1]);
				cameraRotationMatrix = rotY * rotX;

				cameraPositionMatrix.MakeTranslation(cameraPosition);

				Mat4x4 invCameraMatrix = cameraPositionMatrix * cameraRotationMatrix;
				cameraMatrix = invCameraMatrix.CalcInv();
			}

			// update network input

			EnterCriticalSection(&s_oscMessageMtx);
			{
				while (!s_oscMessages.empty())
				{
					const OscMessage & message = s_oscMessages.front();

					switch (message.type)
					{
					case kOscMessageType_SetScene:
						break;

						//

					case kOscMessageType_Sprite:
						{
							spriteSystem.addSprite(
								"Diamond.scml",
								0,
								virtualToScreenX(message.param[0]),
								virtualToScreenY(message.param[1]),
								0.f, message.param[2] / 100.f);
							//spriteSystem.addSprite(message.str.c_str(), 0, message.param[0], message.param[1], 0.f, message.param[2]);
						}
						break;

					case kOscMessageType_Video:
						{
							video.setup(message.str.c_str(), message.param[0], message.param[1], 1.f, true);
						}
						break;

						//

					case kOscMessageType_TimeDilation:
						{
							TimeDilationEffect effect;
							effect.duration = message.param[0];
							effect.durationRcp = 1.f / effect.duration;
							effect.multiplier = message.param[1];
							timeDilationEffects.push_back(effect);
						}
						break;

						//

					case kOscMessageType_Swipe:
						break;

					default:
						fassert(false);
						break;
					}

					s_oscMessages.pop_front();
				}
			}
			LeaveCriticalSection(&s_oscMessageMtx);

			// figure out time dilation

			float timeDilationMultiplier = 1.f;

		#if 1
			for (auto i = timeDilationEffects.begin(); i != timeDilationEffects.end(); )
			{
				TimeDilationEffect & e = *i;

				const float multiplier = lerp(1.f, e.multiplier, e.duration * e.duration);

				if (multiplier < timeDilationMultiplier)
					timeDilationMultiplier = multiplier;

				e.duration = Calc::Max(0.f, e.duration - dtReal);

				if (e.duration == 0.f)
					i = timeDilationEffects.erase(i);
				else
					++i;
			}
		#endif

			const float dt = dtReal * timeDilationMultiplier;

			// process effects

			scene->tick(dt);

			rain.tick(dt);

			starCluster.tick(dt);

			for (int i = 0; i < 10; ++i)
				clothPiece.tick(dt / 10.f);

			spriteSystem.tick(dt);

			boxes.tick(dt);

			video.tick(dt);

		#if 0
			if ((rand() % 30) == 0)
			{
				clothPiece.vertices[rand() % clothPiece.sx][rand() % clothPiece.sy].vx += 20.f;
			}
		#endif

			if (mouse.isDown(BUTTON_LEFT))
			{
				const float mouseX = mouse.x / 40.f;
				const float mouseY = mouse.y / 40.f;

				for (int x = 0; x < clothPiece.sx; ++x)
				{
					const int y = clothPiece.sy - 1;

					ClothPiece::Vertex & v = clothPiece.vertices[x][y];

					const float dx = mouseX - v.x;
					const float dy = mouseY - v.y;

					const float a = x / float(clothPiece.sx - 1) * 30.f;

					v.vx += a * dx * dt;
					v.vy += a * dy * dt;
				}
			}

			// draw

			DrawableList drawableList;

			scene->draw(drawableList);

			if (drawRain)
				rain.draw(drawableList);

			if (drawStarCluster)
				starCluster.draw(drawableList);

			if (drawCloth)
				clothPiece.draw(drawableList);

			if (drawSprites)
				spriteSystem.draw(drawableList);

			if (drawVideo)
				video.draw(drawableList);

			drawableList.sort();

			framework.beginDraw(0, 0, 0, 0);
			{
				// camera setup

				Camera cameras[kNumScreens];

				const float dx = +sqrtf(2.f) / 2.f; // todo : rotate the screen instead of hacking their positions
				const float dz = -sqrtf(2.f) / 2.f; // todo : rotate the screen instead of hacking their positions

				Vec3 _screenCorners[8] =
				{
					Vec3(-0.5f - dx, 0.f,  dz),
					Vec3(-0.5f,      0.f, 0.f),
					Vec3(+0.5f,      0.f, 0.f),
					Vec3(+0.5f + dx, 0.f,  dz),

					Vec3(-0.5f - dx, 1.f,  dz),
					Vec3(-0.5f,      1.f, 0.f),
					Vec3(+0.5f,      1.f, 0.f),
					Vec3(+0.5f + dx, 1.f,  dz),
				};

				Vec3 screenCorners[kNumScreens][4];

				const Vec3 cameraPosition(0.f, .5f, -1.f);

				for (int c = 0; c < kNumScreens; ++c)
				{
					Camera & camera = cameras[c];

					screenCorners[c][0] = _screenCorners[c + 0 + 0],
					screenCorners[c][1] = _screenCorners[c + 1 + 0],
					screenCorners[c][2] = _screenCorners[c + 1 + 4],
					screenCorners[c][3] = _screenCorners[c + 0 + 4],

					camera.setup(cameraPosition, screenCorners[c], 4, c);
				}

				pushSurface(&surface);

				if (clearScreen)
				{
					glClearColor(0.f, 0.f, 0.f, 1.f);
					glClear(GL_COLOR_BUFFER_BIT);
				}
				else
				{
					// basically BLEND_SUBTRACT, but keep the alpha channel in-tact
					glEnable(GL_BLEND);
					fassert(glBlendEquation);
					if (glBlendEquation)
						glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);

					//setColorf(config.midiGetValue(102, 1.f), config.midiGetValue(102, 1.f)/2.f, config.midiGetValue(102, 1.f)/4.f, 1.f);
					setColor(2, 2, 2, 255);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}

				setBlend(BLEND_ALPHA);

			#if 1
				for (int c = 0; c < kNumScreens; ++c)
				{
					const Camera & camera = cameras[c];

					int sx;
					int sy;

					camera.beginView(c, sx, sy);
					{
						setBlend(BLEND_ADD);

						if (debugDraw)
						{
							drawTestObjects();
						}

						DrawableList drawableList;

						if (drawBoxes)
							boxes.draw(drawableList);

						drawableList.sort();

						drawableList.draw();

					#if 1
						if (drawPCM && ((c == 0) || (c == 2)) && audioInHistorySize > 0)
						{
							applyTransformWithViewportSize(sx, sy);

							setColor(colorWhite);

							glLineWidth(1.5f);
							gxBegin(GL_LINES);
							{
								const float scaleX = GFX_SX / float(numSamplesThisFrame - 2);
								const float scaleY = 300.f / float(1 << 15);

								for (int i = 0; i < numSamplesThisFrame - 1; ++i)
								{
									gxVertex2f((i + 0) * scaleX, GFX_SY/2.f + samplesThisFrame[i + 0] * scaleY);
									gxVertex2f((i + 1) * scaleX, GFX_SY/2.f + samplesThisFrame[i + 1] * scaleY);
								}
							}
							gxEnd();
							glLineWidth(1.f);
						}
					#endif

						setBlend(BLEND_ALPHA);
					}
					camera.endView();
				}
			#endif

				setBlend(BLEND_ADD);
				drawableList.draw();

				setColorf(.25f, .5f, 1.f, loudnessThisFrame / 8.f);
				drawRect(0, 0, GFX_SX, GFX_SY);

				// test effect

				if (postProcess)
				{
					setBlend(BLEND_OPAQUE);
					Shader & shader = jitterShader;
					shader.setTexture("colormap", 0, surface.getTexture(), true, false);
					shader.setTexture("jittermap", 1, 0, true, true);
					shader.setImmediate("jitterStrength", 1.f);
					shader.setImmediate("time", time);
					surface.postprocess(shader);
				}

				static volatile bool doBoxblur = false;
				static volatile bool doLuminance = false;
				static volatile bool doFlowmap = false;
				static volatile bool doDistortionBars = false;
				static volatile bool doFxaa = false;

				if (doBoxblur)
				{
					setBlend(BLEND_OPAQUE);
					Shader & shader = boxblurShader;
					setShader(shader);
					shader.setTexture("colormap", 0, surface.getTexture(), true, false);
					ShaderBuffer buffer;
					BoxblurData data;
					const float radius = 2.f;
					data.radiusX = radius * (1.f / GFX_SX);
					data.radiusY = radius * (1.f / GFX_SY);
					buffer.setData(&data, sizeof(data));
					shader.setBuffer("BoxblurBlock", buffer);
					surface.postprocess(shader);
				}

				if (doLuminance)
				{
					setBlend(BLEND_OPAQUE);
					Shader & shader = luminanceShader;
					setShader(shader);
					shader.setTexture("colormap", 0, surface.getTexture(), true, false);
					ShaderBuffer buffer;
					LuminanceData data;
					data.power = cosf(framework.time) + 1.f + config.midiGetValue(104, 1.f / 8.f) * 8.f;
					data.scale = 1.f * config.midiGetValue(103, 1.f);
					buffer.setData(&data, sizeof(data));
					shader.setBuffer("LuminanceBlock", buffer);
					surface.postprocess(shader);
				}

				if (doFlowmap)
				{
					setBlend(BLEND_OPAQUE);
					Shader & shader = flowmapShader;
					setShader(shader);
					shader.setTexture("colormap", 0, surface.getTexture(), true, false);
					shader.setTexture("flowmap", 0, surface.getTexture(), true, false); // todo
					shader.setImmediate("time", framework.time);
					ShaderBuffer buffer;
					FlowmapData data;
					data.strength = cosf(framework.time) * 200.f;
					buffer.setData(&data, sizeof(data));
					shader.setBuffer("FlowmapBlock", buffer);
					surface.postprocess(shader);
				}

				if (doDistortionBars)
				{
					setBlend(BLEND_OPAQUE);
					Shader & shader = distortionBarsShader;
					setShader(shader);
					shader.setTexture("colormap", 0, surface.getTexture(), true, false);
					ShaderBuffer buffer;
					DistortionBarsData data;
					Mat4x4 matR;
					Mat4x4 matT;
					matR.MakeRotationZ(framework.time * .1f);
					//matT.MakeTranslation(GFX_SX/2.f, GFX_SY/2.f, 0.f);
					matT.MakeTranslation(mouse.x, GFX_SY - mouse.y, 0.f);
					Mat4x4 mat = matT * matR;
					data.px = mat(0, 0);
					data.py = mat(0, 1);
					data.pd = mat(3, 0) * data.px + mat(3, 1) * data.py;
					data.pScale = .1f;
					data.qx = mat(1, 0);
					data.qy = mat(1, 1);
					data.qd = mat(3, 0) * data.qx + mat(3, 1) * data.qy;
					data.qScale = 1.f / 200.f;
					buffer.setData(&data, sizeof(data));
					shader.setBuffer("DistotionBarsBlock", buffer); // fixme : block name
					surface.postprocess(shader);
				}

				if (doFxaa)
				{
					setBlend(BLEND_OPAQUE);
					Shader & shader = fxaaShader;
					setShader(shader);
					shader.setTexture("colormap", 0, surface.getTexture(), true, true);
					shader.setImmediate("inverseVP", 1.f / (surface.getWidth() / framework.minification), 1.f / (surface.getHeight() / framework.minification));
					surface.postprocess(shader);
				}

				popSurface();

				const GLuint surfaceTexture = surface.getTexture();

			#if 1
				gxSetTexture(surfaceTexture);
				{
					setBlend(BLEND_OPAQUE);
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
					setBlend(BLEND_ALPHA);
				}
				gxSetTexture(0);
			#endif

				if (drawProjectorSetup)
				{
					glClearColor(0.05f, 0.05f, 0.05f, 0.f);
					glClear(GL_COLOR_BUFFER_BIT);

					setBlend(BLEND_ALPHA);

				#if 0 // todo : move to camera viewport rendering ?
					// draw projector bounds

					setColorf(1.f, 1.f, 1.f, .25f);
					for (int i = 0; i < kNumScreens; ++i)
						drawRectLine(virtualToScreenX(-150 + i * 100), virtualToScreenY(0.f), virtualToScreenX(-150 + (i + 1) * 100), virtualToScreenY(100));
				#endif

					// draw 3D projector setup

					//glEnable(GL_DEPTH_TEST);
					//glDepthFunc(GL_LESS);

					{
						Mat4x4 projection;
						projection.MakePerspectiveLH(90.f * Calc::deg2rad, float(GFX_SY) / float(GFX_SX), 0.01f, 100.f);
						gxMatrixMode(GL_PROJECTION);
						gxPushMatrix();
						gxLoadMatrixf(projection.m_v);
						{
							gxMatrixMode(GL_MODELVIEW);
							gxPushMatrix();
							gxLoadMatrixf(cameraMatrix.m_v);
							{
								setBlend(BLEND_ADD);

								// draw ground

								drawGroundPlane(0.f);

								// draw the projector screens

								for (int c = 0; c < kNumScreens; ++c)
								{
									drawScreen(screenCorners[c], surfaceTexture, c);
								}

								// draw test objects

								// todo : add drawScene function. this code is getting duplicated with viewport render

								if (debugDraw && keyboard.isDown(SDLK_LSHIFT))
								{
									drawTestObjects();

									DrawableList drawableList;

									if (drawBoxes)
										boxes.draw(drawableList);

									drawableList.sort();

									drawableList.draw();
								}

								// draw the cameras

								for (int c = 0; c < kNumScreens; ++c)
								{
									if (c < kNumScreens)
									{
										const Camera & camera = cameras[c];

										drawCamera(camera, c == activeCamera ? 1.f : .1f);
									}
								}

								setBlend(BLEND_ALPHA);
							}
							gxMatrixMode(GL_MODELVIEW);
							gxPopMatrix();
						}
						gxMatrixMode(GL_PROJECTION);
						gxPopMatrix();

						gxMatrixMode(GL_MODELVIEW);
					}
					glDisable(GL_DEPTH_TEST);
				}

				if (drawScreenIds)
				{
					for (int c = 0; c < kNumScreens; ++c)
					{
						const Camera & camera = cameras[c];

						int sx;
						int sy;

						camera.beginView(c, sx, sy);
						{
							setBlend(BLEND_ADD);

							applyTransformWithViewportSize(sx, sy);

							setFont("calibri.ttf");
							setColor(colorWhite);
							drawText(sx/2, sy/2, 250, 0.f, 0.f, "%d", c + 1);
						}
						camera.endView();
					}
				}

				if (drawActiveEffects)
				{
					setFont("VeraMono.ttf");
					setColor(colorWhite);
					const int spacingY = 28;
					const int fontSize = 24;
					int x = 20;
					int y = 20;
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "effects list:");
					x += 50;
					for (auto i = g_effectsByName.begin(); i != g_effectsByName.end(); ++i)
					{
						drawText(x, y += spacingY, fontSize, +1.f, +1.f, "%-40s : %p", i->first.c_str(), i->second);
					}
				}

				if (drawHelp)
				{
					setFont("calibri.ttf");
					setColor(colorWhite);
					const int spacingY = 28;
					const int fontSize = 24;
					int x = 20;
					int y = 20;
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "Press F1 to toggle help");
					x += 50;
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "1: toggle rain effect");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "2: toggle star cluster effect");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "3: toggle cloth effect");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "4: toggle sprite effects");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "5: toggle video effects");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "I: identify screens");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "S: toggle project setup view");
					x += 50;
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "RIGHT SHIFT: enable camera controls in project setup view");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "ARROW KEYS: move the camera around in project setup view");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "END: change the active virtual camera in project setup view");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "LEFT SHIFT: when pressed, draw test objects in 3D space");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "");
					x -= 50;
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "A: spawn a Spriter effect");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "G: when pressed, enables gravity on cloth");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "R: reload data caches");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "C: disable screen clear and enable a fade effect instead");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "P: toggle fullscreen shader effect");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "D: toggle debug draw");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "");
					drawText(x, y += spacingY, fontSize, +1.f, +1.f, "ESCAPE: quit");

					//

					drawText(GFX_SX/2, GFX_SY/2, 32, 0.f, 0.f, "Listening for OSC messages on port %d", OSC_RECV_PORT);

					static int easeFunction = 0;
					if (keyboard.wentDown(SDLK_SPACE))
						easeFunction = (easeFunction + 1) % kEaseType_Count;
					setColor(colorWhite);
					for (int i = 0; i <= 100; ++i)
						drawCircle(GFX_SX/2 + i, GFX_SY/2 + evalEase(i / 100.f, (EaseType)easeFunction, mouse.y / float(GFX_SY) * 2.f) * 100.f, 5.f, 4);
				}
			}

			delete [] samplesThisFrame;
			samplesThisFrame = nullptr;

			framework.endDraw();

			time += dt;
			timeReal += dtReal;
		}

		delete scene;
		scene = nullptr;

		framework.shutdown();
	}

	s_oscReceiveSocket->AsynchronousBreak();
	WaitForSingleObject(s_oscMessageThread, INFINITE);
	CloseHandle(s_oscMessageThread);

	delete s_oscReceiveSocket;
	s_oscReceiveSocket = nullptr;

	//

	delete [] audioInHistory;
	audioInHistory = nullptr;
	audioInHistorySize = 0;
	audioInHistoryMaxSize = 0;

	audioIn.shutdown();

	//

	return 0;
}
