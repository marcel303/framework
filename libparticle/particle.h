#pragma once

#include <string>

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

	void set(float r, float g, float b, float a);
	void modulateWith(const ParticleColor & other);
	void interpolateBetween(const ParticleColor & v1, const ParticleColor & v2, const float t);
	void interpolateBetweenLinear(const ParticleColor & v1, const ParticleColor & v2, const float t);

	void save(tinyxml2::XMLPrinter * printer) const;
	void load(const tinyxml2::XMLElement * elem);
};

struct ParticleCurve
{
	struct Key
	{
		float t;
		float value;
		
		Key();
		
		bool operator<(const Key & other) const;
	};
	
	Key * keys;
	int numKeys;

	ParticleCurve();

	Key * allocKey();
	void freeKey(Key *& key);
	void clearKeys();
	Key * sortKeys(Key * keyToReturn = 0);
	
	void setLinear(float v1, float v2);
	float sample(const float t) const;

	void save(tinyxml2::XMLPrinter * printer) const;
	void load(const tinyxml2::XMLElement * elem);
};

struct ParticleColorCurve
{
	struct Key
	{
		float t;
		ParticleColor color;

		Key();

		bool operator<(const Key & other) const;
	};

	Key * keys;
	int numKeys;
	
	bool useLinearColorSpace;

	ParticleColorCurve();

	Key * allocKey();
	void freeKey(Key *& key);
	void clearKeys();
	Key * sortKeys(Key * keyToReturn = 0);
	
	void setLinear(const ParticleColor & v1, const ParticleColor & v2);
	void setLinearAlpha(float v1, float v2);
	void sample(const float t, const bool linearColorSpace, ParticleColor & result) const;

	void save(tinyxml2::XMLPrinter * printer) const;
	void load(const tinyxml2::XMLElement * elem);
};

struct ParticleEmitterInfo
{
	char name[16];
	float duration;           // The length of time the system will run.
	bool loop;                // If enabled, the system will start again at the end of its duration time and continue to repeat the cycle.
	bool prewarm;             // If enabled, the system will be initialized as though it had already completed a full cycle (only works if Looping is also enabled).
	float startDelay;         // Delay in seconds before the system starts emitting once enabled.
	float startLifetime;      // The initial lifetime for particles.
	float startSpeed;         // The initial speed of each particle in the appropriate direction.
	float startSpeedAngle;    // The initial movement direction of each particle, unless random start direction is enabled.
	float startSize;          // The initial size of each particle.
	float startRotation;      // The initial rotation angle of each particle.
	ParticleColor startColor; // The initial color of each particle.
	float gravityMultiplier;  // Scales the gravity value set in the physics manager. A value of zero will switch gravity off.
	bool inheritVelocity;     // Do the particles start with the same velocity as the particle system object?
	bool worldSpace;          // Should particles be animated in the parent object�s local space (and therefore move with the object) or in world space?
	int maxParticles;         // The maximum number of particles in the system at once. Older particles will be removed when the limit is reached.
	char materialName[32];

	//

	ParticleEmitterInfo();

	void save(tinyxml2::XMLPrinter * printer) const;
	void load(const tinyxml2::XMLElement * elem);
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
		kShapeCircle = 'c',
		kShapeBox3d = 'B',
		kShapeSphere = 's'
	};

	Shape shape;                        // The shape of the emission volume.
	bool randomDirection;               // When enabled, the particles� initial direction will be chosen randomly.
	float circleRadius;                 // The radius of the circular aspect of the shape (Circle and Sphere only).
	float boxSizeX, boxSizeY, boxSizeZ; // Width, height and depth of the box shape (for Box and Box3d only).
	bool emitFromShell;                 // Should particles be emitted from the edge of the circle rather than the centre? (For Circle and Rect only.)

	// -- velocity over lifetime

	bool velocityOverLifetime;
	float velocityOverLifetimeValueX;
	float velocityOverLifetimeValueY;
	float velocityOverLifetimeValueZ;

	// -- limit velocity over lifetime

	bool velocityOverLifetimeLimit;
	ParticleCurve velocityOverLifetimeLimitCurve;
	float velocityOverLifetimeLimitDampen;

	// -- force over lifetime

	bool forceOverLifetime;
	float forceOverLifetimeValueX;
	float forceOverLifetimeValueY;
	float forceOverLifetimeValueZ;

	// -- color over lifetime

	bool colorOverLifetime;
	ParticleColorCurve colorOverLifetimeCurve;

	// -- color by speed

	bool colorBySpeed;
	ParticleColorCurve colorBySpeedCurve;
	float colorBySpeedRangeMin; // The low and high ends of the speed range to which the color gradient is mapped.
	float colorBySpeedRangeMax;

	// -- size over lifetime

	bool sizeOverLifetime;
	ParticleCurve sizeOverLifetimeCurve;

	// -- size by speed

	bool sizeBySpeed;
	ParticleCurve sizeBySpeedCurve;
	float sizeBySpeedRangeMin; // The low and high ends of the speed range to which the size curve is mapped.
	float sizeBySpeedRangeMax;

	// -- rotation over lifetime

	bool rotationOverLifetime;
	float rotationOverLifetimeValue;

	// -- rotation by speed

	bool rotationBySpeed;
	ParticleCurve rotationBySpeedCurve;
	float rotationBySpeedRangeMin; // The low and high ends of the speed range to which the rotation curve is mapped.
	float rotationBySpeedRangeMax;

	// -- collision

	bool collision;
	float bounciness;
	float lifetimeLoss;    // The fraction of a particle�s total lifetime that it loses if it collides.
	float minKillSpeed;    // Particles travelling below this speed after a collision will be removed from the system.
	float collisionRadius; // Approximate size of a particle, used to avoid clipping with collision planes

	// -- sub emitters

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

		void save(tinyxml2::XMLPrinter * printer) const;
		void load(const tinyxml2::XMLElement * elem);

		bool enabled;
		float chance;
		int count;
		char emitterName[32];
	} subEmitters[kSubEmitterEvent_COUNT];

	// -- drawing

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

	bool allocBurst(Burst *& burst);
	void clearBursts();

	void save(tinyxml2::XMLPrinter * printer) const;
	void load(const tinyxml2::XMLElement * elem);
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

	bool emitParticle(
		const ParticleCallbacks & cbs,
		const ParticleEmitterInfo & pei,
		const ParticleInfo & pi, ParticlePool & pool,
		const float timeOffsetconst,
		const float gravityX,
		const float gravityY,
		const float gravityZ,
		const float positionX,
		const float positionY,
		const float positionZ,
		const float speedX,
		const float speedY,
		const float speedZ,
		Particle *& p);
	void restart(ParticlePool & pool);
};

struct Particle
{
	float life;        // Remaining lifetime (in seconds).
	float lifeRcp;     // 1 / the initial lifetime. When life is multiplied with this values, the result will be between 0 and 1.
	float position[3]; // The current position of the particle.
	float speed[3];    // The current speed of the particle (in units per second).
	float speedScalar;
	float rotation;    // The current rotation (in degrees) of the particle.

	Particle * prev;
	Particle * next;

	Particle();
};

struct ParticlePool
{
	Particle * head;
	Particle * tail;

	ParticlePool();
	~ParticlePool();

	Particle * allocParticle();
	Particle * freeParticle(Particle * p);
};

struct ParticleCallbacks
{
	void * userData;

	// -- random number generation. roll your own (deterministic or not) rng
	int   (*randomInt  )(void * userData, int   min, int   max);
	float (*randomFloat)(void * userData, float min, float max);
	
	// -- sub-emitters
	bool (*getEmitterByName)(
		void * userData,
		const char * name,
		const ParticleEmitterInfo *& pei,
		const ParticleInfo *& pi,
		ParticlePool *& pool,
		ParticleEmitter *& pe);
	
	// -- collision detection
	bool (*checkCollision)(
		void * userData,
		float x1, float y1, float z1,
		float x2, float y2, float z2,
		float & t,
		float & nx, float & ny, float & nz);

	ParticleCallbacks()
		: userData(0)
		, randomInt(0)
		, randomFloat(0)
		, getEmitterByName(0)
		, checkCollision(0)
	{
	}
};

struct ParticleSystem
{
	ParticleEmitterInfo emitterInfo;
	ParticleInfo particleInfo;
	ParticleEmitter emitter;
	ParticlePool pool;
	std::string basePath;

	~ParticleSystem();

	bool tick(
		const ParticleCallbacks & cbs,
		const float gravityX,
		const float gravityY,
		const float gravityZ,
		float dt);
	
	void draw() const;

	void restart();
};

bool tickParticle(
	const ParticleCallbacks & cbs,
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const float timeStep,
	const float gravityX,
	const float gravityY,
	const float gravityZ,
	Particle & p);

void handleSubEmitter(
	const ParticleCallbacks & cbs,
	const ParticleInfo & pi,
	const float gravityX,
	const float gravityY,
	const float gravityZ,
	const Particle & p,
	const ParticleInfo::SubEmitterEvent e);

void getParticleSpawnLocation(
	const ParticleCallbacks & cbs,
	const ParticleInfo & pi,
	float & x,
	float & y,
	float & z);

void computeParticleColor(
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const float particleLife,
	const float particleSpeed,
	ParticleColor & result);

float computeParticleSize(
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const float particleLife,
	const float particleSpeed);

float computeParticleRotation(
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const float timeStep,
	const float particleLife,
	const float particleSpeed,
	const float particleRotation);

bool tickParticleEmitter(
	const ParticleCallbacks & cbs,
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	ParticlePool & pool,
	const float timeStep,
	const float gravityX,
	const float gravityY,
	const float gravityZ,
	ParticleEmitter & pe);

void drawParticles(
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const ParticlePool & pool,
	const char * basePath);
