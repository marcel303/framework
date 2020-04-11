/*

ShadowMapDrawer assists in the drawing of shadow maps

it allows one to register a number of lights
and render functions for drawing geometry

on draw, it will prioritize lights to determine whichs ones to draw
shadow maps for. these shadow maps are then packed into a
shadow map atlas

the depth values from the shadow map atlas can be
fetched from a shader using the shader utils provided
by the library. these can then be directly compared with
depths from arbitrary points to a light after transforming
them to the light's clip space using its shadow matrix

note : shadow maps are packed into an atlas, to make it easier
       to bind all of the shadow maps. this makes it suitable for
       forward rendering, with an arbitrary number of lights with
       optional shadows

*/

#include "framework.h"
#include "gx_render.h"
#include "Mat4x4.h"
#include "renderer.h"
#include <vector>

namespace rOne
{
	class ShadowMapDrawer
	{

	private:

		enum LightType
		{
			kLightType_Spot,
			kLightType_Directional
		};

		struct Light
		{
			int id;

			LightType type;
			Mat4x4 lightToWorld;
			Mat4x4 worldToLight;
			float nearDistance;
			float farDistance;
			float spotAngle;
			float directionalExtents;
			
			float viewDistance; // for priority sort
		};
		
		std::vector<DepthTarget> depthTargets;
		std::vector<ColorTarget> colorTargets;
		
		std::vector<Light> lights;
		
		ShaderBuffer shaderBuffer;
		
		ColorTarget depthAtlas;
		ColorTarget colorAtlas;
		
		static void calculateProjectionMatrixForLight(const Light & light, Mat4x4 & projectionMatrix);
		
		void generateDepthAtlas();
		void generateColorAtlas();
		
	public:

		RenderFunction drawOpaque;
		RenderFunction drawTranslucent;
		
		bool enableColorShadows = false;

		~ShadowMapDrawer();
		
		void alloc(const int maxShadowMaps, const int resolution);
		void free();
		
		void addSpotLight(
			const int id,
			const Mat4x4 & lightToWorld,
			const float angle,
			const float nearDistance,
			const float farDistance);
		void addDirectionalLight(
			const int id,
			const Mat4x4 & lightToWorld,
			const float startDistance,
			const float endDistance,
			const float extents);
		
		void drawShadowMaps(const Mat4x4 & worldToView);
		
		void setShaderData(Shader & shader, int & nextTextureUnit, const Mat4x4 & worldToView);
		int getShadowMapId(const int id) const;
		
		void reset();
		
		void showRenderTargets() const;
	};
}
