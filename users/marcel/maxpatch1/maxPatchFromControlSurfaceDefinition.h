#pragma once

namespace ControlSurfaceDefinition
{
	struct Surface;
}

namespace max
{
	struct Patch;
}

bool maxPatchFromControlSurfaceDefinition(const ControlSurfaceDefinition::Surface & surface, max::Patch & patch);
