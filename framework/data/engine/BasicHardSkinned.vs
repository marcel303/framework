static const char * s_basicHardSkinnedVs = R"SHADER(

include engine/ShaderVS.txt

shader_out vec4 v_color;
shader_out vec3 v_normal;
shader_out vec2 v_texcoord0;

void main()
{
	vec4 position = unpackPosition();

	position = objectToProjection(position);
	
	vec3 normal = unpackNormal().xyz;

	normal = objectToView3(normal);
	normal = normalize(normal);
	
	vec2 texcoord = unpackTexcoord(0);
	
	vec4 color = unpackColor();

	// debug color
	
	if (drawColorTexcoords())
		color.rg *= texcoord.xy;
	if (drawColorNormals())
		color.rgb *= (normalize(unpackNormal()).xyz + vec3(1.0)) / 2.0;
	
	//
	
	gl_Position = position;
	
	v_color = color;
	v_normal = normal;
	v_texcoord0 = texcoord;
}

)SHADER";
