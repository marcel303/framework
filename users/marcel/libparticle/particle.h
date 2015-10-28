#pragma once

#pragma pack(push)
#pragma pack(1)

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

struct Particle;
struct ParticleCallbacks;
struct ParticleColor;
struct ParticleColorCurve;
struct ParticleCurve;
struct ParticleEmitter;
struct ParticleEmitterInfo;
struct ParticleInfo;
struct ParticlePool;

struct ParticleColor
{
	float rgba[4];

	ParticleColor();
	ParticleColor(bool noinit) { }
	ParticleColor(float r, float g, float b, float a);

	bool operator==(const ParticleColor & other) const;
	bool operator!=(const ParticleColor & other) const;

	void set(float r, float g, float b, float a);
	void modulateWith(const ParticleColor & other);
	void interpolateBetween(const ParticleColor & v1, const ParticleColor & v2, const float t);

	void save(tinyxml2::XMLPrinter * printer);
	void load(tinyxml2::XMLElement * elem);
};

struct ParticleCurve
{
	float valueMin;
	float valueMax; // fixme : only linear for now..

	ParticleCurve();

	void setLinear(float v1, float v2);
	float sample(const float t) const;

	void save(tinyxml2::XMLPrinter * printer);
	void load(tinyxml2::XMLElement * elem);
};

struct ParticleColorCurve
{
	static const int kMaxKeys = 10;

	struct Key
	{
		float t;
		ParticleColor color;

		Key();

		bool operator<(const Key & other) const;
		bool operator==(const Key & other) const;
		bool operator!=(const Key & other) const;
	};

	Key keys[kMaxKeys];
	int numKeys;

	ParticleColorCurve();

	bool operator==(const ParticleColorCurve & other) const;
	bool operator!=(const ParticleColorCurve & other) const;

	bool allocKey(Key *& key);
	void freeKey(Key *& key);
	void clearKeys();
	Key * sortKeys(Key * keyToReturn = 0);
	void setLinear(const ParticleColor & v1, const ParticleColor & v2);
	void setLinearAlpha(float v1, float v2);
	void sample(const float t, ParticleColor & result) const;

	void save(tinyxml2::XMLPrinter * printer);
	void load(tinyxml2::XMLElement * elem);
};

struct ParticleEmitterInfo
{
	char name[16];
	float duration; // The length of time the system will run.
	bool loop; // If enabled, the system will start again at the end of its duration time and continue to repeat the cycle.
	bool prewarm; // If enabled, the system will be initialized as though it had already completed a full cycle (only works if Looping is also enabled).
	float startDelay; // Delay in seconds before the system starts emitting once enabled.
	float startLifetime; // The initial lifetime for particles.
	float startSpeed; // The initial speed of each particle in the appropriate direction.
	float startSize; // The initial size of each particle.
	float startRotation; // The initial rotation angle of each particle.
	ParticleColor startColor; // The initial color of each particle.
	float gravityMultiplier; // Scales the gravity value set in the physics manager. A value of zero will switch gravity off.
	bool inheritVelocity; // Do the particles start with the same velocity as the particle system object?
	bool worldSpace; // Should particles be animated in the parent object’s local space (and therefore move with the object) or in world space?
	int maxParticles; // The maximum number of particles in the system at once. Older particles will be removed when the limit is reached.
	char materialName[32];

	//

	ParticleEmitterInfo();

	bool operator==(const ParticleEmitterInfo & other) const;
	bool operator!=(const ParticleEmitterInfo & other) const;

	void save(tinyxml2::XMLPrinter * printer);
	void load(tinyxml2::XMLElement * elem);
};

struct ParticleInfo
{
	static const int kMaxBursts = 10;

	// emission

	float rate; // The number of particles emitted per unit of time or distance moved (selected from the adjacent popup menu).
	// todo : emit over time or over distance moved

	struct Burst
	{
		Burst()
			: time(0.f)
			, numParticles(10)
		{
		}

		float time;
		int numParticles;
	} bursts[kMaxBursts];
	int numBursts; // Allows extra particles to be emitted at specified times (only available when the Rate is in Time mode).

	// shape

	enum Shape
	{
		kShapeEdge = 'e',
		kShapeBox = 'b',
		kShapeCircle = 'c'
	};

	Shape shape; // The shape of the emission volume.
	bool randomDirection; // When enabled, the particles’ initial direction will be chosen randomly.
	float circleRadius; // The radius of the circular aspect of the shape (Circle only).
	float boxSizeX, boxSizeY; // Width, height of the rect shape (for Rect only).
	bool emitFromShell; // Should particles be emitted from the edge of the circle rather than the centre? (For Circle and Rect only.)

	// velocity over lifetime

	bool velocityOverLifetime;
	float velocityOverLifetimeValueX;
	float velocityOverLifetimeValueY;

	// limit velocity over lifetime

	bool velocityOverLifetimeLimit;
	ParticleCurve velocityOverLifetimeLimitCurve;
	float velocityOverLifetimeLimitDampen;

	// force over lifetime

	bool forceOverLifetime;
	float forceOverLifetimeValueX;
	float forceOverLifetimeValueY;

	// color over lifetime

	bool colorOverLifetime;
	ParticleColorCurve colorOverLifetimeCurve;

	// color by speed

	bool colorBySpeed;
	ParticleColorCurve colorBySpeedCurve;
	float colorBySpeedRangeMin; // The low and high ends of the speed range to which the color gradient is mapped.
	float colorBySpeedRangeMax;

	// size over lifetime

	bool sizeOverLifetime;
	ParticleCurve sizeOverLifetimeCurve;

	// size by speed

	bool sizeBySpeed;
	ParticleCurve sizeBySpeedCurve;
	float sizeBySpeedRangeMin; // The low and high ends of the speed range to which the size curve is mapped.
	float sizeBySpeedRangeMax;

	// rotation over lifetime

	bool rotationOverLifetime;
	float rotationOverLifetimeValue;

	// rotation by speed

	bool rotationBySpeed;
	ParticleCurve rotationBySpeedCurve;
	float rotationBySpeedRangeMin; // The low and high ends of the speed range to which the rotation curve is mapped.
	float rotationBySpeedRangeMax;

	// external forces

	// collision

	bool collision;
	// todo : add plane collision support?
	float bounciness;
	float lifetimeLoss; // The fraction of a particle’s total lifetime that it loses if it collides.
	float minKillSpeed; // Particles travelling below this speed after a collision will be removed from the system.
	float collisionRadius; // Approximate size of a particle, used to avoid clipping with collision planes

	// sub emitters

	bool enableSubEmitters;

	enum SubEmitterEvent
	{
		kSubEmitterEvent_Birth,
		kSubEmitterEvent_Collision,
		kSubEmitterEvent_Death,
		kSubEmitterEvent_COUNT
	};

	struct SubEmitter
	{
		SubEmitter();

		void save(tinyxml2::XMLPrinter * printer);
		void load(tinyxml2::XMLElement * elem);

		bool enabled;
		float chance;
		int count;
		char emitterName[32];
	} subEmitters[kSubEmitterEvent_COUNT];

	// texture sheet animation

	// todo

	// renderer

	enum SortMode
	{
		kSortMode_YoungestFirst = 'y',
		kSortMode_OldestFirst = 'o'
	};

	SortMode sortMode;

	enum BlendMode
	{
		kBlendMode_AlphaBlended = 'b',
		kBlendMode_Additive = 'a'
	};

	BlendMode blendMode;

	//

	ParticleInfo();

	bool operator==(const ParticleInfo & other) const;
	bool operator!=(const ParticleInfo & other) const;

	bool allocBurst(Burst *& burst);
	void clearBursts();

	void save(tinyxml2::XMLPrinter * printer);
	void load(tinyxml2::XMLElement * elem);
};

struct ParticleEmitter
{
	bool active;
	bool delaying;
	float time;
	float totalTime;

	ParticleEmitter();

	bool allocParticle(ParticlePool & pool, Particle *& p);
	void clearParticles(ParticlePool & pool);

	bool emitParticle(const ParticleCallbacks & cbs, const ParticleEmitterInfo & pei, const ParticleInfo & pi, ParticlePool & pool, const float timeOffsetconst, const float gravityX, const float gravityY, Particle *& p);
	void restart(ParticlePool & pool);
};

struct Particle
{
	float life;
	float lifeRcp;
	float position[2];
	float speed[2];
	float speedScalar;
	float rotation;

	Particle * prev;
	Particle * next;

	Particle();
};

struct ParticlePool
{
	Particle * head;
	Particle * tail;

	ParticlePool();

	Particle * allocParticle();
	Particle * freeParticle(Particle * p);
};

struct ParticleCallbacks
{
	int (*randomInt)(int min, int max);
	float (*randomFloat)(float min, float max);
	bool (*getEmitterByName)(const char * name, const ParticleEmitterInfo *& pei, const ParticleInfo *& pi, ParticlePool *& pool, ParticleEmitter *& pe);
	bool (*checkCollision)(float x1, float y1, float x2, float y2, float & t, float & nx, float & ny);

	ParticleCallbacks()
		: randomInt(0)
		, randomFloat(0)
		, getEmitterByName(0)
		, checkCollision(0)
	{
	}
};

bool tickParticle(const ParticleCallbacks & cbs, const ParticleEmitterInfo & pei, const ParticleInfo & pi, const float timeStep, const float gravityX, const float gravityY, Particle & p);
void handleSubEmitter(const ParticleCallbacks & cbs, const ParticleInfo & pi, const float gravityX, const float gravityY, const Particle & p, const ParticleInfo::SubEmitterEvent e);
void getParticleSpawnLocation(const ParticleInfo & pi, float & x, float & y);
void computeParticleColor(const ParticleEmitterInfo & pei, const ParticleInfo & pi, const float particleLife, const float particleSpeed, ParticleColor & result);
float computeParticleSize(const ParticleEmitterInfo & pei, const ParticleInfo & pi, const float particleLife, const float particleSpeed);
float computeParticleRotation(const ParticleEmitterInfo & pei, const ParticleInfo & pi, const float timeStep, const float particleLife, const float particleSpeed, const float particleRotation);

void tickParticleEmitter(const ParticleCallbacks & cbs, const ParticleEmitterInfo & pei, const ParticleInfo & pi, ParticlePool & pool, const float timeStep, const float gravityX, const float gravityY, ParticleEmitter & pe);

#pragma pack(pop)
