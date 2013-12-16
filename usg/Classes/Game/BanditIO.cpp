#include "Bandit.h"
#include "BanditIO.h"
#include "GameState.h"
#include "Path.h"
#include "StringEx.h"

namespace Bandits
{
	Bandit* BanditReader::Read(EntityBandit* entity, VRCC::CompiledComposition* composition, int level)
	{
		Bandit* bandit = new Bandit();
		
		// Allocate links
		
		Link** links = new Link*[composition->m_LinkCount];
		
		for (int i = 0; i < composition->m_LinkCount; ++i)
		{
			links[i] = new Link();
		}
		
		// Read links
		
		for (int i = 0; i < composition->m_LinkCount; ++i)
		{
			VRCC::Composition_Link& ccLink = composition->m_Links[i];
			
			Link* parent = 0;
			if (ccLink.m_ParentIndex >= 0)
				parent = links[ccLink.m_ParentIndex];
			Vec2F position = ccLink.m_Location;
			VectorShape* shape = 0;
			
			if (ccLink.m_ShapeName != String::Empty)
			{
				shape = (VectorShape*)g_GameState->m_ResMgr.Find(Path::ReplaceExtension(ccLink.m_ShapeName, "vgc").c_str(), "")->data;
				Assert(shape != 0);
			}
			
			int flags = ccLink.m_Flags;
			
			links[i]->Setup(
				bandit,
				parent,
				position,
				composition->m_Links[i].m_AngleBase,
				composition->m_Links[i].m_Angle,
				composition->m_Links[i].m_AngleMin,
				composition->m_Links[i].m_AngleMax,
				composition->m_Links[i].m_AngleSpeed,
				shape, (flags & VRCC::CompositionFlag_Draw_MirrorX) != 0, (flags & VRCC::CompositionFlag_Draw_MirrorY) != 0, flags);
			
			bool alive = level >= ccLink.m_LevelMin && level <= ccLink.m_LevelMax;
			
			links[i]->IsAlive_set(alive);
		}
		
		// Fix child lists
		
		for (int i = 0; i < composition->m_LinkCount; ++i)
		{
			VRCC::Composition_Link& ccLink = composition->m_Links[i];
			
#if 1
			links[i]->ChildCount_set(ccLink.m_ChildIndices.Count_get());
			
			int j = 0;
			
			for (Col::ListNode<int>* node = ccLink.m_ChildIndices.m_Head; node; node = node->m_Next)
			{
				links[i]->Child_set(j, links[node->m_Object]);
				
				++j;
			}
#else
			links[i]->ChildCount_set((int)ccLink.m_ChildIndices.size());
			
			for (size_t j = 0; j < ccLink.m_ChildIndices.size(); ++j)
			{
				links[i]->Child_set(j, links[ccLink.m_ChildIndices[j]]);
			}
#endif
		}
		
		bandit->Setup(entity, links, composition->m_LinkCount, links[composition->m_RootIndex]);
		
		bandit->Finalize();
		
		return bandit;
	}
}
