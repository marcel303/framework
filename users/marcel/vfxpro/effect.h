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

//

extern float virtualToScreenX(const float x);
extern float virtualToScreenY(const float y);

extern const int SCREEN_SX;
extern const int SCREEN_SY;

extern const int GFX_SX;
extern const int GFX_SY;

extern Config config;

//

struct Effect;

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
	bool is3D; // when set to 3D, the effect is rendered using a separate virtual camera to each screen. when false, it will use simple 1:1 mapping onto screen coordinates
	Mat4x4 transform; // transformation matrix for 3D effects
	bool is2D;
	TweenFloat screenX;
	TweenFloat screenY;
	TweenFloat scaleX;
	TweenFloat scaleY;
	TweenFloat scale;
	TweenFloat z;

	bool debugEnabled;

	Effect(const char * name)
		: is3D(false)
		, is2D(false)
		, screenX(0.f)
		, screenY(0.f)
		, scaleX(1.f)
		, scaleY(1.f)
		, scale(1.f)
		, z(0.f)
		, debugEnabled(true)
	{
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

	virtual void tick(const float dt) = 0;
	virtual void draw(DrawableList & list) = 0;
	virtual void draw() = 0;
	virtual void handleSignal(const std::string & name) { }
};

//

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
		if (!m_effect->debugEnabled)
			return;

		gxPushMatrix();
		{
			if (m_effect->is3D)
				gxMultMatrixf(m_effect->transform.m_v);
			else if (m_effect->is2D)
			{
				gxTranslatef(virtualToScreenX(m_effect->screenX), virtualToScreenY(m_effect->screenY), 0.f);
				gxScalef(m_effect->scaleX * m_effect->scale, m_effect->scaleY * m_effect->scale, 1.f);
			}

			m_effect->draw();
		}
		gxPopMatrix();
	}
};

//

//

struct Effect_Fsfx : Effect
{
	std::string m_shader;
	TweenFloat m_alpha;
	TweenFloat m_param1;
	TweenFloat m_param2;
	TweenFloat m_param3;
	TweenFloat m_param4;

	Effect_Fsfx(const char * name, const char * shader)
		: Effect(name)
		, m_alpha(1.f)
		, m_param1(0.f)
		, m_param2(0.f)
		, m_param3(0.f)
		, m_param4(0.f)
	{
		addVar("alpha", m_alpha);
		addVar("param1", m_param1);
		addVar("param2", m_param2);
		addVar("param3", m_param3);
		addVar("param4", m_param4);

		m_shader = shader;
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
		if (m_alpha <= 0.f)
			return;

		setBlend(BLEND_OPAQUE);

		Shader shader(m_shader.c_str(), "fsfx.vs", m_shader.c_str());
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		ShaderBuffer buffer;
		FsfxData data;
		data.alpha = m_alpha;
		data.time = g_currentScene->m_time;
		data.param1 = m_param1;
		data.param2 = m_param2;
		data.param3 = m_param3;
		data.param4 = m_param4;
		buffer.setData(&data, sizeof(data));
		shader.setBuffer("FsfxBlock", buffer);
		g_currentSurface->postprocess(shader);

		setBlend(BLEND_ADD);
	}
};

//

struct Effect_Rain : Effect
{
	ParticleSystem m_particleSystem;
	Array<float> m_particleSizes;
	EffectTimer m_spawnTimer;
	TweenFloat m_alpha;

	Effect_Rain(const char * name, const int numRainDrops)
		: Effect(name)
		, m_particleSystem(numRainDrops)
		, m_alpha(1.f)
	{
		addVar("alpha", m_alpha);

		m_particleSizes.resize(numRainDrops, true);
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);

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
	std::string m_filename;
	bool m_centered;

	Effect_Picture(const char * name, const char * filename, bool centered)
		: Effect(name)
		, m_alpha(1.f)
		, m_centered(true)
	{
		is2D = true;

		addVar("alpha", m_alpha);

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
	bool m_centered;

	MediaPlayer m_mediaPlayer;

	Effect_Video(const char * name, const char * filename, const bool centered, const bool play)
		: Effect(name)
		, m_alpha(1.f)
		, m_centered(true)
	{
		is2D = true;

		addVar("alpha", m_alpha);

		m_filename = filename;
		m_centered = centered;

		if (play)
		{
			handleSignal("start");
		}
	}

	virtual void tick(const float dt) override
	{
		TweenFloatCollection::tick(dt);

		if (m_mediaPlayer.isActive())
		{
			m_mediaPlayer.tick(dt);

			if (!m_mediaPlayer.isActive())
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
		if (m_mediaPlayer.texture)
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

				setColorf(1.f, 1.f, 1.f, m_alpha);
				m_mediaPlayer.draw();
			}
			gxPopMatrix();
		}
	}

	virtual void handleSignal(const std::string & name)
	{
		if (name == "start" && !m_mediaPlayer.isActive())
		{
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

	Effect_Luminance(const char * name)
		: Effect(name)
		, m_alpha(1.f)
		, m_power(1.f)
		, m_mul(1.f)
		, m_darken(0.f)
	{
		addVar("alpha", m_alpha);
		addVar("power", m_power);
		addVar("mul", m_mul);
		addVar("darken", m_darken);
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
		LuminanceData data;
		data.alpha = m_alpha;
		data.power = m_power;
		data.scale = m_mul;
		data.darken = m_darken;
		//logDebug("p=%g, m=%g", (float)m_power, (float)m_mul);
		buffer.setData(&data, sizeof(data));
		shader.setBuffer("LuminanceBlock", buffer);
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
		shader.setTexture("lut", 1, m_lutSprite->getTexture(), true, false);
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
		, m_mapSprite(nullptr)
		, m_strength(0.f)
		, m_darken(0.f)
	{
		addVar("alpha", m_alpha);
		addVar("strength", m_strength);
		addVar("darken", m_darken);

		m_map = map;
		m_mapSprite = new Sprite(map);
	}

	virtual ~Effect_Flowmap()
	{
		delete m_mapSprite;
		m_mapSprite = nullptr;
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

		Shader shader("flowmap");
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		shader.setTexture("flowmap", 1, m_mapSprite->getTexture(), true, false); // todo
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
