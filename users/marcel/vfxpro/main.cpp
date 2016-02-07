#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "Calc.h"
#include "framework.h"

#define SCREEN_SX 1024
#define SCREEN_SY 768

#define GFX_SX (SCREEN_SX * 3)
#define GFX_SY (SCREEN_SY * 1)

#define OSC_ADDRESS "127.0.0.1"
#define OSC_RECV_PORT 1121
#define OSC_SEND_PORT 1121
#define OSC_OUTPUT_BUFFER_SIZE 1024

/*

:: todo :: OSC

-

:: todo :: configuration

- 

:: todo :: projector output

- 

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

#define MAX_PARTICLES 10000

template <typename T>
bool isValidIndex(const T & value) { return value != ((T)-1); }

struct ParticleSystem
{
	uint16_t freeList[MAX_PARTICLES];
	uint16_t numFree;

	bool alive[MAX_PARTICLES];
	bool autoKill[MAX_PARTICLES];

	float x[MAX_PARTICLES];
	float y[MAX_PARTICLES];
	float vx[MAX_PARTICLES];
	float vy[MAX_PARTICLES];
	float sx[MAX_PARTICLES];
	float sy[MAX_PARTICLES];
	float angle[MAX_PARTICLES];
	float vangle[MAX_PARTICLES];
	float life[MAX_PARTICLES];
	float lifeRcp[MAX_PARTICLES];

	ParticleSystem()
	{
		memset(this, 0, sizeof(*this));

		for (int i = 0; i < MAX_PARTICLES; ++i)
			freeList[i] = i;
		numFree = MAX_PARTICLES;
	}

	bool alloc(const bool _autoKill, float _life, uint16_t & id)
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

			angle[id] = 0.f;
			vangle[id] = 0.f;

			life[id] = _life;
			lifeRcp[id] = 1.f / _life;

			return true;
		}
	}

	void free(uint16_t & id)
	{
		if (isValidIndex(id))
		{
			freeList[numFree++] = id;

			id = -1;
		}
	}

	void tick(const float dt)
	{
		for (int i = 0; i < MAX_PARTICLES; ++i)
		{
			if (alive[i])
			{
				life[i] = life[i] - dt;

				if (life[i] < 0.f)
				{
					life[i] = 0.f;

					if (autoKill[i])
					{
						uint16_t id = i;

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

	void draw()
	{
		gxBegin(GL_QUADS);
		{
			for (int i = 0; i < MAX_PARTICLES; ++i)
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

struct Effect_Rain
{
	static const int kMaxParticles = 1024;

	int m_index;
	ParticleSystem * m_particleSystem;
	uint16_t m_particleIds[kMaxParticles];

	Effect_Rain(ParticleSystem & particleSystem)
	{
		memset(this, 0, sizeof(*this));
		memset(m_particleIds, -1, sizeof(m_particleIds));

		m_particleSystem = &particleSystem;
	}

	void tick(const float dt)
	{
		const float gravityY = 400.f;
		const float falloff = .9f;
		const float falloffThisTick = powf(falloff, dt);

		// spawn particles

		for (int i = 0; i < 2; ++i)
		{
			const int index = m_index;

			m_index = (m_index + 1) % kMaxParticles;

			if (isValidIndex(m_particleIds[index]))
				m_particleSystem->free(m_particleIds[index]);

			uint16_t & id = m_particleIds[index];
			m_particleSystem->alloc(false, 10.f, id);
			if (isValidIndex(id))
			{
				m_particleSystem->x[id] = rand() % GFX_SX;
				m_particleSystem->y[id] = 0.f;
				m_particleSystem->vx[id] = 0.f;
				m_particleSystem->vy[id] = 50.f;
				m_particleSystem->sx[id] = 10.f;
				m_particleSystem->sy[id] = 50.f;
				m_particleSystem->vangle[id] = 1.f;
			}
		}

		// update particles

		for (int i = 0; i < kMaxParticles; ++i)
		{
			uint16_t & id = m_particleIds[i];

			if (!m_particleSystem->alive[id])
				continue;

			// integrate gravity

			m_particleSystem->vy[id] += gravityY * dt;

			// collision and bounce

			if (m_particleSystem->y[id] > GFX_SY)
			{
				m_particleSystem->vy[id] *= -1.f;
			}

			// velocity falloff

			m_particleSystem->vx[id] *= falloffThisTick;
			m_particleSystem->vy[id] *= falloffThisTick;

			// check if the particle is dead

			if (m_particleSystem->life[id] == 0.f || m_particleSystem->y[id] < 0.f)
			{
				m_particleSystem->free(id);
			}
		}
	}
};

#define CLOTHPIECE_MAX_SX 16
#define CLOTHPIECE_MAX_SY 16

struct ClothPiece
{
	struct Vertex
	{
		bool isFixed;
		float x;
		float y;
		float vx;
		float vy;
	};

	int sx;
	int sy;
	Vertex vertices[CLOTHPIECE_MAX_SX][CLOTHPIECE_MAX_SY];

	ClothPiece()
	{
		memset(this, 0, sizeof(*this));
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

				v.isFixed = (y == 0) || (y == sy - 1);

				v.x = x * 1.5f;
				v.y = y * 1.2f;
				v.vx = 0.f;
				v.vy = 0.f;
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

	void tick(const float dt)
	{
		const float eps = 1e-10f;

		const float gravityX = 0.f;
		const float gravityY = keyboard.isDown(SDLK_g) ? 2.f : 0.f;

		const float springConstant = 100.f;
		const float falloff = .1f;
		const float falloffThisTick = powf(falloff, dt);

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

				v.vx *= falloffThisTick;
				v.vy *= falloffThisTick;
			}
		}
	}

	void draw()
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

class MyOscPacketListener : public osc::OscPacketListener
{
protected:
	virtual void ProcessMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint)
	{
		try
		{
			if (strcmp(m.AddressPattern(), "/test1") == 0)
			{
				osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
				bool a1;
				osc::int32 a2;
				float a3;
				const char *a4;
				args >> a1 >> a2 >> a3 >> a4 >> osc::EndMessage;

				log("received '/test1' message with arguments: %d %d %g %s", a1, a2 ,a3, a4);
			}
		}
		catch (osc::Exception & e)
		{
			logError("error while parsing message: %s: %s", m.AddressPattern(), e.what());
		}
	}
};

int main(int argc, char * argv[])
{
	// initialise OSC

	UdpTransmitSocket sendSocket(IpEndpointName(OSC_ADDRESS, OSC_SEND_PORT));

	MyOscPacketListener listener;
	UdpListeningReceiveSocket recvSocket(IpEndpointName(IpEndpointName::ANY_ADDRESS, OSC_RECV_PORT), &listener);

	//recvSocket.Run();

	framework.fullscreen = false;
	//framework.minification = 3;

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		ParticleSystem particleSystem;

		Effect_Rain rain(particleSystem);

		ClothPiece clothPiece;
		clothPiece.setup(CLOTHPIECE_MAX_SX, CLOTHPIECE_MAX_SY);

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

			const float dt = Calc::Min(1.f / 30.f, framework.timeStep);

			// update network input

			// update network output

		#if 0
			char buffer[OSC_OUTPUT_BUFFER_SIZE];
			osc::OutboundPacketStream p(buffer, OSC_OUTPUT_BUFFER_SIZE);

			p
				<< osc::BeginBundleImmediate
				<< osc::BeginMessage("/chat")
				<< freq1 << freq2 << freq3 << 4
				<< osc::EndMessage
				<< osc::EndBundle;

			transmitSocket.Send(p.Data(), p.Size());
		#endif

			// process particle system

			particleSystem.tick(dt);

			// process effects

			rain.tick(dt);

			clothPiece.tick(dt);

			if ((rand() % 30) == 0)
			{
				clothPiece.vertices[rand() % clothPiece.sx][rand() % clothPiece.sy].vx += 20.f;
			}

			// draw

			framework.beginDraw(0, 0, 0, 0);
			{
				particleSystem.draw();

				gxPushMatrix();
				{
					gxScalef(40.f, 40.f, 1.f);

					for (int i = 3; i >= 1; --i)
					{
						glLineWidth(i);
						clothPiece.draw();
					}
				}
				gxPopMatrix();
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
