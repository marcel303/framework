include engine/ShaderVS.txt

varying vec4 v_color;
varying vec2 v_texcoord0;

void main()
{
	vec4 position = unpackPosition();
	
	ivec4 skinningBlendIndices = unpackSkinningBlendIndices();
	vec4 skinningBlendWeights = unpackSkinningBlendWeights();
	
	position = objectToWorld_Skinned(
		skinningBlendIndices,
		skinningBlendWeights,
		position);
	
	position = objectToProjection(position);
	
	vec3 normal = objectToWorld_Skinned(
		skinningBlendIndices,
		skinningBlendWeights,
		unpackNormal()).xyz;
	
	normal = normalize(normal);
	
	vec2 texcoord = unpackTexcoord(0);
	
	// debug color
	
	vec4 color = vec4(1.0);
	
	if (drawColorTexcoords())
		color.rg *= texcoord.xy;
	if (drawColorNormals())
		color.rgb *= (normal + vec3(1.0)) / 2.0;
	if (drawColorBlendIndices())
		color.rgb *= skinningBlendIndices.xyz / 32.0;
	if (drawColorBlendWeights())
		color.rgb *= skinningBlendWeights.xyz;
	
	//
	
	gl_Position = position;
	
	v_texcoord0 = texcoord;
	
	v_color = color;
}
