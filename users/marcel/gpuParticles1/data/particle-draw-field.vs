include engine/ShaderVS.txt
include particle-utils.txt

uniform sampler2D p;

shader_out vec2 v_texcoord;

void main()
{
	int local_index = gl_VertexID % 6;

	vec4 positionAndUv = vertexIndexToParticleLocalPosAndUv(local_index);

	vec2 position = positionAndUv.xy;
	vec2 uv = positionAndUv.zw;
	
	//

	int particle_index = gl_VertexID / 6;
	vec2 sizeRcp = vec2(1.0) / textureSize(p, 0);
	position += texture(p, vec2(sizeRcp.x * (particle_index + 0.5), 0.5)).xy;

	gl_Position = ModelViewProjectionMatrix * vec4(position, 0.0, 1.0);

	v_texcoord = uv;
}
