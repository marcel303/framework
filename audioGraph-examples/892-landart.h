#pragma once

#include <stdint.h>
#include <vector>

extern const int GFX_SX;
extern const int GFX_SY;

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
	
	float scrollSpeed = 1.f;
	
	int activeVideo = 0;
	
	void init();
	void shut();
	
	void tick(const float dt);
	
	uint32_t getVideoTexture() const;
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
