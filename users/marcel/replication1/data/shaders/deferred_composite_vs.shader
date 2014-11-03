      wvp          position           texcoord        |  float4x4 INPUT_wvp;/* : register(c0);*/
typedef struct
{
float4 position : POSITION;
float2 texcoord : TEXCOORD0;
} INPUT;


typedef struct
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD0;
} OUTPUT;

OUTPUT main(INPUT input)
{
	OUTPUT result;

	result.position = mul(INPUT_wvp, input.position);
	result.texcoord = input.texcoord;

	return result;
}

