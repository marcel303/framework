include engine/ShaderVS.txt

uniform float time;
uniform vec3 cameraPosition;

shader_out vec4 v_color;
shader_out vec2 v_texcoord0;

void integrateDirectionalLight(
	inout vec3 totalLightColor,
	in vec3 position,
	in vec3 normal,
	in vec3 lightDirection,
	in vec3 lightColor)
{
	float diffuse = max(0.0, - dot(lightDirection, normal));

	vec3 viewVector = - normalize(position - cameraPosition);
	vec3 reflectionVector = normalize(normal - 2.0 * dot(normal, lightDirection) * lightDirection);
	float specular = max(0.0, dot(viewVector, reflectionVector));
	specular = pow(specular, 16.0);

	float lightIntensity = diffuse + specular;

	totalLightColor += lightColor * lightIntensity;
}

void integrateOmniLight(
	inout vec3 totalLightColor,
	in vec3 position,
	in vec3 normal,
	in vec3 lightPosition,
	in vec3 lightColor,
	in float lightIntensity)
{
	vec3 delta = position - lightPosition;
	float distance = length(delta);

	float attenuation = 1.0 / (distance * distance + 0.01);

	totalLightColor += lightColor * lightIntensity * attenuation;
}

void main()
{
	vec4 position = unpackPosition();

#if 0
	float distortionAmount = (1.0 - cos(time * 0.0345)) / 2.0;

	position.x += distortionAmount * sin((time * 11000.0 + position.x - position.y + position.z) / 5000.0) * 4000.0;
	position.z += distortionAmount * sin((time * 13000.0 + position.x - position.y + position.z) / 6000.0) * 4000.0;
#endif

	vec4 position_world = objectToWorld(position);

	float distance = length(position_world.xyz - cameraPosition);

	position = objectToProjection(position);
	
	vec3 normal_world = objectToWorld(unpackNormal()).xyz;
	
	normal_world = normalize(normal_world);

	vec3 totalLightColor = vec3(0.0);

	integrateDirectionalLight(
		totalLightColor,
		position_world.xyz,
		normal_world,
		normalize(vec3(1.0, 1.0, 1.0)) * 1.4,
		vec3(4.0, 2.0, 1.0) * max(0.0, 0.3 + 0.25 * sin(time * 1.2)));

#if 1
	integrateDirectionalLight(
		totalLightColor,
		position_world.xyz,
		normal_world,
		normalize(vec3(-1.0, 1.0, 1.0)) * 1.2,
		vec3(1.0, 2.0, 4.0) * max(0.0, 0.3 + 0.25 * sin(time * 1.43)));
#endif

	for (int i = 0; i < 3; ++i)
	{
		float t = time * (i + 1) / 3;

		integrateOmniLight(
			totalLightColor,
			position_world.xyz,
			normal_world,
			vec3(sin(t / 1.234), sin(t / 2.345), sin(t / 3.456)) * 140.0,
			vec3(0.5, 0.1, 0.5),
			10000.0);
	}

	vec2 texcoord = unpackTexcoord(0);
	
	// debug color
	
	vec4 color = vec4(1.0);
	
	if (drawColorTexcoords())
		color.rg *= texcoord.xy;
	if (drawColorNormals())
		color.rgb *= (normalize(unpackNormal()).xyz + vec3(1.0)) / 2.0;
	
	color.rgb = pow(color.rgb, vec3(2.2));
	color.rgb *= totalLightColor;
	color.rgb = pow(color.rgb, vec3(1.0 / 2.2));

	vec3 fogColor = vec3(0.2, 0.1, 0.05);
	color.rgb = mix(color.rgb, fogColor, distance / 5000.0);

	//
	
	gl_Position = position;
	
	v_color = color;
	v_texcoord0 = texcoord;
}
