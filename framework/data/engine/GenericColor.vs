include engine/ShaderVS.txt

shader_out vec4 v_color;

void main()
{
	vec4 position = unpackPosition();
	
	position = objectToProjection(position);
	
	vec4 color = unpackColor();
	
	gl_Position = position;
	
	v_color = color;
}
