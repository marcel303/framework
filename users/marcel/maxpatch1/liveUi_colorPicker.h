#pragma once

#include "controlSurfaceDefinition.h"

namespace liveui
{
	struct ColorPicker
	{
		enum State
		{
			kState_Inactive,
			kState_Picker,
			kState_Slider1,
			kState_Slider2
		};

		State state = kState_Inactive;

		bool tick(const int x, const int y, const int sx, const int sy, ControlSurfaceDefinition::Vector4 & color, const ControlSurfaceDefinition::ColorSpace colorSpace, const bool inputIsCaptured);
		void draw(const int x, const int y, const int sx, const int sy, ControlSurfaceDefinition::Vector4 & color, const ControlSurfaceDefinition::ColorSpace colorSpace);
	};
}