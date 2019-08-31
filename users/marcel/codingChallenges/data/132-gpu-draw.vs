include engine/ShaderVS.txt

shader_out vec4 v_color;
shader_out vec3 v_texcoord;

uniform float depth;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = objectToProjection(position);
	
	v_color = unpackColor();
	v_texcoord = vec3(unpackTexcoord(0), depth);
}