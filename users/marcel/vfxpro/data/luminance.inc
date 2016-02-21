struct LuminanceData
{
	float power;
	float scale;
};

layout (std140) uniform LuminanceBlock
{
	LuminanceData luminanceData;
};
