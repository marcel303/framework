include engine/ShaderVS.txt

shader_out vec4 v_color;
shader_out vec3 v_normal;
shader_out vec2 v_texcoord0;

void main()
{
	vec4 position = unpackPosition();
	
	position = objectToProjection(position);
	
	vec4 color = unpackColor();
	
	vec3 normal = unpackNormal().xyz;

	normal = objectToView3(normal);
	normal = normalize(normal);
	
	vec2 texcoord = unpackTexcoord(0);
	
	gl_Position = position;
	
	v_normal = normal;
	v_color = color;
	v_texcoord0 = texcoord;
}
