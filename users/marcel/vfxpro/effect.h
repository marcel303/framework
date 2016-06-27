#pragma once

#include "Calc.h"
#include "config.h"
#include "data/ShaderConstants.h"
#include "drawable.h"
#include "framework.h"
#include "scene.h"
#include "types.h"
#include "video.h"
#include <map>

#include "StringEx.h" // todo : to cpp

//

extern float virtualToScreenX(const float x);
extern float virtualToScreenY(const float y);
extern float screenXToVirtual(const float x);
extern float screenYToVirtual(const float y);

extern const int SCREEN_SX;
extern const int SCREEN_SY;

extern const int GFX_SX;
extern const int GFX_SY;

extern Config config;

extern float g_pcmVolume;
extern GLuint g_pcmTexture;
extern GLuint g_fftTexture;

//

struct Effect;
struct EffectInfo;

//

struct EffectInfo
{
	std::string imageName;
	std::string paramName[4];
};

struct EffectInfosByName : public std::map<std::string, EffectInfo>
{
	bool load(const char * filename);
};

extern EffectInfosByName g_effectInfosByName;

extern std::string effectParamToName(const std::string & effectName, const std::string & param);
extern std::string nameToEffectParam(const std::string & effectName, const std::string & name);

//

extern void registerEffect(const char * name, Effect * effect);
extern void unregisterEffect(Effect * effect);

extern std::map<std::string, Effect*> g_effectsByName;

//

const static float eps = 1e-10f;
const static float pi2 = float(M_PI) * 2.f;

//

struct Effect : TweenFloatCollection
{
	TweenFloat visible;
	bool is3D; // when set to 3D, the effect is rendered using a separate virtual camera to each screen. when false, it will use simple 1:1 mapping onto screen coordinates
	Mat4x4 transform; // transformation matrix for 3D effects
	bool is2D;
	BlendMode blendMode;
	TweenFloat screenX;
	TweenFloat screenY;
	TweenFloat scaleX;
	TweenFloat scaleY;
	TweenFloat scale;
	TweenFloat z;

	bool debugEnabled;

	Effect(const char * name)
		: visible(1.f)
		, is3D(false)
		, is2D(false)
		, blendMode(kBlendMode_Add)
		, screenX(0.f)
		, screenY(0.f)
		, scaleX(1.f)
		, scaleY(1.f)
		, scale(1.f)
		, z(0.f)
		, debugEnabled(true)
	{
		addVar("visible", visible);
		addVar("x", screenX);
		addVar("y", screenY);
		addVar("scale_x", scaleX);
		addVar("scale_y", scaleY);
		addVar("scale", scale);
		addVar("z", z);

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

	void setTextures(Shader & shader)
	{
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		shader.setTexture("pcm", 1, g_pcmTexture, true, false);
		shader.setTexture("fft", 2, g_fftTexture, true, false);
	}

	virtual void tick(const float dt) = 0;
	virtual void draw(DrawableList & list) = 0;
	virtual void draw() = 0;
	virtual void handleSignal(const std::string & name) { }
};

//

struct EffectDrawable : Drawable
{
	Effect * m_effect;

	EffectDrawable(Effect * effect);

	virtual void draw() override;
};

//

struct Effect_Fsfx : Effect
{
	std::string m_shader;
	TweenFloat m_alpha;
	TweenFloat m_param1;
	TweenFloat m_param2;
	TweenFloat m_param3;
	TweenFloat m_param4;
	std::vector<std::string> m_images;
	GLuint m_textureArray;

	Effect_Fsfx(const char * name, const char * shader, const std::vector<std::string> & images);
	virtual ~Effect_Fsfx() override;

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Rain : Effect
{
	ParticleSystem m_particleSystem;
	Array<float> m_particleSizes;
	EffectTimer m_spawnTimer;
	TweenFloat m_alpha;
	TweenFloat m_gravity;
	TweenFloat m_falloff;
	TweenFloat m_spawnRate;
	TweenFloat m_spawnLife;
	TweenFloat m_spawnY;
	TweenFloat m_bounce;

	Effect_Rain(const char * name, const int numRainDrops)
		: Effect(name)
		, m_particleSystem(numRainDrops)
		, m_alpha(1.f)
		, m_gravity(100.f)
		, m_falloff(0.f)
		, m_spawnRate(1.f)
		, m_spawnLife(1.f)
		, m_spawnY(0.f)
		, m_bounce(1.f)
	{
		addVar("alpha", m_alpha);
		addVar("gravity", m_gravity);
		addVar("falloff", m_falloff);
		addVar("spawn_rate", m_spawnRate);
		addVar("spawn_life", m_spawnLife);
		addVar("spawn_y", m_spawnY);
		addVar("bounce", m_bounce);

		m_particleSizes.resize(numRainDrops, true);
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);

		const float gravityY = m_gravity;
		const float falloff = Calc::Max(0.f, 1.f - m_falloff);
		const float falloffThisTick = powf(falloff, dt);

		const Sprite sprite("rain.png");
		const float spriteSx = sprite.getWidth();
		const float spriteSy = sprite.getHeight();

		// spawn particles

		m_spawnTimer.tick(dt);

		while (m_spawnRate != 0.f && m_spawnTimer.consume(1.f / m_spawnRate))
		{
			int id;

			if (!m_particleSystem.alloc(false, m_spawnLife, id))
				continue;

			m_particleSystem.x[id] = rand() % GFX_SX;
			m_particleSystem.y[id] = m_spawnY;
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
				m_particleSystem.vy[i] *= -m_bounce;
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
			m_particleSystem.draw(m_alpha);
		}
		gxSetTexture(0);
	}
};

//

struct Effect_StarCluster : Effect
{
	ParticleSystem m_particleSystem;
	bool m_localSpace;
	TweenFloat m_alpha;
	TweenFloat m_gravityX;
	TweenFloat m_gravityY;

	Effect_StarCluster(const char * name, const int numStars)
		: Effect(name)
		, m_particleSystem(numStars)
		, m_alpha(1.f)
	{
		is2D = true;

		addVar("alpha", m_alpha);
		addVar("gravity_x", m_gravityX);
		addVar("gravity_y", m_gravityY);

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
		TweenFloatCollection::tick(dt);

		// affect stars based on force from center

		for (int i = 0; i < m_particleSystem.numParticles; ++i)
		{
			if (!m_particleSystem.alive[i])
				continue;

			const float dx = m_particleSystem.x[i] - m_gravityX;
			const float dy = m_particleSystem.y[i] - m_gravityY;
			const float ds = sqrtf(dx * dx + dy * dy) + eps;

#if 0
			const float as = 100.f;
			const float ax = -dx / ds * as;
			const float ay = -dy / ds * as;
#else
			const float ax = -dx;
			const float ay = -dy;
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
			m_particleSystem.draw(m_alpha);
		}
		gxSetTexture(0);
	}
};

//

#define CLOTH_MAX_SX 16
#define CLOTH_MAX_SY 16

struct Effect_Cloth : Effect
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
	Vertex vertices[CLOTH_MAX_SX][CLOTH_MAX_SY];

	Effect_Cloth(const char * name)
		: Effect(name)
	{
		// todo : set is2D (?)

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
		TweenFloatCollection::tick(dt);

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

//

struct Effect_SpriteSystem : Effect
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

	Effect_SpriteSystem(const char * name)
		: Effect(name)
	{
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);

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

			new (list)SpriteDrawable(&s);
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

//

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
		TweenFloatCollection::tick(dt);

		for (auto i = m_boxes.begin(); i != m_boxes.end();)
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
					gxNormal3f(0.f, 0.f, -1.f);
					gxVertex3f(-1.f, -1.f, -1.f);
					gxVertex3f(+1.f, -1.f, -1.f);
					gxVertex3f(+1.f, +1.f, -1.f);
					gxVertex3f(-1.f, +1.f, -1.f);

					gxNormal3f(0.f, 0.f, +1.f);
					gxVertex3f(-1.f, -1.f, +1.f);
					gxVertex3f(+1.f, -1.f, +1.f);
					gxVertex3f(+1.f, +1.f, +1.f);
					gxVertex3f(-1.f, +1.f, +1.f);

					gxNormal3f(-1.f, 0.f, 0.f);
					gxVertex3f(-1.f, -1.f, -1.f);
					gxVertex3f(-1.f, +1.f, -1.f);
					gxVertex3f(-1.f, +1.f, +1.f);
					gxVertex3f(-1.f, -1.f, +1.f);

					gxNormal3f(+1.f, 0.f, 0.f);
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

//

struct Effect_Picture : Effect
{
	TweenFloat m_alpha;
	TweenFloat m_angle;
	std::string m_filename;
	bool m_centered;

	Effect_Picture(const char * name, const char * filename, bool centered)
		: Effect(name)
		, m_alpha(1.f)
		, m_angle(0.f)
		, m_centered(true)
	{
		is2D = true;

		addVar("alpha", m_alpha);
		addVar("angle", m_angle);

		m_filename = filename;
		m_centered = centered;
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		gxPushMatrix();
		{
			Sprite sprite(m_filename.c_str());

			const int sx = sprite.getWidth();
			const int sy = sprite.getHeight();
			const float scaleX = SCREEN_SX / float(sx);
			const float scaleY = SCREEN_SY / float(sy);
			const float scale = Calc::Min(scaleX, scaleY);

			gxRotatef(m_angle, 0.f, 0.f, 1.f);
			gxScalef(scale, scale, 1.f);
			if (m_centered)
				gxTranslatef(-sx / 2.f, -sy / 2.f, 0.f);

			setColorf(1.f, 1.f, 1.f, m_alpha);
			sprite.drawEx(0.f, 0.f, 0.f, 1.f, 1.f, false, FILTER_LINEAR);
		}
		gxPopMatrix();
	}
};

//

struct Effect_Video : Effect
{
	TweenFloat m_alpha;
	std::string m_filename;
	std::string m_shader;
	bool m_centered;
	TweenFloat m_speed;

	MediaPlayer m_mediaPlayer;

	Effect_Video(const char * name, const char * filename, const char * shader, const bool centered, const bool play)
		: Effect(name)
		, m_alpha(1.f)
		, m_centered(true)
		, m_speed(1.f)
	{
		is2D = true;

		addVar("alpha", m_alpha);
		addVar("speed", m_speed);

		m_filename = filename;
		m_shader = shader;
		m_centered = centered;

		if (play)
		{
			handleSignal("start");
		}
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);

		if (m_mediaPlayer.isActive(m_mediaPlayer.context))
		{
			m_mediaPlayer.speed = m_speed;
			//m_mediaPlayer.tick(dt);

			if (!m_mediaPlayer.isActive(m_mediaPlayer.context))
			{
				m_mediaPlayer.close();
			}
		}
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		if (m_mediaPlayer.getTexture())
		{
			gxPushMatrix();
			{
				const int sx = m_mediaPlayer.sx;
				const int sy = m_mediaPlayer.sy;
				const float scaleX = SCREEN_SX / float(sx);
				const float scaleY = SCREEN_SY / float(sy);
				const float scale = Calc::Min(scaleX, scaleY);

				gxScalef(scale, scale, 1.f);
				if (m_centered)
					gxTranslatef(-sx / 2.f, -sy / 2.f, 0.f);

				if (!m_shader.empty())
				{
					Shader shader(m_shader.c_str());
					shader.setTexture("colormap", 0, m_mediaPlayer.getTexture(), true, true);

					setShader(shader);
					{
						drawRect(0, m_mediaPlayer.sy, m_mediaPlayer.sx, 0);
					}
					clearShader();
				}
				else
				{
					setColorf(1.f, 1.f, 1.f, m_alpha);
					m_mediaPlayer.draw();
				}
			}
			gxPopMatrix();
		}
	}

	virtual void handleSignal(const std::string & name)
	{
		if (name == "start")
		{
			if (m_mediaPlayer.isActive(m_mediaPlayer.context))
			{
				m_mediaPlayer.close();
			}

			if (!m_mediaPlayer.open(m_filename.c_str()))
			{
				logWarning("failed to open %s", m_filename.c_str());
			}
		}
	}
};

//

struct Effect_Luminance : Effect
{
	TweenFloat m_alpha;
	TweenFloat m_power;
	TweenFloat m_mul;
	TweenFloat m_darken;
	TweenFloat m_darkenAlpha;

	Effect_Luminance(const char * name)
		: Effect(name)
		, m_alpha(1.f)
		, m_power(1.f)
		, m_mul(1.f)
		, m_darken(0.f)
		, m_darkenAlpha(0.f)
	{
		addVar("alpha", m_alpha);
		addVar("power", m_power);
		addVar("mul", m_mul);
		addVar("darken", m_darken);
		addVar("darken_alpha", m_darkenAlpha);
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		setBlend(BLEND_OPAQUE);

		Shader shader("luminance");
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		ShaderBuffer buffer;
		ShaderBuffer buffer2;
		LuminanceData data;
		data.alpha = m_alpha;
		data.power = m_power;
		data.scale = m_mul;
		data.darken = m_darken;
		LuminanceData2 data2;
		data2.darkenAlpha = m_darkenAlpha;
		//logDebug("p=%g, m=%g", (float)m_power, (float)m_mul);
		buffer.setData(&data, sizeof(data));
		shader.setBuffer("LuminanceBlock", buffer);
		buffer2.setData(&data2, sizeof(data2));
		shader.setBuffer("LuminanceBlock2", buffer2);
		g_currentSurface->postprocess(shader);

		setBlend(BLEND_ADD);
	}
};

//

struct Effect_ColorLut2D : Effect
{
	TweenFloat m_alpha;
	Sprite * m_lutSprite;
	TweenFloat m_lutStart;
	TweenFloat m_lutEnd;
	TweenFloat m_numTaps;

	Effect_ColorLut2D(const char * name, const char * lut)
		: Effect(name)
		, m_alpha(1.f)
		, m_lutSprite(nullptr)
		, m_lutStart(0.f)
		, m_lutEnd(1.f)
		, m_numTaps(1.f)
	{
		addVar("alpha", m_alpha);
		addVar("lut_start", m_lutStart);
		addVar("lut_end", m_lutEnd);
		addVar("num_taps", m_numTaps);

		m_lutSprite = new Sprite(lut);
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		setBlend(BLEND_OPAQUE);

		Shader shader("colorlut2d");
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		shader.setTexture("lut", 1, m_lutSprite->getTexture(), true, true);
		ShaderBuffer buffer;
		ColorLut2DData data;
		data.alpha = m_alpha;
		data.lutStart = m_lutStart;
		data.lutEnd = m_lutEnd;
		data.numTaps = m_numTaps;
		buffer.setData(&data, sizeof(data));
		shader.setBuffer("ColorLut2DBlock", buffer);
		g_currentSurface->postprocess(shader);

		setBlend(BLEND_ADD);
	}
};

//

struct Effect_Flowmap : Effect
{
	TweenFloat m_alpha;
	std::string m_map;
	Sprite * m_mapSprite;
	TweenFloat m_strength;
	TweenFloat m_darken;

	Effect_Flowmap(const char * name, const char * map)
		: Effect(name)
		, m_alpha(1.f)
		, m_strength(0.f)
		, m_darken(0.f)
	{
		addVar("alpha", m_alpha);
		addVar("strength", m_strength);
		addVar("darken", m_darken);

		m_map = map;
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		setBlend(BLEND_OPAQUE);

		Sprite mapSprite(m_map.c_str());

		Shader shader("flowmap");
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		shader.setTexture("flowmap", 1, mapSprite.getTexture(), true, false);
		shader.setImmediate("time", g_currentScene->m_time);
		ShaderBuffer buffer;
		FlowmapData data;
		data.alpha = m_alpha;
		data.strength = m_strength;
		data.darken = m_darken;
		buffer.setData(&data, sizeof(data));
		shader.setBuffer("FlowmapBlock", buffer);
		g_currentSurface->postprocess(shader);

		setBlend(BLEND_ADD);
	}
};

//

struct Effect_Vignette : Effect
{
	TweenFloat m_alpha;
	TweenFloat m_innerRadius;
	TweenFloat m_distance;

	Effect_Vignette(const char * name)
		: Effect(name)
		, m_alpha(1.f)
		, m_innerRadius(0.f)
		, m_distance(100.f)
	{
		addVar("alpha", m_alpha);
		addVar("inner_radius", m_innerRadius);
		addVar("distance", m_distance);
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		setBlend(BLEND_OPAQUE);

		Shader shader("vignette");
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		ShaderBuffer buffer;
		VignetteData data;
		data.alpha = m_alpha;
		data.innerRadius = m_innerRadius;
		data.distanceRcp = 1.f / m_distance;
		buffer.setData(&data, sizeof(data));
		shader.setBuffer("VignetteBlock", buffer);
		g_currentSurface->postprocess(shader);

		setBlend(BLEND_ADD);
	}
};

//

struct Effect_Clockwork : Effect
{
	TweenFloat m_alpha;
	TweenFloat m_innerRadius;
	TweenFloat m_distance;

	Effect_Clockwork(const char * name)
		: Effect(name)
		, m_alpha(1.f)
		, m_innerRadius(0.f)
		, m_distance(100.f)
	{
		addVar("alpha", m_alpha);
		addVar("inner_radius", m_innerRadius);
		addVar("distance", m_distance);
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		setBlend(BLEND_OPAQUE);

		Shader shader("vignette");
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		ShaderBuffer buffer;
		VignetteData data;
		data.alpha = m_alpha;
		data.innerRadius = m_innerRadius;
		data.distanceRcp = 1.f / m_distance;
		buffer.setData(&data, sizeof(data));
		shader.setBuffer("VignetteBlock", buffer);
		g_currentSurface->postprocess(shader);

		setBlend(BLEND_ADD);
	}
};

//

struct Effect_DrawPicture : Effect
{
	TweenFloat m_alpha;
	TweenFloat m_step;
	std::string m_map;

	bool m_draw;

	struct Coord
	{
		float x;
		float y;
	};

	float m_distance;

	std::vector<Coord> m_coords;
	Coord m_lastCoord;

	Effect_DrawPicture(const char * name, const char * map)
		: Effect(name)
		, m_alpha(1.f)
		, m_step(10.f)
		, m_draw(false)
		, m_distance(0.f)
	{
		addVar("alpha", m_alpha);
		addVar("step", m_step);

		m_map = map;
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);

		const bool draw = mouse.isDown(BUTTON_LEFT);

		if (draw)
		{
			Coord coord;
			coord.x = mouse.x;
			coord.y = mouse.y;

			if (m_draw)
			{
				const float dx = coord.x - m_lastCoord.x;
				const float dy = coord.y - m_lastCoord.y;
				const float ds = sqrtf(dx * dx + dy * dy);

				if (ds > 0.f)
				{
					float distance = m_distance + ds;

					while (distance >= m_step)
					{
						distance -= m_step;
						m_lastCoord.x += dx / ds * m_step;
						m_lastCoord.y += dy / ds * m_step;
						m_coords.push_back(m_lastCoord);
					}

					const float dx = coord.x - m_lastCoord.x;
					const float dy = coord.y - m_lastCoord.y;
					const float ds = sqrtf(dx * dx + dy * dy);
					m_distance = ds;
				}
			}
			else
			{
				m_lastCoord = coord;
				m_coords.push_back(m_lastCoord);
			}
		}
		else
		{
		}

		m_draw = draw;
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		if (!m_coords.empty())
		{
			Sprite sprite(m_map.c_str());

			const int sx = sprite.getWidth();
			const int sy = sprite.getHeight();

			gxPushMatrix();
			{
				gxTranslatef(-sx / 2.f * scale, -sy / 2.f * scale, 0.f);

				for (auto coord : m_coords)
				{
					sprite.drawEx(coord.x, coord.y, 0.f, scale, scale, false, FILTER_LINEAR);
				}
			}
			gxPopMatrix();

			m_coords.clear();
		}
	}
};

//

struct Effect_Blit : Effect
{
	TweenFloat m_alpha;
	TweenFloat m_centered;
	TweenFloat m_absolute;
	TweenFloat m_srcX;
	TweenFloat m_srcY;
	TweenFloat m_srcSx;
	TweenFloat m_srcSy;
	std::string m_layer;

	Effect_Blit(const char * name, const char * layer);

	void transformCoords(float x, float y, bool addSize, float & out_x, float & out_y, float & out_u, float & out_v);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Blocks : public Effect
{
	struct Block
	{
		Block();

		float x;
		float y;
		float size;
		float speed;
		float value;
	};

	TweenFloat m_alpha;
	TweenFloat m_numBlocks;
	TweenFloat m_minSpeed;
	TweenFloat m_maxSpeed;
	TweenFloat m_minSize;
	TweenFloat m_maxSize;

	std::vector<Block> m_blocks;

	Effect_Blocks(const char * name);

	void spawnBlock();

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Lines : public Effect
{
	struct Line
	{
		Line();

		bool active;
		float x;
		float y;
		float sx;
		float sy;
		float speedX;
		float speedY;
		float c;
	};

	TweenFloat m_alpha;
	TweenFloat m_spawnRate;
	TweenFloat m_minSpeed;
	TweenFloat m_maxSpeed;
	TweenFloat m_minSize;
	TweenFloat m_maxSize;
	TweenFloat m_color1;
	TweenFloat m_color2;
	TweenFloat m_thickness;
	TweenFloat m_spawnTimer;

	std::vector<Line> m_lines;

	Effect_Lines(const char * name, const int numLines);

	void spawnLine();

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Bars : public Effect
{
	static const int kNumLayers = 3;

	struct Bar
	{
		Bar();

		float skipSize;
		float drawSize;
		int color;
	};

	TweenFloat m_alpha;
	TweenFloat m_shuffleRate;
	TweenFloat m_shuffleTimer;
	TweenFloat m_shuffle;
	TweenFloat m_baseSize;
	TweenFloat m_minSize;
	TweenFloat m_maxSize;
	TweenFloat m_sizePow;
	TweenFloat m_topAlpha;
	TweenFloat m_bottomAlpha;

	std::vector<Bar> m_bars[kNumLayers];

	Effect_Bars(const char * name);

	void initializeBars();
	void shuffleBar(int layer);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

