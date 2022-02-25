include engine/ShaderVS.txt

uniform vec3 u_geometryColor;

shader_out vec3 v_color;

void main()
{
	vec4 position = unpackPosition();
	
	position = objectToProjection(position);
	
	vec4 normal = unpackNormal();
	
	gl_Position = position;

	vec3 color = u_geometryColor;

	color += normal.rgb * 0.2;
	
	v_color = color;
}
