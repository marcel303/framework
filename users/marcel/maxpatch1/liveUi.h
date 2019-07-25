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
		ControlSurfaceDefinition::Vector4 defaultValue4;
		float doubleClickTimer = 0.f;
		bool valueHasChanged = false;
		ControlSurfaceDefinition::Vector4 value4;
		
		int liveState[4] = { };
	};
	
	std::vector<Elem> elems;
	
	Elem * hoverElem = nullptr;
	
	Elem * activeElem = nullptr;
	
	std::vector<OscSender*> oscSenders;
	
	//
	
	LiveUi & osc(const char * ipAddress, const int udpPort);
	
	void addElem(ControlSurfaceDefinition::Element * elem, ControlSurfaceDefinition::ElementLayout * layoutElement);
	
	Elem * findElem(const ControlSurfaceDefinition::Element * surfaceElement);
	
	void tick(const float dt, bool & inputIsCaptured);
	
	void draw() const;
	void drawTooltip() const;
};
