include engine/ShaderSkinnedVS.txt

shader_out vec3 v_position_view;
shader_out vec4 v_color;
shader_out vec2 v_texcoord0;
shader_out vec3 v_normal_view;

void main()
{
	ivec4 skinningBlendIndices = unpackSkinningBlendIndices();
	vec4 skinningBlendWeights = unpackSkinningBlendWeights();

	vec4 position = boneToObject_Skinned(
		skinningBlendIndices,
		skinningBlendWeights,
		unpackPosition());

	gl_Position = objectToProjection(position);

	v_position_view = objectToView(position).xyz;

	v_texcoord0 = unpackTexcoord(0);

	vec4 normal = boneToObject_Skinned(
		skinningBlendIndices,
		skinningBlendWeights,
		unpackNormal());

	v_normal_view = objectToView3(normal.xyz);
}
