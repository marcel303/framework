      wvp          w         position           color           normal           texcoord        �  float4x4 INPUT_wvp;/* : register(c0);*/
float4x4 INPUT_w;/* : register(c4);*/
struct INPUT
{
float4 position : POSITION;
float4 color : COLOR;
float3 normal : NORMAL;
float2 texcoord : TEXCOORD0;
};


struct OUTPUT
{
	float4 position : POSITION;
	float4 color : TEXCOORD0;
	float4 normal : TEXCOORD1;
	float2 texcoord : TEXCOORD2;
};

OUTPUT main(INPUT input)
{
	OUTPUT result;

	result.position = mul(input.position, INPUT_wvp);
	//result.color = input.color;
	result.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	result.normal = float4(mul(input.normal, (float3x3)INPUT_w), 1.0f);
	result.texcoord = input.texcoord;

	return result;
}

