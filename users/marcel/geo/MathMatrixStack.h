#pragma once

#include "Mat4x4.h"
#include "Vec3.h"
#include <vector>

class MatrixStack
{

public:
	
	MatrixStack();
	~MatrixStack();
	
private:
	
	std::vector<Mat4x4> vMatrix;
	
public:
	
	void Push();
	void Pop();
	
public:
	
	Mat4x4& GetMatrix();
	const Mat4x4& GetMatrix() const;
	
public:
	
	void ApplyTranslation(Vec3Arg translation);
	void ApplyRotationAngleAxis(float angle, Vec3Arg axis);
	void ApplyRotationEuler(Vec3Arg rotation);
	void ApplyRotationX(float angle);
	void ApplyRotationY(float angle);
	void ApplyRotationZ(float angle);
	void ApplyScaling(Vec3Arg scale);
	
};
