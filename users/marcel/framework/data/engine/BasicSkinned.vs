include engine/ShaderVS.txt

varying vec4 color;
varying vec2 texcoord0;

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
	
	//
	
	gl_Position = position;
	
	color = vec4(in_texcoord.xy, 1.0, 1.0);
	//color = vec4(skinningBlendWeights.xyz, 1.0);
	//color = vec4(vec3(skinningBlendIndices.xyz)/30.0, 1.0);
	
	texcoord0 = unpackTexcoord(0);
}
