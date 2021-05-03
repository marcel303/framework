#pragma once

// forward declarations

class Mat4x4;

struct RenderDrawable;

//

typedef void (*RenderDrawableFunction)(const RenderDrawable & drawable);

struct RenderDrawable
{
	RenderDrawable * next = nullptr;
	
	RenderDrawableFunction function = nullptr;
	
	Mat4x4 * transform = nullptr;
	
	void * data = nullptr;
	
	float viewZ = 0.f;

	template <typename T>
	T & getData() const
	{
		return (T&)data;
	}
};

struct RenderDrawableCompare_IncreasingViewZ
{
	bool operator()(const RenderDrawable * a, const RenderDrawable * b) const
	{
		return a->viewZ < b->viewZ;
	}
};

struct RenderDrawableCompare_DecreasingViewZ
{
	bool operator()(const RenderDrawable * a, const RenderDrawable * b) const
	{
		return a->viewZ > b->viewZ;
	}
};
