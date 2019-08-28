#pragma once

#include "fileEditor.h"
#include "imgui-framework.h"

struct FileEditor_Model : FileEditor
{
	Model model;
	Vec3 min;
	Vec3 max;
	float rotationX = 0.f;
	float rotationY = 0.f;
	
	bool showColorNormals = true;
	bool showColorTexCoords = false;
	bool showNormals = false;
	float normalsScale = 1.f;
	bool showBones = false;
	bool showBindPose = false;
	bool showUnskinned = false;
	bool showHardskinned = false;
	bool showColorBlendIndices = false;
	bool showColorBlendWeights = false;
	bool showBoundingBox = false;
	bool enableLighting = true;
	float scale = 1.f;
	Vec3 ambientLight_color = Vec3(.5f, .5f, .5f);
	Vec3 directionalLight_color = Vec3(.5f, .5f, .5f);
	Vec3 directionalLight_direction = Vec3(1.f, 1.f, -1.f);
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Model(const char * path);
	virtual ~FileEditor_Model() override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
