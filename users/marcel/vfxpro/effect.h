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

extern int GFX_SX;
extern int GFX_SY;

extern Config config;

extern float g_pcmVolume;
extern GLuint g_pcmTexture;
extern GLuint g_fftTexture;
extern GLuint g_fftTextureWithFade;

extern bool g_isReplay;

//

struct Effect;
struct EffectInfo;
struct VideoLoop;

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
	std::string m_image;
	std::vector<std::string> m_images;
	std::vector<Color> m_colors;
	GLuint m_textureArray;
	float m_time;

	Effect_Fsfx(const char * name, const char * shader, const char * image, const std::vector<std::string> & images, const std::vector<Color> & colors);
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
	TweenFloat m_sizeX;
	TweenFloat m_sizeY;
	TweenFloat m_speedScaleX;
	TweenFloat m_speedScaleY;
	TweenFloat m_size1;
	TweenFloat m_size2;

	Effect_Rain(const char * name, const int numRainDrops);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
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
		SpriteInfo();

		bool alive;
		std::string filename;
		float z;
		SpriterState spriterState;
	};

	struct SpriteDrawable : Drawable
	{
		SpriteInfo * m_spriteInfo;

		SpriteDrawable(SpriteInfo * spriteInfo);

		virtual void draw() override;
	};

	SpriteInfo m_sprites[kMaxSprites];

	Effect_SpriteSystem(const char * name);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;

	void addSprite(const char * filename, const int animIndex, const float x, const float y, const float z, const float scale);
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
		virtual ~Box();

		bool tick(const float dt);
	};

	std::string m_shader;
	std::list<Box*> m_boxes;
	TweenFloat m_outline;

	Effect_Boxes(const char * name, const bool screenSpace, const char * shader);
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
	std::string m_filename2;
	std::string m_shader;
	bool m_centered;

	Effect_Picture(const char * name, const char * filename, const char * filename2, const char * shader, bool centered);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

#if ENABLE_VIDEO

struct Effect_Video : Effect
{
	TweenFloat m_alpha;
	std::string m_filename;
	std::string m_shader;
	bool m_yuv;
	bool m_centered;
	TweenFloat m_hideWhenDone;

	MediaPlayer m_mediaPlayer;
	bool m_playing;
	double m_time;
	double m_startTime;

	Effect_Video(const char * name, const char * filename, const char * shader, const bool yuv, const bool centered, const bool play);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;

	virtual void handleSignal(const std::string & name) override;
	virtual void syncTime(const float time) override;
};

struct Effect_VideoLoop : Effect
{
	TweenFloat m_alpha;
	std::string m_filename;
	std::string m_shader;
	bool m_yuv;
	bool m_centered;

	VideoLoop * m_videoLoop;
	bool m_playing;

	Effect_VideoLoop(const char * name, const char * filename, const char * shader, const bool yuv, const bool centered, const bool play);
	virtual ~Effect_VideoLoop();

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;

	virtual void handleSignal(const std::string & name) override;
};

#endif

//

struct Effect_Luminance : Effect
{
	TweenFloat m_alpha;
	TweenFloat m_power;
	TweenFloat m_mul;
	TweenFloat m_darken;
	TweenFloat m_darkenAlpha;

	Effect_Luminance(const char * name);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_ColorLut2D : Effect
{
	std::string m_lut;
	TweenFloat m_alpha;
	TweenFloat m_lutStart;
	TweenFloat m_lutEnd;
	TweenFloat m_numTaps;

	Effect_ColorLut2D(const char * name, const char * lut);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Flowmap : Effect
{
	TweenFloat m_alpha;
	std::string m_map;
	Sprite * m_mapSprite;
	TweenFloat m_strength;
	TweenFloat m_darken;

	Effect_Flowmap(const char * name, const char * map);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Vignette : Effect
{
	TweenFloat m_alpha;
	TweenFloat m_innerRadius;
	TweenFloat m_distance;

	Effect_Vignette(const char * name);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Clockwork : Effect
{
	TweenFloat m_alpha;
	TweenFloat m_innerRadius;
	TweenFloat m_distance;

	Effect_Clockwork(const char * name);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
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

	Effect_DrawPicture(const char * name, const char * map);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
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

	TweenFloat m_alpha;

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

//

struct Effect_Beams : Effect
{
	struct Beam
	{
		Beam()
		{
			memset(this, 0, sizeof(*this));

			length1Speed = 1.f;
			length2Speed = 1.f;
		}

		float angle;
		float length;
		float length2;
		float length1Speed;
		float length2Speed;
		float thickness;
		float offsetX;
		float offsetY;
	};

	TweenFloat m_alpha;
	TweenFloat m_beamTime;
	float m_beamTimer;
	float m_beamTimerRcp;
	TweenFloat m_beamSpeed;
	TweenFloat m_beamSize1;
	TweenFloat m_beamSize2;
	TweenFloat m_beamOffset;

	std::vector<Beam> m_beams;

	Effect_Beams(const char * name);
	virtual ~Effect_Beams();

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;

	virtual void handleSignal(const std::string & name) override;
};

//

struct Effect_FXAA : Effect
{
	Effect_FXAA(const char * name);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Fireworks : Effect
{
	const static int PS = 20000;

	enum PT
	{
		kPT_Root,
		kPT_Child1,
		kPT_Child2,
	};

	struct P
	{
		float x;
		float y;
		float oldX;
		float oldY;
		float vx;
		float vy;
		float life;
		float lifeRcp;
		int type;
		Color color;
	};

	P ps[PS];

	float spawnValue;
	TweenFloat spawnRate;

	int nextParticleIndex;

	TweenFloat m_rootSpeed;
	TweenFloat m_rootSpeedVar;
	TweenFloat m_rootLife;
	TweenFloat m_child1Speed;
	TweenFloat m_child1SpeedVar;
	TweenFloat m_child1Life;
	TweenFloat m_child2Speed;
	TweenFloat m_child2SpeedVar;
	TweenFloat m_child2Life;

	Effect_Fireworks(const char * name);

	P * nextParticle();

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Sparklies : Effect
{
	ParticleSystem m_particleSystem;
	TweenFloat m_alpha;

	Effect_Sparklies(const char * name);

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
};

//

struct Effect_Wobbly : Effect
{
	struct WaterSim
	{
		static const int kNumElems = 768;
		
		double p[kNumElems];
		double v[kNumElems];
		
		WaterSim();
		
		void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds);
	};
	
	struct WaterDrop
	{
		bool isAlive;
		bool isApplying;
		
		double x;
		double y;
		double vx;
		double direction;
		double strength;
		double applyRadius;
		double applyTime;
		double applyTimeRcp;
		double fadeInTime;
		double fadeInTimeRcp;
		
		WaterDrop();
		
		void tick(const double dt, const double stretch, WaterSim & sim);
		
		double toWaterP(const WaterSim & sim, const double x, const double stretch) const;
		double checkIntersection(const WaterSim & sim, const double stretch, const double radius, const double bias) const;
	};
	
	struct Fade
	{
		double falloff;
		double falloffD;
		
		Fade()
			: falloff(0.0)
			, falloffD(1.0)
		{
		}
		
		void tick(const double dt)
		{
			const double f = std::pow(1.0 - falloffD, dt);
			
			falloff *= f;
		}
	};
	
	TweenFloat m_drop;
	TweenFloat m_showDrops;
	TweenFloat m_wobbliness;
	TweenFloat m_closedEnds;
	TweenFloat m_stretch;
	TweenFloat m_numIterations;
	TweenFloat m_alpha;
	
	std::string m_shader;
	
	WaterSim * m_waterSim;
	
	std::vector<WaterDrop> m_waterDrops;
	
	Fade fade;
	
	GLuint elementsTexture;

	Effect_Wobbly(const char * name, const char * shader);
	virtual ~Effect_Wobbly();

	virtual void tick(const float dt) override;
	virtual void draw(DrawableList & list) override;
	virtual void draw() override;
	
	virtual void handleSignal(const std::string & name) override;
};
