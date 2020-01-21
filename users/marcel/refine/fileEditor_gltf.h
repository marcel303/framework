#pragma once

#include "fileEditor.h"
#include "gltf.h"
#include "gltf-draw.h"
#include "imgui-framework.h"

struct FileEditor_Gltf : FileEditor
{
	gltf::Scene scene;
	gltf::BufferCache bufferCache;
	float rotationX = 0.f;
	float rotationY = 0.f;
	
	bool showBoundingBox = false;
	bool showAxis = false;
	bool enableLighting = true;
	float desiredScale = 1.f;
	float currentScale = 1.f;
	Vec3 ambientLight_color = Vec3(.5f, .5f, .5f);
	float directionalLight_intensity = 1.f;
	Vec3 directionalLight_color = Vec3(.5f, .5f, .5f);
	Vec3 directionalLight_direction = Vec3(1.f, 1.f, -1.f);
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Gltf(const char * path);
	virtual ~FileEditor_Gltf() override;
	
	virtual bool reflect(TypeDB & typeDB, StructuredType & type) override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
