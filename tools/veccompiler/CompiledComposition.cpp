#include "CompiledComposition.h"
#include "Exception.h"
#include "StreamReader.h"
#include "StreamWriter.h"

#define USE_LIST 1

namespace VRCC
{
	Composition_Link::Composition_Link()
	{
		m_Index = -1;
		m_ParentIndex = -1;
		
		m_AngleBase = 0.0f;
		m_Angle = 0.0f;
		m_AngleMin = 0.0f;
		m_AngleMax = 0.0f;
		m_AngleSpeed = 0.0f;
		m_LevelMin = 0;
		m_LevelMax = 1000;
		m_Flags = 0;
	}
	
	CompiledComposition::CompiledComposition()
	{
		m_RootIndex = 0;
		
		m_Links = 0;
		m_LinkCount = 0;
	}
	
	CompiledComposition::~CompiledComposition()
	{
		Allocate(0);
	}

	void CompiledComposition::Load(Stream* stream)
	{
		StreamReader reader(stream, false);
		
		m_RootIndex = reader.ReadInt32();
		
		int linkCount = reader.ReadInt32();
		
		Allocate(linkCount);
		
		for (int i = 0; i < linkCount; ++i)
		{
			Composition_Link& link = m_Links[i];
			
			link.m_Index = reader.ReadInt32();
			link.m_ParentIndex = reader.ReadInt32();
			int childCount = reader.ReadInt32();
			for (int j = 0; j < childCount; ++j)
#if USE_LIST
				link.m_ChildIndices.AddTail(reader.ReadInt32());
#else
				link.m_ChildIndices.push_back(reader.ReadInt32());
#endif
			link.m_Location[0] = reader.ReadFloat();
			link.m_Location[1] = reader.ReadFloat();
			link.m_AngleBase = reader.ReadFloat();
			link.m_Angle = reader.ReadFloat();
			link.m_AngleMin = reader.ReadFloat();
			link.m_AngleMax = reader.ReadFloat();
			link.m_AngleSpeed = reader.ReadFloat();
			link.m_LevelMin = reader.ReadInt32();
			link.m_LevelMax = reader.ReadInt32();
			link.m_Flags  = reader.ReadInt32();
			link.m_ShapeName = reader.ReadText_Binary();
		}
	}
	
	void CompiledComposition::Save(Stream* stream)
	{
		StreamWriter writer(stream, false);
		
		writer.WriteInt32(m_RootIndex);
		
		writer.WriteInt32(m_LinkCount);
		
		for (int i = 0; i < m_LinkCount; ++i)
		{
			const Composition_Link& link = m_Links[i];
			
			writer.WriteInt32(link.m_Index);
			writer.WriteInt32(link.m_ParentIndex);
			
#if USE_LIST
			writer.WriteInt32(link.m_ChildIndices.Count_get());
			for (Col::ListNode<int>* node = link.m_ChildIndices.m_Head; node; node = node->m_Next)
				writer.WriteInt32(node->m_Object);
#else
			writer.WriteInt32((int)link.m_ChildIndices.size());
			for (size_t j = 0; j < link.m_ChildIndices.size(); ++j)
				writer.WriteInt32(link.m_ChildIndices[j]);
#endif
			writer.WriteFloat(link.m_Location[0]);
			writer.WriteFloat(link.m_Location[1]);
			writer.WriteFloat(link.m_AngleBase);
			writer.WriteFloat(link.m_Angle);
			writer.WriteFloat(link.m_AngleMin);
			writer.WriteFloat(link.m_AngleMax);
			writer.WriteFloat(link.m_AngleSpeed);
			writer.WriteInt32(link.m_LevelMin);
			writer.WriteInt32(link.m_LevelMax);
			writer.WriteInt32(link.m_Flags);
			writer.WriteText_Binary(link.m_ShapeName);
			
			if (0)
			printf("save: link: idx=%d, pidx=%d, chldcnt=%d, shape=%s\n",
				link.m_Index,
				link.m_ParentIndex,
#if USE_LIST
				link.m_ChildIndices.Count_get(),
#else
				(int)link.m_ChildIndices.size(),
#endif
				link.m_ShapeName.c_str());
		}
	}
	
	void CompiledComposition::Allocate(int linkCount)
	{
		delete[] m_Links;
		m_Links = 0;
		m_LinkCount = 0;
		
		if (linkCount > 0)
		{
			m_Links = new Composition_Link[linkCount];
			m_LinkCount = linkCount;
		}
	}
}
