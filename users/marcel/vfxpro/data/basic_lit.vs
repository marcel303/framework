include engine/ShaderVS.txt
include ShaderConstants.h

shader_out vec2 texcoord;
shader_out vec3 position;
shader_out vec3 normal;

void main()
{
	gl_Position = ModelViewProjectionMatrix * in_position4;
	
	texcoord = vec2(in_texcoord);
	position = (ModelViewMatrix * in_position4).xyz;
	normal = (ModelViewMatrix * vec4(in_normal, 0.f)).xyz;
}
