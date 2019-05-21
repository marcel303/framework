static const char * s_basicSkinnedVs = R"SHADER(

include engine/ShaderVS.txt

shader_out vec4 v_color;
shader_out vec2 v_texcoord0;
shader_out vec3 v_normal;

void main()
{
	vec4 position = unpackPosition();

	vec3 normal;
	
	vec4 color = unpackColor();

	if (drawUnSkinned())
	{
		position = objectToProjection(position);

		normal = unpackNormal().xyz;

		normal = objectToView3(normal);
		normal = normalize(normal);
	}
	else if (drawHardSkinned())
	{
		ivec4 skinningBlendIndices = unpackSkinningBlendIndices();
		vec4 skinningBlendWeights = unpackSkinningBlendWeights();

		position = boneToObject_HardSkinned(
			skinningBlendIndices,
			skinningBlendWeights,
			position);
		
		position = objectToProjection(position);
		
		normal = boneToObject_HardSkinned(
			skinningBlendIndices,
			skinningBlendWeights,
			unpackNormal()).xyz;

		normal = objectToView3(normal);
		normal = normalize(normal);
	}
	else
	{
		ivec4 skinningBlendIndices = unpackSkinningBlendIndices();
		vec4 skinningBlendWeights = unpackSkinningBlendWeights();
		
		position = boneToObject_Skinned(
			skinningBlendIndices,
			skinningBlendWeights,
			position);
		
		position = objectToProjection(position);
		
		normal = boneToObject_Skinned(
			skinningBlendIndices,
			skinningBlendWeights,
			unpackNormal()).xyz;
		
		normal = objectToView3(normal);
		normal = normalize(normal);

		// debug color

		if (drawColorBlendIndices())
			color.rgb *= vec3(skinningBlendIndices.xyz) / 32.0;
		if (drawColorBlendWeights())
			color.rgb *= skinningBlendWeights.xyz;
	}
	
	vec2 texcoord = unpackTexcoord(0);
	
	// debug color

	if (drawColorTexcoords())
		color.rg *= texcoord.xy;
	if (drawColorNormals())
		color.rgb *= (normalize(unpackNormal()).xyz + vec3(1.0)) / 2.0;
	
	//
	
	gl_Position = position;
	
	v_color = color;
	v_texcoord0 = texcoord;
	v_normal = normal;
}

)SHADER";
