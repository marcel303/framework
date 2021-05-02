#pragma once

// todo : move to separate file ?

#include "MemAllocators2.h"

typedef MemAllocatorTransient RenderDrawableAllocator;

//

#include "renderDrawable.h"

#include "Mat4x4.h"

#include <algorithm> // std::sort
#include <new> // placement new

struct RenderDrawableList
{
	RenderDrawableAllocator * allocator = nullptr;
	
	RenderDrawable * drawable_head = nullptr;
	RenderDrawable * drawable_tail = nullptr;
	
	Mat4x4 worldToView;
	
	void * allocTransient(int size)
	{
		return allocator->Alloc(size);
	}
	
	template <typename T>
	T * allocTransient()
	{
		T * result = (T*)allocTransient(sizeof(T));
		return result;
	}
	
	template <typename T>
	T * newTransient()
	{
		void * mem = allocTransient<T>();
		T * result = new (mem) T();
		return result;
	}
	
	template <typename T>
	T * copyTransient(const T & value)
	{
		T * result = allocTransient<T>();
		
		*result = value;
		
		return result;
	}
	
	void captureBegin(const Mat4x4 & in_worldToView, RenderDrawableAllocator * in_allocator)
	{
		worldToView = in_worldToView;
		
		allocator = in_allocator;
		
		drawable_head = newTransient<RenderDrawable>();
		drawable_head->function = [](const RenderDrawable & drawable) { };
		drawable_tail = drawable_head;
	}
	
	void captureEnd()
	{
		drawable_tail->next = nullptr;
	}
	
	void sortByViewZ()
	{
		if (drawable_head != nullptr)
		{
			int numDrawables = 0;
			for (auto * drawable = drawable_head; drawable != nullptr; drawable = drawable->next)
				numDrawables++;
				
			RenderDrawable ** drawables = (RenderDrawable**)alloca(numDrawables * sizeof(RenderDrawable*));
			numDrawables = 0;
			for (auto * drawable = drawable_head; drawable != nullptr; drawable = drawable->next)
				drawables[numDrawables++] = drawable;
			
			std::sort(drawables, drawables + numDrawables, [](const RenderDrawable * a, const RenderDrawable * b)
			{
				// todo : sort order depends on pass. should specify as function argument
				return a->viewZ < b->viewZ;
			});
			
			// patch the linked list
			for (int i = 1; i < numDrawables; ++i)
				drawables[i - 1]->next = drawables[i];
			drawables[numDrawables - 1]->next = nullptr;
			// assign the new head
			drawable_head = drawables[0];
		}
	}
	
	void execute() const
	{
		for (auto * drawable = drawable_head; drawable != nullptr; drawable = drawable->next)
		{
			drawable->function(*drawable);
		}
	}
	
	RenderDrawableList & add(const RenderDrawableFunction & function)
	{
		auto * drawable = newTransient<RenderDrawable>();
	
		drawable->function = function;
		
		drawable_tail->next = drawable;
		drawable_tail = drawable;
		
		return *this;
	}
	
	RenderDrawableList & data(void * data)
	{
		drawable_tail->data = data;
		
		return *this;
	}

	RenderDrawableList & transform(const Mat4x4 & transform)
	{
		drawable_tail->transform = copyTransient(transform);
		
		drawable_tail->viewZ = worldToView.Mul4(transform.GetTranslation())[2];
		
		return *this;
	}

	RenderDrawableList & worldPosition(Vec3Arg position)
	{
		drawable_tail->viewZ = worldToView.Mul4(position)[2];
		
		return *this;
	}
};
