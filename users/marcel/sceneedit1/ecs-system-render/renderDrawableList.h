#pragma once

#include "renderDrawable.h"

#include "Mat4x4.h"
#include "MemAllocators2.h"

#include <algorithm> // std::sort
#include <new> // placement new

#define SORT_USING_PRIORITY_QUEUE 1

#if SORT_USING_PRIORITY_QUEUE
	#include <queue>
#endif

struct RenderDrawableAllocator : public MemAllocatorTransient
{
	RenderDrawableAllocator(const size_t size)
		: MemAllocatorTransient(size)
	{
	}
};

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
	
	template <typename C>
	void sort()
	{
		if (drawable_head != nullptr)
		{
		#if SORT_USING_PRIORITY_QUEUE
			std::priority_queue<RenderDrawable*, std::vector<RenderDrawable*>, C> queue;
			
			for (auto * drawable = drawable_head; drawable != nullptr; drawable = drawable->next)
				queue.push(drawable);
			
			drawable_head = queue.top(); queue.pop();
			drawable_tail = drawable_head;
			drawable_tail->next = nullptr;
			
			while (queue.empty() == false)
			{
				auto * drawable = queue.top(); queue.pop();
				
				drawable->next = drawable_head;
				drawable_head = drawable;
			}
		#else
			// create sortable list
			
			int numDrawables = 0;
			for (auto * drawable = drawable_head; drawable != nullptr; drawable = drawable->next)
				numDrawables++;
				
			RenderDrawable ** drawables = (RenderDrawable**)alloca(numDrawables * sizeof(RenderDrawable*));
			numDrawables = 0;
			for (auto * drawable = drawable_head; drawable != nullptr; drawable = drawable->next)
				drawables[numDrawables++] = drawable;
			
			std::stable_sort(drawables, drawables + numDrawables, C());
			
			// patch the linked list
			
			for (int i = 1; i < numDrawables; ++i)
				drawables[i - 1]->next = drawables[i];
			drawables[numDrawables - 1]->next = nullptr;
			
			// assign the new head
			
			drawable_head = drawables[0];
			drawable_tail = drawables[numDrawables - 1];
		#endif
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
	
	RenderDrawableList & viewZ(const float value)
	{
		drawable_tail->viewZ = value;
		
		return *this;
	}
};
