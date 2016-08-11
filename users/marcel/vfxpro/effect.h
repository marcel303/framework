#pragma once

#include "BezierPath.h"
#include "Calc.h"
#include "config.h"
#include "data/ShaderConstants.h"
#include "drawable.h"
#include "framework.h"
#include "scene.h"
#include "types.h"
#include "video.h"
#include <map>
#include <vector>

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

extern bool g_isReplay;

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
	std::string typeName;
	TweenFloat visible;
	bool is3D; // when set to 3D, the effect is rendered using a separate virtual camera to each screen. when false, it will use simple 1:1 mapping onto screen coordinates
	Mat4x4 transform; // transformation matrix for 3D effects
	bool is2D;
	bool is2DAbsolute;
	BlendMode blendMode;
	TweenFloat screenX;
	TweenFloat screenY;
	TweenFloat scaleX;
	TweenFloat scaleY;
	TweenFloat scale;
	TweenFloat angle;
	TweenFloat z;
	TweenFloat timeMultiplier;

	bool debugEnabled;

	Effect(const char * name);
	virtual ~Effect();

	Vec2 screenToLocal(Vec2Arg v) const;
	Vec2 localToScreen(Vec2Arg v) const;
	Vec3 worldToLocal(Vec3Arg v, const bool withTranslation) const;
	Vec3 localToWorld(Vec3Arg v, const bool withTranslation) const;

	void setTextures(Shader & shader);
	void applyBlendMode() const;

	void tickBase(const float dt);

	virtual void tick(const float dt) = 0;
	virtual void draw(DrawableList & list) = 0;
	virtual void draw() = 0;
	virtual void handleSignal(const std::string & name) { }
	virtual void syncTime(const float time) { }

	// TweenFloatCollection

	virtual TweenFloat * getVar(const char * name) override;
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
	float m_time;

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
	TweenFloat m_speedScaleX;
	TweenFloat m_speedScaleY;
	TweenFloat m_size1;
	TweenFloat m_size2;

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
		, m_size1(1.f)
		, m_size2(1.f)
		, m_speedScaleX(0.f)
		, m_speedScaleY(0.f)
	{
		is2DAbsolute = true;

		addVar("alpha", m_alpha);
		addVar("gravity", m_gravity);
		addVar("falloff", m_falloff);
		addVar("spawn_rate", m_spawnRate);
		addVar("spawn_life", m_spawnLife);
		addVar("spawn_y", m_spawnY);
		addVar("bounce", m_bounce);
		addVar("size1", m_size1);
		addVar("size2", m_size2);
		addVar("speed_scale_x", m_speedScaleX);
		addVar("speed_scale_y", m_speedScaleY);

		m_particleSizes.resize(numRainDrops, true);
	}

	virtual void tick(const float dt) override
	{
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

			const float life = m_particleSystem.life[i] * m_particleSystem.lifeRcp[i];
			
			float size = m_particleSizes[i];

			size *= lerp((float)m_size2, (float)m_size1, life);

			m_particleSystem.sx[i] = size * spriteSx;
			m_particleSystem.sy[i] = size * spriteSy;

			if (m_speedScaleX != 0.f)
				m_particleSystem.sx[i] *= m_particleSystem.vx[i] * m_speedScaleX;
			if (m_speedScaleY != 0.f)
				m_particleSystem.sy[i] *= m_particleSystem.vy[i] * m_speedScaleY;

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
		if (m_alpha <= 0.f)
			return;

		gxSetTexture(Sprite("rain.png").getTexture());
		{
			setColor(colorWhite);

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

	Effect_StarCluster(const char * name, const int numStars);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
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

	Effect_Cloth(const char * name);

	void setup(int _sx, int _sy);

	Vertex * getVertex(int x, int y);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;

	void doDraw();
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

//

struct Effect_Boxes : Effect
{
	struct Box : TweenFloatCollection
	{
		std::string m_name;

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

		Box();

		bool tick(const float dt);
	};

	std::list<Box*> m_boxes;

	Effect_Boxes(const char * name);
	virtual ~Effect_Boxes() override;

	Box * addBox(
		const char * name,
		const float tx, const float ty, const float tz,
		const float sx, const float sy, const float sz,
		const int axis);
	Box * findBoxByName(const char * name);

	//

	virtual TweenFloat * getVar(const char * name) override;

	//

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
	virtual void handleSignal(const std::string & message) override;
};

//

struct Effect_Picture : Effect
{
	TweenFloat m_alpha;
	std::string m_filename;
	bool m_centered;

	Effect_Picture(const char * name, const char * filename, bool centered);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Video : Effect
{
	TweenFloat m_alpha;
	std::string m_filename;
	std::string m_shader;
	bool m_centered;
	TweenFloat m_speed;
	TweenFloat m_hideWhenDone;

	MediaPlayer m_mediaPlayer;
	float m_startTime;

	Effect_Video(const char * name, const char * filename, const char * shader, const bool centered, const bool play);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;

	virtual void handleSignal(const std::string & name) override;
	virtual void syncTime(const float time) override;
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
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		setBlend(BLEND_OPAQUE);
		setColor(colorWhite);

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
	}

	virtual void draw(DrawableList & list) override
	{
		new (list) EffectDrawable(this);
	}

	virtual void draw() override
	{
		setBlend(BLEND_OPAQUE);
		setColor(colorWhite);

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
		setColor(colorWhite);

		Sprite mapSprite(m_map.c_str());

		Shader shader("flowmap");
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		shader.setTexture("flowmap", 1, mapSprite.getTexture(), true, false);
		shader.setImmediate("flow_time", g_currentScene->m_time);
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
		setColor(colorWhite);

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
		setColor(colorWhite);

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
				gxColor4f(1.f, 1.f, 1.f, 1.f);

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

struct Effect_Blocks : Effect
{
	struct Block
	{
		Block();

		float x;
		float y;
		float size;
		float speed;
		float value;
		GLuint picture;
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

struct Effect_Lines : Effect
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

struct Effect_Bars : Effect
{
	static const int kNumLayers = 1;

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

	std::vector<Color> m_colorBarColors;

	Effect_Bars(const char * name);

	void initializeBars();
	void shuffleBar(int layer);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;

	virtual void handleSignal(const std::string & message) override;
};

//

struct Effect_Text : Effect
{
	TweenFloat m_alpha;
	Color m_color;
	std::string m_font;
	int m_fontSize;
	std::string m_text;
	TweenFloat m_textAlignX;
	TweenFloat m_textAlignY;

	Effect_Text(const char * name, const Color & color, const char * font, const int fontSize, const char * text);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Bezier : Effect
{
	struct Node
	{
		Node()
			: moveDelay(0.f)
			, time(0.f)
		{
		}

		BezierNode bezierNode;
		Vec2F tangentSpeed;
		Vec2F positionSpeed;
		float moveDelay;
		float time;
	};

	struct Segment
	{
		Color color;
		float time;
		float timeRcp;
		float growTime;
		float growTimeRcp;
		int numGhosts;
		float ghostSize;

		std::vector<Node> nodes;
	};

	std::vector<Segment> segments;

	ColorCurve colorCurve;

	Effect_Bezier(const char * name, const char * colors);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;

	virtual void handleSignal(const std::string & name) override;

	void generateThrow();
	void generateSegment(const Vec2F & p1, const Vec2F & p2, const float posSpeed, const float tanSpeed, const float posVary, const float tanVary, const int numNodes, const int numGhosts, const float ghostSize, const Color & color, const float duration, const float growTime, const bool fixedEnds);
};

//

struct Effect_Smoke : Effect
{
	Surface * m_surface;
	bool m_capture;
	std::string m_layer;

	TweenFloat m_alpha;
	TweenFloat m_strength;
	TweenFloat m_darken;
	TweenFloat m_darkenAlpha;
	TweenFloat m_multiply;

	Effect_Smoke(const char * name, const char * layer);
	virtual ~Effect_Smoke();

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;

	virtual void handleSignal(const std::string & name) override;

	void captureSurface();
};
