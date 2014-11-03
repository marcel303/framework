   	   albedoTex          color           normal           texcoord        �  sampler2D INPUT_albedoTex;/* : register(s0);*/
struct INPUT
{
float4 color : TEXCOORD0;
float4 normal : TEXCOORD1;
float2 texcoord : TEXCOORD2;
};


struct OUTPUT
{
	float4 normal : COLOR0;
	float4 albedo : COLOR1;
	float4 params : COLOR2;
};

OUTPUT main(INPUT input)
{
	OUTPUT result;

	result.normal = float4(input.normal.xyz, 1.0f);
	result.albedo = input.color;// + tex2D(INPUT_albedoTex, input.texcoord);
	result.albedo = tex2D(INPUT_albedoTex, input.texcoord)  * 0.2f + input.color;
	//result.albedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
	//result.albedo = input.texcoord.xyxy;
	result.params = float4(1.0f, 1.0f, 1.0f, 1.0f);

	return result;
}

