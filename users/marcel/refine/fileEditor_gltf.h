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
	float desiredScale = 1.f;
	float currentScale = 1.f;
	
	gltf::AlphaMode alphaMode = gltf::kAlphaMode_AlphaBlend;
	bool sortDrawablesByViewDistance = false;
	bool enableBufferCache = true;
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Gltf(const char * path);
	virtual ~FileEditor_Gltf() override;
	
	virtual bool reflect(TypeDB & typeDB, StructuredType & type) override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
