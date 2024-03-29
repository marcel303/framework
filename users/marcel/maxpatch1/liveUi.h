#pragma once

#include "controlSurfaceDefinition.h"
#include <vector>

struct OscSender;

struct LiveUi
{
	struct Elem
	{
		ControlSurfaceDefinition::Element * elem = nullptr;
		
		int x = 0;
		int y = 0;
		int sx = 0;
		int sy = 0;
		
		float value = 0.f;
		float defaultValue = 0.f;
		
		ControlSurfaceDefinition::Vector4 value4;
		ControlSurfaceDefinition::Vector4 defaultValue4;
		
		float doubleClickTimer = 0.f;
		bool valueHasChanged = false;
		
		int liveState[4] = { };
		
		void setToDefault();
	};
	
	std::vector<Elem> elems;
	
	Elem * hoverElem = nullptr;
	
	Elem * activeElem = nullptr;
	
	std::vector<OscSender*> oscSenders;
	
	mutable GxTextureId colorPickerTextureSat0 = 0;
	mutable GxTextureId colorPickerTextureSat1 = 0;
	
	//

	~LiveUi();
	
	static GxTextureId generateColorPickerTexture(const float saturation);
	
	void shut();

	LiveUi & osc(const char * ipAddress, const int udpPort);
	
	void addElem(ControlSurfaceDefinition::Element * elem);
	
	Elem * findElem(const ControlSurfaceDefinition::Element * surfaceElement);
	
	void setToDefault();
	
	void forceSendOsc();
	void sendChangedValuesOverOsc();
	
	/**
	 * Apply layouts. The first layout is considered the 'base' layout, while subsequent layouts (if any)
	 * apply overrides to the base layout.
	 */
	void applyLayouts(const ControlSurfaceDefinition::Surface & surface, const ControlSurfaceDefinition::SurfaceLayout * layouts[], const int numLayouts);
	
	/**
	 * Apply a single layout.
	 */
	void applyLayout(const ControlSurfaceDefinition::Surface & surface, const ControlSurfaceDefinition::SurfaceLayout & layout);
	
	void tick(const float dt, bool & inputIsCaptured);
	
	void draw() const;
	void drawTooltip() const;
};
