#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "Calc.h"
#include "framework.h"
#include <algorithm>
#include <list>
#include <Windows.h>

const static int kNumScreens = 3;

#define SCREEN_SX 1024
#define SCREEN_SY 768

#define GFX_SX (SCREEN_SX * kNumScreens)
#define GFX_SY (SCREEN_SY * 1)

#define OSC_ADDRESS "127.0.0.1"
#define OSC_RECV_PORT 1121

const float eps = 1e-10f;
const float pi2 = M_PI * 2.f;

/*

:: todo :: OSC

-

:: todo :: configuration

- 

:: todo :: projector output

- 

:: todo :: post processing and graphics quality

- smooth line drawing with high AA. use a post process pass to blur the result ?

:: todo :: visuals tech 2D

-

:: todo :: visuals tech 3D

- virtual camera positioning

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

template <typename T>
bool isValidIndex(const T & value) { return value != ((T)-1); }

template <typename T>
struct Array
{
	T * data;

	Array()
		: data(nullptr)
	{
	}

	Array(int numElements)
		: data(nullptr)
	{
		resize(numElements);
	}

	~Array()
	{
		resize(0, false);
	}

	void resize(int numElements, bool zeroMemory)
	{
		if (data != nullptr)
		{
			delete [] data;
			data = nullptr;
		}

		if (numElements != 0)
		{
			data = new T[numElements];

			if (zeroMemory)
			{
				memset(data, 0, sizeof(T) * numElements);
			}
		}
	}

	T & operator[](int index)
	{
		assert(index >= 0);

		return data[index];
	}
};

struct Drawable
{
	float m_z;

	Drawable(float z)
		: m_z(z)
	{
	}

	virtual ~Drawable()
	{
		// nop
	}

	virtual void draw() = 0;

	bool operator<(const Drawable * other) const
	{
		return m_z > other->m_z;
	}
};

struct DrawableList
{
	static const int kMaxDrawables = 1024;

	int numDrawables;
	Drawable * drawables[kMaxDrawables];

	DrawableList()
		: numDrawables(0)
	{
	}

	~DrawableList()
	{
		// todo : use a transient allocator instead of malloc/free

		for (int i = 0; i < numDrawables; ++i)
		{
			delete drawables[i];
			drawables[i] = nullptr;
		}

		numDrawables = 0;
	}

	void add(Drawable * drawable)
	{
		assert(numDrawables < kMaxDrawables);

		if (numDrawables < kMaxDrawables)
		{
			drawables[numDrawables++] = drawable;
		}
	}

	void sort()
	{
		std::sort(drawables, drawables + numDrawables);
	}

	void draw()
	{
		for (int i = 0; i < numDrawables; ++i)
		{
			drawables[i]->draw();
		}
	}
};

void * operator new(size_t size, DrawableList & list)
{
	Drawable * drawable = (Drawable*)malloc(size);

	list.add(drawable);

	return drawable;
}

void operator delete(void * p, DrawableList & list)
{
	free(p);
}

struct Effect
{
	bool is3D; // when set to 3D, the effect is rendered using a separate virtual camera to each screen. when false, it will use simple 1:1 mapping onto screen coordinates
	Mat4x4 transform; // transformation matrix for 3D effects
	float screenX;
	float screenY;
	float scaleX;
	float scaleY;
	float z;

	Effect()
		: is3D(false)
		, screenX(0.f)
		, screenY(0.f)
		, scaleX(1.f)
		, scaleY(1.f)
		, z(0.f)
	{
		transform.MakeIdentity();
	}

	virtual ~Effect()
	{
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

	ParticleSystem(int numElements)
		: numParticles(0)
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

	void free(int & id)
	{
		if (isValidIndex(id))
		{
			alive[id] = false;

			freeList[numFree++] = id;

			id = -1;
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
						int id = i;

						free(id);

						break;
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

					gxTexCoord2f(0.f, 1.f); gxVertex2f(x[i] + (- c * sx_2 - s * sy_2), y[i] + (+ s * sx_2 - c * sy_2));
					gxTexCoord2f(1.f, 1.f); gxVertex2f(x[i] + (+ c * sx_2 - s * sy_2), y[i] + (- s * sx_2 - c * sy_2));
					gxTexCoord2f(1.f, 0.f); gxVertex2f(x[i] + (+ c * sx_2 + s * sy_2), y[i] + (- s * sx_2 + c * sy_2));
					gxTexCoord2f(0.f, 0.f); gxVertex2f(x[i] + (- c * sx_2 + s * sy_2), y[i] + (+ s * sx_2 + c * sy_2));
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

	Effect_Rain(int numRainDrops)
		: m_particleSystem(numRainDrops)
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

		for (int i = 0; i < 2; ++i)
		{
			int id;

			if (!m_particleSystem.alloc(false, 5.f, id))
				continue;

			m_particleSystem.x[id] = rand() % GFX_SX;
			m_particleSystem.y[id] = 0.f;
			m_particleSystem.vx[id] = 0.f;
			m_particleSystem.vy[id] = 50.f;
			//m_particleSystem.sx[id] = 5.f;
			//m_particleSystem.sy[id] = 15.f;
			m_particleSystem.sx[id] = sprite.getWidth() / 4.f;
			m_particleSystem.sy[id] = sprite.getHeight() / 4.f;
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
			m_particleSystem.sy[i] = size * spriteSy;

			// check if the particle is dead

			if (m_particleSystem.life[i] == 0.f || m_particleSystem.y[i] < 0.f)
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

	Effect_StarCluster(int numStars)
		: m_particleSystem(numStars)
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

	ClothPiece()
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
		const float gravityY = keyboard.isDown(SDLK_g) ? 2.f : 0.f;

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

					const float a = (ds - 1.f) * springConstant;

					const float ax = gravityX + dx / (ds + eps) * a;
					const float ay = gravityY + dy / (ds + eps) * a;

					v.vx += ax * dt;
					v.vy += ay * dt;
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
			gxScalef(20.f, 20.f, 1.f);

			for (int i = 3; i >= 1; --i)
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

static float virtualToScreenX(float x)
{
	return ((x / 100.f) + 1.5f) * SCREEN_SX;
}

static float virtualToScreenY(float y)
{
	return (y / 100.f) * SCREEN_SY;
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

	void setup(Vec3Arg position, Vec3 * screenCorners, int numScreenCorners)
	{
		Vec3 screenCenter(0.f, 0.f, 0.f);

		for (int i = 0; i < numScreenCorners; ++i)
			screenCenter += screenCorners[i];
		screenCenter /= numScreenCorners;

		const Vec3 upVector(0.f, 1.f, 0.f);

		Mat4x4 lookatMatrix;
		lookatMatrix.MakeLookat(position, screenCenter, upVector);

		Vec3 * screenCornersInCameraSpace = (Vec3*)alloca(numScreenCorners * sizeof(Vec3));
		Mat4x4 invLookatMatrix = lookatMatrix.CalcInv();

		for (int i = 0; i < numScreenCorners; ++i)
			screenCornersInCameraSpace[i] = lookatMatrix * screenCorners[i];

		Vec3 minCorner = screenCornersInCameraSpace[0];
		Vec3 maxCorner = screenCornersInCameraSpace[0];

		for (int i = 0; i < numScreenCorners; ++i)
		{
			for (int a = 0; a < 3; ++a)
			{
				if (screenCornersInCameraSpace[i][a] < minCorner[a])
					minCorner[a] = screenCornersInCameraSpace[i][a];
				if (screenCornersInCameraSpace[i][a] > maxCorner[a])
					maxCorner[a] = screenCornersInCameraSpace[i][a];
			}
		}

		const float sx = maxCorner[0] - minCorner[1];
		const float sy = maxCorner[1] - minCorner[1];

		// todo : calculate horizontal and vertical fov. setup projection matrix

		const Vec3 delta = screenCenter - position;
		const float distanceToScreen = delta.CalcSize();

		const float fovX = 2.0f * atan2f(sy / 2.f, distanceToScreen);
		const float fovY = 2.0f * atan2f(sx / 2.f, distanceToScreen);

		Mat4x4 projection;
		projection.MakePerspectiveLH(fovX, sy / sx, .001f, 10.f);

		//

		cameraToWorld = invLookatMatrix;
		worldToCamera = lookatMatrix;
		cameraToView = projection;
	}
};

static void drawTestObjects()
{
	for (int k = 0; k < 3; ++k)
	{
		gxPushMatrix();
		{
			gxTranslatef((k - 1) / 1.1f, .5f + (k - 1) / 4.f, +1.f);
			gxScalef(.5f, .5f, .5f);

			gxBegin(GL_LINES);
			{
				gxColor4f(1.f, 0.f, 0.f, 1.f); gxVertex3f(-1.f,  0.f,  0.f); gxVertex3f(+1.f,  0.f,  0.f);
				gxColor4f(0.f, 1.f, 0.f, 1.f); gxVertex3f( 0.f, -1.f,  0.f); gxVertex3f( 0.f, +1.f,  0.f);
				gxColor4f(0.f, 0.f, 1.f, 1.f); gxVertex3f( 0.f,  0.f, -1.f); gxVertex3f( 0.f,  0.f, +1.f);
			}
			gxEnd();
		}
		gxPopMatrix();
	}
}

static void drawGroundPlane(const float y)
{
	gxColor4f(.1f, .1f, .1f, 1.f);
	gxBegin(GL_QUADS);
	{
		gxVertex3f(-100.f, y, -100.f);
		gxVertex3f(+100.f, y, -100.f);
		gxVertex3f(+100.f, y, +100.f);
		gxVertex3f(-100.f, y, +100.f);
	}
	gxEnd();
}

static void drawCamera(const Camera & camera)
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
				gxColor4f(1.f, 0.f, 0.f, 1.f); gxVertex3f(0.f, 0.f, 0.f); gxVertex3f(1.f, 0.f, 0.f);
				gxColor4f(0.f, 1.f, 0.f, 1.f); gxVertex3f(0.f, 0.f, 0.f); gxVertex3f(0.f, 1.f, 0.f);
				gxColor4f(0.f, 0.f, 1.f, 1.f); gxVertex3f(0.f, 0.f, 0.f); gxVertex3f(0.f, 0.f, 1.f);
			}
			gxEnd();
		}
		gxPopMatrix();

		const Mat4x4 invView = camera.cameraToView.CalcInv();
		
		const const Vec3 p[5] =
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
				for (int i = 0; i < 5; ++i)
				{
					gxColor4f(1.f, 1.f, 1.f, 1.f);
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
		gxColor4f(1.f, 1.f, 1.f, 1.f);
		glDisable(GL_DEPTH_TEST);

		setBlend(BLEND_ADD);
	}
	else
	{
		setBlend(BLEND_OPAQUE);
	}

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

	glEnable(GL_DEPTH_TEST);
	setBlend(BLEND_ALPHA);

	gxColor4f(1.f, 1.f, 1.f, 1.f);
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

	// initialise OSC

	InitializeCriticalSectionAndSpinCount(&s_oscMessageMtx, 256);

	s_oscMessageThread = CreateThread(NULL, 64 * 1024, ExecuteOscThread, NULL, CREATE_SUSPENDED, NULL);
	ResumeThread(s_oscMessageThread);

	//recvSocket.Run();

	framework.fullscreen = false;
	framework.minification = 2;
	framework.enableDepthBuffer = true;

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		std::list<TimeDilationEffect> timeDilationEffects;

		bool clearScreen = true;
		bool debugDraw = true;
		bool cameraControl = true;
		bool postProcess = false;

		bool drawRain = true;
		bool drawStarCluster = true;
		bool drawCloth = true;
		bool drawSprites = true;
		bool drawVideo = true;

		bool drawProjectorSetup = false;

		Effect_Rain rain(10000);

		Effect_StarCluster starCluster(100);
		starCluster.screenX = virtualToScreenX(0);
		starCluster.screenY = virtualToScreenY(50);

		ClothPiece clothPiece;
		clothPiece.setup(CLOTHPIECE_MAX_SX, CLOTHPIECE_MAX_SY);

		SpriteSystem spriteSystem;

		Surface surface(GFX_SX, GFX_SY);

		Shader jitterShader("jitter");

		Vec3 cameraPosition(0.f, .75f, -1.5f);
		Vec3 cameraRotation(0.f, 0.f, 0.f);
		Mat4x4 cameraMatrix;
		cameraMatrix.MakeIdentity();

		int activeCamera = 0;

		bool stop = false;

		while (!stop)
		{
			// process

			framework.process();

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
			if (keyboard.wentDown(SDLK_p))
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
				drawVideo = !drawVideo;

			if (keyboard.wentDown(SDLK_s))
				drawProjectorSetup = !drawProjectorSetup;

			const float dtReal = Calc::Min(1.f / 30.f, framework.timeStep);

			Mat4x4 cameraPositionMatrix;
			Mat4x4 cameraRotationMatrix;

			if (cameraControl)
			{
				if (keyboard.isDown(SDLK_RSHIFT))
				{
					cameraRotation[0] -= mouse.dy / 100.f;
					cameraRotation[1] -= mouse.dx / 100.f;
				}

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

				if (keyboard.wentDown(SDLK_HOME))
					cameraRotation.SetZero();
				if (keyboard.wentDown(SDLK_END))
					activeCamera = (activeCamera + 1) % kNumScreens;
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

			const float dt = dtReal * timeDilationMultiplier;

			// process effects

			rain.tick(dt);

			starCluster.tick(dt);

			for (int i = 0; i < 10; ++i)
				clothPiece.tick(dt / 10.f);

			spriteSystem.tick(dt);

		#if 0
			if ((rand() % 30) == 0)
			{
				clothPiece.vertices[rand() % clothPiece.sx][rand() % clothPiece.sy].vx += 20.f;
			}
		#endif

			if (mouse.isDown(BUTTON_LEFT))
			{
				const float mouseX = mouse.x / 20.f;
				const float mouseY = mouse.y / 20.f;

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

			if (drawRain)
				rain.draw(drawableList);

			if (drawStarCluster)
				starCluster.draw(drawableList);

			if (drawCloth)
				clothPiece.draw(drawableList);

			if (drawSprites)
				spriteSystem.draw(drawableList);

			drawableList.sort();

			framework.beginDraw(0, 0, 0, 0, true);
			{
				// camera setup

				Camera cameras[kNumScreens];

				const float depth = -1.f; // todo : rotate the screen instead of hacking their positions

				Vec3 _screenCorners[8] =
				{
					Vec3(-1.5f, 0.f, depth),
					Vec3(-0.5f, 0.f, 0.f),
					Vec3(+0.5f, 0.f, 0.f),
					Vec3(+1.5f, 0.f, depth),

					Vec3(-1.5f, 1.f, depth),
					Vec3(-0.5f, 1.f, 0.f),
					Vec3(+0.5f, 1.f, 0.f),
					Vec3(+1.5f, 1.f, depth),
				};

				Vec3 screenCorners[kNumScreens][4];

				const Vec3 cameraPosition(0.f, 0.5f, -1.f);

				for (int c = 0; c < kNumScreens; ++c)
				{
					Camera & camera = cameras[c];

					screenCorners[c][0] = _screenCorners[c + 0 + 0],
					screenCorners[c][1] = _screenCorners[c + 1 + 0],
					screenCorners[c][2] = _screenCorners[c + 1 + 4],
					screenCorners[c][3] = _screenCorners[c + 0 + 4],

					camera.setup(cameraPosition, screenCorners[c], 4);
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

					setColor(4, 2, 1, 255);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}

			#if 1
				const bool testCameraProjection = true;

				for (int c = 0; c < kNumScreens; ++c)
				{
					const Camera & camera = cameras[c];

					gxMatrixMode(GL_PROJECTION);
					gxPushMatrix();
					{
						if (testCameraProjection)
							gxLoadMatrixf(camera.cameraToView.m_v);

						gxMatrixMode(GL_MODELVIEW);
						gxPushMatrix();
						{
							if (testCameraProjection)
								gxLoadMatrixf(camera.worldToCamera.m_v);

							const int x1 = virtualToScreenX(-150 + (c + 0) * 100);
							const int y1 = virtualToScreenY(0);
							const int x2 = virtualToScreenX(-150 + (c + 1) * 100);
							const int y2 = virtualToScreenY(100);

							const int sx = x2 - x1;
							const int sy = y2 - y1;

						#if 1
							glViewport(
								x1 / framework.minification,
								y1 / framework.minification,
								sx / framework.minification,
								sy / framework.minification);
						#else
							glViewport(
								x1,
								y1,
								sx,
								sy);
						#endif

						#if 0
							if (debugDraw && !testCameraProjection)
							{
								applyTransformWithViewportSize(sx, sy);

								setColorf(1.f, 0.f, 0.f, .5f);
								drawRect(50, 50, sx - 50, sy - 50);
							}
						#endif

						#if 1
							if (debugDraw && testCameraProjection)
							{
								drawTestObjects();
							}
						#endif
						}
						gxMatrixMode(GL_MODELVIEW);
						gxPopMatrix();
					}
					gxMatrixMode(GL_PROJECTION);
					gxPopMatrix();

					gxMatrixMode(GL_MODELVIEW);
				}

				// dirty hack to restore viewport

				pushSurface(nullptr);
				popSurface();
			#endif

				setBlend(BLEND_ADD);
				drawableList.draw();

				// test effect

				if (postProcess)
				{
					setBlend(BLEND_OPAQUE);
					jitterShader.setTexture("colormap", 0, surface.getTexture(), true, false);
					jitterShader.setTexture("jittermap", 1, 0, true, true);
					jitterShader.setImmediate("jitterStrength", 1.f);
					jitterShader.setImmediate("time", framework.time);
					surface.postprocess(jitterShader);
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

				if (debugDraw && drawProjectorSetup)
				{
					glClearColor(0.05f, 0.05, 0.05, 0.f);
					glClear(GL_COLOR_BUFFER_BIT);

					setBlend(BLEND_ALPHA);

				#if 0 // todo : move to camera viewport rendering ?
					// draw projector bounds

					setColorf(1.f, 1.f, 1.f, .25f);
					for (int i = 0; i < kNumScreens; ++i)
						drawRectLine(virtualToScreenX(-150 + i * 100), virtualToScreenY(0.f), virtualToScreenX(-150 + (i + 1) * 100), virtualToScreenY(100));
				#endif

					// draw 3D projector setup

					glEnable(GL_DEPTH_TEST);

					glDepthFunc(GL_LESS);
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
								// draw ground

								drawGroundPlane(0.f);

								// draw the projector screens

								for (int c = 0; c < kNumScreens; ++c)
								{
									drawScreen(screenCorners[c], surfaceTexture, c);
								}

								// draw test objects

								drawTestObjects();

								// draw the cameras

								//for (int c = 0; c < kNumScreens; ++c)
								{
									int c = activeCamera;

									const Camera & camera = cameras[c];

									drawCamera(camera);
								}
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
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	s_oscReceiveSocket->AsynchronousBreak();
	WaitForSingleObject(s_oscMessageThread, INFINITE);
	CloseHandle(s_oscMessageThread);

	delete s_oscReceiveSocket;
	s_oscReceiveSocket = nullptr;

	return 0;
}
