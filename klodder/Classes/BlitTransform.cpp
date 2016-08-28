#include "BlitTransform.h"
#include "Mat3x2.h"

BlitTransform::BlitTransform()
{
	anchorX = 0.0f;
	anchorY = 0.0f;
	angle = 0.0f;
	scale = 1.0f;
	x = 0.0f;
	y = 0.0f;
}

void BlitTransform::ToMatrix(Mat3x2 & out_Matrix) const
{
	Mat3x2 matT1;
	Mat3x2 matR;
	Mat3x2 matS;
	Mat3x2 matT2;
	
	matT1.MakeTranslation(Vec2F(x, y));
	matR.MakeRotation(angle);
	matS.MakeScaling(scale, scale);
	matT2.MakeTranslation(Vec2F(-anchorX, -anchorY));
	
	out_Matrix = matT1 * matR * matS * matT2;
}
