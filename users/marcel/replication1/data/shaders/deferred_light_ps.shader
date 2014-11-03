   	   normalTex       	   paramsTex         texcoord        R  sampler2D INPUT_normalTex;/* : register(s0);*/
sampler2D INPUT_paramsTex;/* : register(s1);*/
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

	float4 normal = tex2D(INPUT_normalTex, input.texcoord);
	float4 params = tex2D(INPUT_paramsTex, input.texcoord);

	float4 lightDir = float4(1.0f, 1.0f, 0.0f, 0.0f);
	lightDir = normalize(lightDir);

	//float intensity = dot(lightDir, normal);
	float intensity = 1.0f;

	result.color = input.texcoord.xyxy * intensity;

	return result;
}

