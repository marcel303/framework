include engine/ShaderVS.txt
include particle-utils.txt

uniform sampler2D p;

shader_out vec2 v_velocity;

void main()
{
	int local_index = gl_VertexID % 6;

	vec4 positionAndUv = vertexIndexToParticleLocalPosAndUv(local_index);

	vec2 position = positionAndUv.xy;
	vec2 uv = positionAndUv.zw;
	
	//

	int particle_index = gl_VertexID / 6;
	vec4 particlePositionAndVelocity = lookupParticlePositionAndVelocity(p, particle_index);
	position += particlePositionAndVelocity.xy;

	gl_Position = ModelViewProjectionMatrix * vec4(position, 0.0, 1.0);

	v_velocity = particlePositionAndVelocity.zw;
}
