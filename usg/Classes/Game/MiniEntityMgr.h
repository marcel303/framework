#pragma once

#include "ColList.h"
#include "Entity.h"

namespace Game
{
	class MiniEntityMgr
	{
	public:
		typedef bool (*IterateBeginCB)(void* obj);
		typedef Entity* (*IterateNextCB)(void* obj);
		
		MiniEntityMgr();
		void Initialize();
		
		void Setup(void* obj, IterateBeginCB iterateBegin, IterateNextCB iterateNext);
		
		void Update(float dt);
		void UpdateSB(SelectionBuffer* sb);
		void PostUpdate();
		void Render_Below();
		void Render();
		void Render_Additive();
		
	private:
		void* m_Obj;
		IterateBeginCB m_IterateBegin;
		IterateNextCB m_IterateNext;
	};
	
	class DynEntityMgr
	{
	public:
		DynEntityMgr();
		void Initialize();
		
		void Update(float dt);
		void UpdateSB(SelectionBuffer* sb);
		void PostUpdate();
		void Render_Below();
		void Render();
		void Render_Additive();
		
		void Add(Entity* entity);
		void Clear();
		
		void ForEach(CallBack callBack);
		
		CallBack OnEntityRemove;
		
	private:
		void Remove(Col::ListNode<Entity*>* node);
					
		Col::List<Entity*> m_Entities;
		
		static bool IterateBegin(void* obj);
		static Entity* IterateNext(void* obj);
		Col::ListNode<Entity*>* m_IterationNode;
		MiniEntityMgr m_EntityMgr;
	};
	
	template <class T, int N>
	class FixedEntityMgr : public MiniEntityMgr
	{
	public:
		FixedEntityMgr() : MiniEntityMgr()
		{
			Setup(this, HandleIterateBegin, HandleIterateNext);
		}
		
		inline T& operator[](int index)
		{
			return m_Entities[index];
		}
		
		inline const T& operator[](int index) const
		{
			return m_Entities[index];
		}
		
	private:
		static bool HandleIterateBegin(void* obj)
		{
			FixedEntityMgr* self = (FixedEntityMgr*)obj;
			
			if (N <= 0)
				return false;
			
			self->m_IterationIndex = -1;
			
			return true;
		}
		
		static Entity* HandleIterateNext(void* obj)
		{
			FixedEntityMgr* self = (FixedEntityMgr*)obj;
			
			self->m_IterationIndex++;
			
			if (self->m_IterationIndex >= N)
				return 0;
			
			return &self->m_Entities[self->m_IterationIndex];
		}
		
		T m_Entities[N];
		int m_IterationIndex;
	};
}

