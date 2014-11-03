      wvp          position           texcoord        l  float4x4 INPUT_wvp;/* : register(c0);*/
struct INPUT
{
float4 position : POSITION;
float2 texcoord : TEXCOORD0;
};


struct OUTPUT
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD0;
};

OUTPUT main(INPUT input)
{
	OUTPUT result;

	result.position = mul(INPUT_wvp, input.position);
	result.texcoord = input.texcoord;

	return result;
}

