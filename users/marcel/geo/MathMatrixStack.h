#ifndef __MathMatrixStack_h__
#define __MathMatrixStack_h__

#include <vector>
#include "MathMatrix.h"

class MatrixStack
{

	public:
	
	MatrixStack();
	~MatrixStack();
	
	private:
	
	std::vector<Matrix> vMatrix;
	
	public:
	
	void Push();
	void Pop();
	
	public:
	
	Matrix& GetMatrix();
	const Matrix& GetMatrix() const;
	
	public:
	
	void ApplyTranslation(Vector translation);
	void ApplyRotationEuler(Vector rotation);
	void ApplyRotationX(float angle);
	void ApplyRotationY(float angle);
	void ApplyRotationZ(float angle);
	void ApplyScaling(Vector scale);
	
};

#endif // !__MathMatrixStack_h__