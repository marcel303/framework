#pragma once

// depth to linear shader

static const char * s_depthToLinearVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_depthToLinearPs = R"SHADER(
	include engine/ShaderPS.txt

	uniform float projection_zNear;
	uniform float projection_zFar;
	uniform sampler2D depthTexture;

	shader_in vec2 texcoord;

	void main()
	{
		// depends on our implementation of MakePerspectiveGL actually,
		// so I should really have derived this myself. but taking a
		// shortcut and copy pasting the code from here,
		// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#setting-up-the-rendertarget-and-the-mvp-matrix
		// works just as it should
		
		float z_b = texture(depthTexture, texcoord).x;
		float z_n = 2.0 * z_b - 1.0;
		float z_e = 2.0 * projection_zNear * projection_zFar / (projection_zFar + projection_zNear - z_n * (projection_zFar - projection_zNear));
		
		shader_fragColor = vec4(z_e);
	}
)SHADER";

// shaded object (forward lighting) shader

static const char * s_shadedObjectVs = R"SHADER(
	include engine/ShaderVS.txt

	uniform mat4x4 lightMVP;

	shader_out vec4 color;
	shader_out vec3 normal;
	shader_out vec4 position_lightSpace;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		normal = unpackNormal().xyz;
		normal = objectToView3(normal);
		normal = normalize(normal);
		
		position_lightSpace = lightMVP * in_position4;
		
		color = unpackColor();
	}
)SHADER";

static const char * s_shadedObjectPs = R"SHADER(
	include engine/ShaderPS.txt

	#define DEBUG 0

	uniform sampler2D depthTexture;
	uniform sampler2D normalTexture;

	shader_in vec4 color;
	shader_in vec3 normal;
	shader_in vec4 position_lightSpace;

	void main()
	{
		shader_fragColor = color;
		
		vec3 projected = position_lightSpace.xyz / position_lightSpace.w;
		
		vec3 coords = projected * 0.5 + vec3(0.5);
		float depth = texture(depthTexture, coords.xy).x;
		
	#if DEBUG
		if (projected.x < -1.0 || projected.x > +1.0)
			shader_fragColor.rgb = vec3(0.5, 0.0, 0.0);
		else if (projected.y < -1.0 || projected.y > +1.0)
			shader_fragColor.rgb = vec3(0.0, 0.5, 0.0);
		else if (projected.z < -1.0 || projected.z > +1.0)
			shader_fragColor.rgb = vec3(0.0, 0.0, 0.5);
		else if (false)
			shader_fragColor.rgb = vec3(projected.z);
		else if (false)
			shader_fragColor.rgb = vec3(coords); // RGB color cube
		else if (false)
			shader_fragColor.rgb = vec3(depth);
		else if (false)
			shader_fragColor.rgb = vec3(coords.z > depth ? 0.5 : 1.0); // correct shadow mapping with serious acne
		else
			shader_fragColor.rgb = vec3(coords.z > depth + 0.001 ? 0.3 : 1.0); // less acne with depth bias
	#else
		shader_fragColor.rgb = vec3(coords.z > depth + 0.001 ? 0.3 : 1.0); // less acne with depth bias
	#endif
	
		// apply some color
		
		shader_fragColor.rgb += color.rgb * 0.3;
		
		// apply a bit of lighting here
		
		float distance = length(projected.xy);
		shader_fragColor.rgb *= max(0.0, 1.0 - distance);
		shader_fragColor.rgb += vec3(0.1);
		
		shader_fragNormal = vec4(normal, 0.0);
	}
)SHADER";

// shadow mapping utility functions include

static const char * s_shadowUtilsTxt = R"SHADER(
	vec3 depthToWorldPosition(float depth, vec2 texcoord, mat4x4 projectionToWorld)
	{
		vec3 coord = vec3(texcoord, depth) * 2.0 - vec3(1.0);

		vec4 position_projection = vec4(coord, 1.0);
		vec4 position_world = projectionToWorld * position_projection;

		position_world /= position_world.w;

		return position_world.xyz;
	}
)SHADER";

// depth to world shader

static const char * s_depthToWorldVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_depthToWorldPs = R"SHADER(
	include engine/ShaderPS.txt
	include shadowUtils.txt

	uniform sampler2D depthTexture;
	uniform mat4x4 projectionToWorld;

	shader_in vec2 texcoord;

	void main()
	{
		float depth = texture(depthTexture, texcoord).x;
		
		vec3 position_world = depthToWorldPosition(depth, texcoord, projectionToWorld);
		
		shader_fragColor = vec4(position_world, 1.0);
	}
)SHADER";

// deferred light shader

static const char * s_deferredLightVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_deferredLightPs = R"SHADER(
	include engine/ShaderPS.txt
	include shadowUtils.txt

	#define DEBUG 0

	uniform sampler2D depthTexture;
	uniform mat4x4 projectionToWorld;

	uniform sampler2D normalTexture;

	uniform mat4x4 lightMVP;
	uniform vec3 lightColor;

	shader_in vec2 texcoord;

	void main()
	{
		float camera_view_depth = texture(depthTexture, texcoord).x;
		
		if (camera_view_depth == 1.0)
		{
			// scene background
			shader_fragColor = vec4(0.0);
			return;
		}
		
		vec3 position_world = depthToWorldPosition(camera_view_depth, texcoord, projectionToWorld);
		vec4 position_lightSpace = lightMVP * vec4(position_world, 1.0);
		
		if (position_lightSpace.w <= 0.0)
		{
			// facing in the other direction
			shader_fragColor = vec4(0.0);
			return;
		}
		
		vec3 projected = position_lightSpace.xyz / position_lightSpace.w;
		vec3 coords = projected * 0.5 + vec3(0.5);
	
		// apply a bit of lighting here
		
		shader_fragColor.rgb = lightColor;
		
		// attenuation
		
		float distanceXY = length(projected.xy);
		float distanceZ = max(0.0, coords.z);
	// todo : make curve value a uniform
		float curveValue = 0.5;
		shader_fragColor.rgb *= pow(max(0.0, 1.0 - distanceXY), curveValue) * max(0.0, 1.0 - distanceZ);
		
		//
		
		shader_fragColor.a = 1.0; // has to be 1.0 because BLEND_ADD multiplies the rgb with this value before addition
	}
)SHADER";

// deferred shadow light shader

static const char * s_deferredLightWithShadowVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_deferredLightWithShadowPs = R"SHADER(
	include engine/ShaderPS.txt
	include shadowUtils.txt

	#define DEBUG 0

	uniform sampler2D depthTexture;
	uniform mat4x4 projectionToWorld;

	uniform sampler2D normalTexture;

	uniform sampler2D lightDepthTexture;
	uniform mat4x4 lightMVP;
	uniform vec3 lightColor;
	uniform vec3 lightPosition_view;

	uniform mat4x4 viewMatrix;

	shader_in vec2 texcoord;

	void main()
	{
		float camera_view_depth = texture(depthTexture, texcoord).x;
		
		if (camera_view_depth == 1.0)
		{
			// scene background
			shader_fragColor = vec4(0.0);
			return;
		}
		
		vec3 position_world = depthToWorldPosition(camera_view_depth, texcoord, projectionToWorld);
		vec4 position_lightSpace = lightMVP * vec4(position_world, 1.0);
	
		if (position_lightSpace.w <= 0.0)
		{
			// facing in the other direction
			shader_fragColor = vec4(0.0);
			return;
		}
		
		vec3 projected = position_lightSpace.xyz / position_lightSpace.w;
		vec3 coords = projected * 0.5 + vec3(0.5);
		
		// perform shadow map lookup
		
		float depth = texture(lightDepthTexture, coords.xy).x;
		
		vec3 shadowColor; // todo : make this a single float, and refactor debug code below
		
		if (depth == 1.0)
		{
			// maximum
			shadowColor = vec3(1.0);
		}
		else
		{
		#if DEBUG
			if (projected.x < -1.0 || projected.x > +1.0)
				shadowColor = vec3(0.5, 0.0, 0.0);
			else if (projected.y < -1.0 || projected.y > +1.0)
				shadowColor = vec3(0.0, 0.5, 0.0);
			else if (projected.z < -1.0 || projected.z > +1.0)
				shadowColor = vec3(0.0, 0.0, 0.5);
			else if (false)
				shadowColor = vec3(projected.z);
			else if (false)
				shadowColor = vec3(coords); // RGB color cube
			else if (false)
				shadowColor = vec3(depth);
			else if (false)
				shadowColor = vec3(coords.z > depth ? 0.5 : 1.0); // correct shadow mapping with serious acne
			else
				shadowColor = vec3(coords.z > depth + 0.001 ? 0.3 : 1.0); // less acne with depth bias
		#else
			shadowColor = vec3(coords.z > depth + 0.001 ? 0.0 : 1.0); // less acne with depth bias
		#endif
		}
	
		// apply a bit of lighting here
		
		shader_fragColor.rgb = shadowColor * lightColor;
		
		// attenuation
		
		float distanceXY = length(projected.xy);
		float distanceZ = max(0.0, coords.z);
	// todo : make curve value a uniform
		float curveValue = 0.5;
		shader_fragColor.rgb *= pow(max(0.0, 1.0 - distanceXY), curveValue) * max(0.0, 1.0 - distanceZ);
		
		// diffuse
		
		vec3 position_view = (viewMatrix * vec4(position_world, 1.0)).xyz;
		vec3 delta = position_view - lightPosition_view;
		vec3 normal_view = texture(normalTexture, texcoord).xyz;
		float amount = dot(normal_view, -normalize(position_view.xyz)); // fixme ! normal should be in light space, or light in view space. now they aren't in the same spaces !
		shader_fragColor.rgb *= amount;
		
		//
		
		shader_fragColor.a = 1.0; // has to be 1.0 because BLEND_ADD multiplies the rgb with this value before addition
	}
)SHADER";

// light application shader

static const char * s_lightApplicationVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_lightApplicationPs = R"SHADER(
	include engine/ShaderPS.txt

	uniform sampler2D colorTexture;
	uniform sampler2D lightTexture;

	uniform vec3 ambient;

	shader_in vec2 texcoord;

	void main()
	{
		vec3 color = texture(colorTexture, texcoord).xyz;
		vec3 light = texture(lightTexture, texcoord).xyz;
		
		light += ambient;
		
		vec3 result = color * light;
	
		shader_fragColor = vec4(result, 1.0);
	}
)SHADER";
