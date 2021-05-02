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
	
	float viewZ = 0.f;
	
	void * data = nullptr;

	template <typename T>
	T & getData() const
	{
		return (T&)data;
	}
};
