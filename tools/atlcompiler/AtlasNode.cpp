#include "Precompiled.h"
#include "AtlasNode.h"

AtlasNode::AtlasNode(AtlasNode* parent)
{
	m_Parent = parent;

	m_Children[0] = 0;
	m_Children[1] = 0;
	m_Children[2] = 0;
	m_Children[3] = 0;

	m_Image = 0;
	m_LeafCount = 0;
	m_UsedLeafCount = 0;
}

void AtlasNode::Setup(int d)
{
	// todo: check size instead of depth
	// min size: 32x32 (?)

	if (d == 0)
	{
		m_LeafCount = 1;

		return;
	}

	m_Children[0] = new AtlasNode(this);
	m_Children[1] = new AtlasNode(this);
	m_Children[2] = new AtlasNode(this);
	m_Children[3] = new AtlasNode(this);

	int sx = m_BB.m_Size[0] / 2;
	int sy = m_BB.m_Size[1] / 2;

	m_Children[0]->m_BB.m_Position = m_BB.m_Position;
	m_Children[1]->m_BB.m_Position = m_BB.m_Position.Add(PointI(sx, 0));
	m_Children[2]->m_BB.m_Position = m_BB.m_Position.Add(PointI(sx, sy));
	m_Children[3]->m_BB.m_Position = m_BB.m_Position.Add(PointI(0, sy));

	for (int i = 0; i < 4; ++i)
		m_Children[i]->m_BB.m_Size = PointI(sx, sy);

	for (int i = 0; i < 4; ++i)
		m_Children[i]->Setup(d - 1);

	for (int i = 0; i < 4; ++i)
		m_LeafCount += m_Children[i]->m_LeafCount;
}

void AtlasNode::UpdateDsc()
{
	if (!IsLeaf_get())
	{
		for (int i = 0; i < 4; ++i)
			m_Children[i]->UpdateDsc();
	}

	m_UsedLeafCount = m_LeafCount;

	//printf("update_dsc: %d / %d\n", m_UsedLeafCount, m_LeafCount);
}

void AtlasNode::UpdateAsc()
{
	if (!IsLeaf_get())
	{
		m_UsedLeafCount = 0;

		for (int i = 0; i < 4; ++i)
			m_UsedLeafCount += m_Children[i]->m_UsedLeafCount;
	}

	//printf("update_asc: %d / %d\n", m_UsedLeafCount, m_LeafCount);

	if (m_Parent)
		m_Parent->UpdateAsc();
}

void AtlasNode::Allocate()
{
	UpdateDsc();

	UpdateAsc();
}

AtlasNode* AtlasNode::FindSpace(PointI size)
{
	bool wouldFit = false;

	if (m_BB.m_Size[0] >= size[0] || m_BB.m_Size[1] >= size[1])
	{
		wouldFit = true;
	}

	if (!wouldFit)
		return 0;

	if (!IsLeaf_get())
	{
		//printf("recurse\n");

		for (int i = 0; i < 4; ++i)
		{
			if (m_Children[i]->m_UsedLeafCount == m_Children[i]->m_LeafCount)
			{
				//printf("skip full\n");

				continue;
			}

			AtlasNode* node = m_Children[i]->FindSpace(size);

			if (node)
			{
				return node;
			}
		}
	}

	if (wouldFit)
	{
		if (m_UsedLeafCount != 0)
		{
			//printf("space occupied\n");

			return 0;
		}
		else
		{
			//printf("found space\n");

			return this;
		}
	}

	return 0;
}

bool AtlasNode::IsLeaf_get() const
{
	return !m_Children[0];
}
