static const char * s_basicHardSkinnedVs = R"SHADER(

include engine/ShaderVS.txt

shader_out vec4 v_color;
shader_out vec2 v_texcoord0;

void main()
{
	vec4 position = unpackPosition();

	position = objectToProjection(position);
	
	vec3 normal = objectToWorld(unpackNormal()).xyz;
	
	normal = normalize(normal);
	
	vec2 texcoord = unpackTexcoord(0);
	
	// debug color
	
	vec4 color = vec4(1.0);
	
	if (drawColorTexcoords())
		color.rg *= texcoord.xy;
	if (drawColorNormals())
		color.rgb *= (normal + vec3(1.0)) / 2.0;
	
	//
	
	gl_Position = position;
	
	v_color = color;
	v_texcoord0 = texcoord;
}

)SHADER";
