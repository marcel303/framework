#ifndef RENDERER_H
#define RENDERER_H
#pragma once

#include "Debug.h"
#include "GraphicsDevice.h"
#include "MatrixStack.h"
#include "Mesh.h"
#include "SoundDevice.h"

class Renderer : public MatrixHandler
{
public:
	static Renderer& I();

	void Initialize();
	void Shutdown();

	inline GraphicsDevice* GetGraphicsDevice()
	{
		return m_gfx;
	}

	inline SoundDevice* GetSoundDevice()
	{
		return m_sfx;
	}

	inline void SetGraphicsDevice(GraphicsDevice* gfx)
	{
		m_gfx = gfx;
	}

	inline void SetSoundDevice(SoundDevice* sfx)
	{
		m_sfx = sfx;
	}

	void RenderQuad(); // renders a quad extending from (-1, -1) to (+1, +1)
	void RenderMesh(Mesh& mesh);

	inline MatrixStack& MatW()
	{
		return m_matW;
	}

	inline MatrixStack& MatV()
	{
		return m_matV;
	}

	inline MatrixStack& MatP()
	{
		return m_matP;
	}

	inline Mat4x4 GetCubeSSMMatrix(CUBE_FACE face, const Vec3& position)
	{
		Mat4x4 t;

		t.MakeTranslation(-position);

		return m_cubeSSMMatrix[face] * t;
	}

private:
	Renderer();
	~Renderer();

	virtual void OnMatrixUpdate(MatrixStack* stack, const Mat4x4& mat);

	INITSTATE;

	GraphicsDevice* m_gfx;
	SoundDevice* m_sfx;

	MatrixStack m_matW;
	MatrixStack m_matV;
	MatrixStack m_matP;

	const static Mat4x4 m_cubeSSMMatrix[6];

	Mesh* m_quad;
};

inline Renderer* RendererPtr()
{
	return &Renderer::I();
}

#endif
