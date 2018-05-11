#pragma once

#include "audioMixer.h"
#include "objects/binauralizer.h"

#define USE_STREAMING 0

#define ENABLE_WELCOME 0

extern const int GFX_SX;
extern const int GFX_SY;

extern SDL_mutex * g_audioMutex;
extern binaural::Mutex * g_binauralMutex;
extern binaural::HRIRSampleSet * g_sampleSet;
extern AudioMixer * g_audioMixer;

struct Starfield;
struct SpacePoints;
struct World;

struct GraphEdit;
struct GraphEdit_RealTimeConnection;
struct GraphEdit_TypeDefinitionLibrary;
struct VfxGraph;

struct VideoLandscape
{
	const int kFontSize = 16;
	
	float fov = 90.f;
	float near = .01f;
	float far = 100.f;
	
	World * world = nullptr;
	
	Starfield * starfield = nullptr;
	
	SpacePoints * spacePoints = nullptr;
	
	//
	
	void init();
	void shut();
	
	void tick(const float dt);
	void draw();
};

struct VfxGraphInstance
{
	VfxGraph * vfxGraph = nullptr;
	
	GraphEdit * graphEdit = nullptr;
	
	GraphEdit_RealTimeConnection * realTimeConnection = nullptr;
	
	~VfxGraphInstance();
};

struct VfxGraphMgr
{
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	std::vector<VfxGraphInstance*> instances;
	
	VfxGraphInstance * activeInstance;
	
	VfxGraphMgr();
	~VfxGraphMgr();
	
	VfxGraphInstance * createInstance(const char * filename);
	
	void freeInstance(VfxGraphInstance *& instance);
	
	void tick(const float dt);
	void draw();
};

extern VfxGraphMgr * g_vfxGraphMgr;
