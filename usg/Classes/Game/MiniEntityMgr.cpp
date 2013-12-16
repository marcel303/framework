#include "MiniEntityMgr.h"

namespace Game
{
	MiniEntityMgr::MiniEntityMgr()
	{
		Initialize();
	}
	
	void MiniEntityMgr::Initialize()
	{
		m_Obj = 0;
		m_IterateBegin = 0;
		m_IterateNext = 0;
	}
	
	void MiniEntityMgr::Setup(void* obj, IterateBeginCB iterateBegin, IterateNextCB iterateNext)
	{
		m_Obj = obj;
		m_IterateBegin = iterateBegin;
		m_IterateNext = iterateNext;
	}
	
	void MiniEntityMgr::Update(float dt)
	{
		if (!m_IterateBegin(m_Obj))
			return;
		
		Entity* entity = 0;
		
		while ((entity = m_IterateNext(m_Obj)))
		{
			if (!entity->IsAlive_get())
				continue;
		
			entity->Update(dt);
		}
	}
	
	void MiniEntityMgr::UpdateSB(SelectionBuffer* sb)
	{
		if (!m_IterateBegin(m_Obj))
			return;
		
		Entity* entity = 0;
		
		while ((entity = m_IterateNext(m_Obj)))
		{
			if (!entity->IsAlive_get())
				continue;
			
			entity->UpdateSB(sb);
		}
	}
	
	void MiniEntityMgr::PostUpdate()
	{
		if (!m_IterateBegin(m_Obj))
			return;
		
		Entity* entity = 0;
		
		while ((entity = m_IterateNext(m_Obj)))
		{
			if (!entity->IsAlive_get())
				continue;
			
			if (entity->Flag_IsSet(EntityFlag_DidDie))
			{
				entity->IsAlive_set(XFALSE);
				
				entity->HandleDie();
			}
		}
	}
	
	void MiniEntityMgr::Render_Below()
	{
		if (!m_IterateBegin(m_Obj))
			return;
		
		Entity* entity = 0;
		
		while ((entity = m_IterateNext(m_Obj)))
		{
			if (!entity->IsAlive_get())
				continue;
			
			entity->Render_Below();
		}
	}
	
	void MiniEntityMgr::Render()
	{
		if (!m_IterateBegin(m_Obj))
			return;
		
		Entity* entity = 0;
		
		while ((entity = m_IterateNext(m_Obj)))
		{
			if (!entity->IsAlive_get())
				continue;
			
			entity->Render();
		}
	}
	
	void MiniEntityMgr::Render_Additive()
	{
		if (!m_IterateBegin(m_Obj))
			return;
		
		Entity* entity = 0;
		
		while ((entity = m_IterateNext(m_Obj)))
		{
			if (!entity->IsAlive_get())
				continue;
			if (!entity->Flag_IsSet(EntityFlag_RenderAdditive))
				continue;
			
			entity->Render_Additive();
		}
	}
	
	//
	
	DynEntityMgr::DynEntityMgr()
	{
		Initialize();
	}
	
	void DynEntityMgr::Initialize()
	{
		m_EntityMgr.Setup(this, IterateBegin, IterateNext);

		m_IterationNode = 0;
	}
	
	void DynEntityMgr::Update(float dt)
	{
		m_EntityMgr.Update(dt);
	}

	void DynEntityMgr::UpdateSB(SelectionBuffer* sb)
	{
		m_EntityMgr.UpdateSB(sb);
	}
	
	void DynEntityMgr::PostUpdate()
	{
		m_EntityMgr.PostUpdate();
		
		for (Col::ListNode<Entity*>* node = m_Entities.m_Head; node;)
		{
			Col::ListNode<Entity*>* next = node->m_Next;
			
			if (!node->m_Object->IsAlive_get())
			{
				Remove(node);
			}
			
			node = next;
		}
	}
	
	void DynEntityMgr::Render_Below()
	{
		m_EntityMgr.Render_Below();
	}
	
	void DynEntityMgr::Render()
	{
		m_EntityMgr.Render();
	}
	
	void DynEntityMgr::Render_Additive()
	{
		m_EntityMgr.Render_Additive();
	}
	
	void DynEntityMgr::Add(Entity* entity)
	{
		Assert(entity != 0);
		
		for (Col::ListNode<Entity*>* node = m_Entities.m_Head; node; node = node->m_Next)
		{
			if (node->m_Object->Layer_get() < entity->Layer_get())
				continue;
			
			m_Entities.AddAfter(node, entity);
			
			return;
		}
		
		m_Entities.AddTail(entity);
	}
	
	void DynEntityMgr::Clear()
	{
		// todo: proper die sequence?

		for (Col::ListNode<Entity*>* node = m_Entities.m_Head; node; node = node->m_Next)
		{
			node->m_Object->Flag_Set(EntityFlag_DidDie);
		}
		
		PostUpdate();
	}
	
	void DynEntityMgr::Remove(Col::ListNode<Entity*>* node)
	{
		if (OnEntityRemove.IsSet())
			OnEntityRemove.Invoke(node->m_Object);
		
		delete node->m_Object;
		node->m_Object = 0;
		
		m_Entities.Remove(node);
	}

	void DynEntityMgr::ForEach(CallBack callBack)
	{
		for (Col::ListNode<Entity*>* node = m_Entities.m_Head; node; node = node->m_Next)
		{
			Entity* entity = node->m_Object;
//		if (IterateBegin(this))
//		{
//			Entity* entity;
			
//			while ((entity = IterateNext(this)))
//			{
				// skip transient entities in for each queries
				
				if (entity->Flag_IsSet(EntityFlag_Transient))
					continue;
				
				callBack.Invoke(entity);
//			}
		}
	}
	
	bool DynEntityMgr::IterateBegin(void* obj)
	{
		DynEntityMgr* self = (DynEntityMgr*)obj;
		
		self->m_IterationNode = 0;
		
		return self->m_Entities.m_Head != 0;
	}
	
	Entity* DynEntityMgr::IterateNext(void* obj)
	{
		DynEntityMgr* self = (DynEntityMgr*)obj;
		
		if (!self->m_IterationNode)
			self->m_IterationNode = self->m_Entities.m_Head;
		else
			self->m_IterationNode = self->m_IterationNode->m_Next;
		
		if (!self->m_IterationNode)
			return 0;
		
		return self->m_IterationNode->m_Object;
	}
};
