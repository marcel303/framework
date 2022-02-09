include engine/ShaderVS.txt

shader_out vec4 v_color;

void main()
{
	vec4 position = unpackPosition();
	
	position = objectToProjection(position);
	
	vec4 color = unpackColor();
	vec4 normal = unpackNormal();
	
	gl_Position = position;

	color.rgb += normal.rgb * 0.2;
	
	v_color = color;
}
