   	   albedoTex          lightTex         texcoord        �  sampler2D INPUT_albedoTex;/* : register(s0);*/
sampler2D INPUT_lightTex;/* : register(s1);*/
struct INPUT
{
float2 texcoord : TEXCOORD0;
};


struct OUTPUT
{
	float4 color : COLOR;
};

OUTPUT main(INPUT input)
{
	OUTPUT result;

	float4 albedo = tex2D(INPUT_albedoTex, input.texcoord);
	float4 lightValue = tex2D(INPUT_lightTex, input.texcoord);
	
	result.color = albedo * lightValue;
	
	return result;
}

