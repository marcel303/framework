#pragma once

namespace gltf
{
	struct Scene;
}

bool loadGltf(const char * path, gltf::Scene & scene);
