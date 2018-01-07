#include "MultiLevelMenu.h"
#include <algorithm>
#include <string.h>

MultiLevelMenuBase::Node::Node()
	: m_parent(0)
	, m_prevSibling(0)
	, m_nextSibling(0)
	, m_firstChild(0)
	, m_currentSelection(0)
	, m_menuItem(0)
	, m_timeValue(0.f)
{
}

MultiLevelMenuBase::Node * MultiLevelMenuBase::Node::FindChild(const std::string & name) const
{
	for (Node * node = m_firstChild; node != 0; node = node->m_nextSibling)
	{
		if (node->m_name == name)
			return node;
	}

	return 0;
}

//

void MultiLevelMenuBase::Create()
{
	Destroy(m_root);
	m_root = new Node();

	struct Sortable
	{
		MultiLevelMenuBase * menu;
		void * menuItem;

		static bool IsLeaf(const char * str)
		{
			while (*str)
				if (*str++ == '/')
					return false;
			return true;
		}

		bool operator<(const Sortable & other) const
		{
			const char * path1 = menu->GetPath(menuItem);
			const char * path2 = menu->GetPath(other.menuItem);
			while (*path1 == *path2 && *path1)
			{
				path1++;
				path2++;
			}
			const bool isLeaf1 = IsLeaf(path1);
			const bool isLeaf2 = IsLeaf(path2);
			if (isLeaf1 != isLeaf2)
				return isLeaf1 > isLeaf2;
			return strcmp(path1, path2) > 0;
		}
	};

	std::vector<Sortable> sortables;

	std::vector<void*> menuItems = GetMenuItems();

	for (auto i = menuItems.begin(); i  != menuItems.end(); ++i)
	{
		Sortable sortable;
		sortable.menu = this;
		sortable.menuItem = *i;
		sortables.push_back(sortable);
	}

	std::sort(sortables.begin(), sortables.end());

	for (auto i = sortables.begin(); i != sortables.end(); ++i)
	{
		void * menuItem = i->menuItem;

		Node * node = m_root;

		std::string path = GetPath(menuItem);

		for (;;)
		{
			size_t pos = path.find('/');

			if (pos != path.npos)
			{
				const std::string childName = path.substr(0, pos);

				Node * child = node->FindChild(childName);

				if (child == 0)
				{
					child = new Node();

					child->m_name = childName;
					child->m_parent = node;

					if (node->m_firstChild)
					{
						node->m_firstChild->m_prevSibling = child;
						child->m_nextSibling = node->m_firstChild;
					}
					node->m_firstChild = child;
					node->m_currentSelection = child;
				}

				node = child;

				path = path.substr(pos + 1);
			}
			else
			{
				Node * leaf = new Node();

				leaf->m_name = path;
				leaf->m_parent = node;
				leaf->m_menuItem = menuItem;

				if (node->m_firstChild)
				{
					node->m_firstChild->m_prevSibling = leaf;
					leaf->m_nextSibling = node->m_firstChild;
				}
				node->m_firstChild = leaf;
				node->m_currentSelection = leaf;

				break;
			}
		}
	}

	//

	m_currentNode = m_root;
}

void MultiLevelMenuBase::Destroy(Node * node)
{
	if (!node)
		return;
	Node * child = node->m_firstChild;
	while (child != 0)
	{
		Node * nextSibling = child->m_nextSibling;
		Destroy(child);
		child = nextSibling;
	}
	delete node;
}

//

MultiLevelMenuBase::MultiLevelMenuBase()
	: m_root(0)
	, m_currentNode(0)
	, m_hasActionsPrevUpdate(false)
	, m_hasActionsCurrUpdate(false)
{
}

MultiLevelMenuBase::~MultiLevelMenuBase()
{
	m_currentNode = 0;

	Destroy(m_root);
	m_root = 0;
}

bool MultiLevelMenuBase::HasNavParent() const
{
	return (m_currentNode->m_parent != 0);
}

void MultiLevelMenuBase::HandleAction(Action action, float dt)
{
	Node *& currentSelection = m_currentNode->m_currentSelection;

	if (currentSelection == 0)
		return; // empty tree

	// make sure the action triggers if it isn't a repeat
	if (!m_hasActionsPrevUpdate && !m_hasActionsCurrUpdate)
		dt = 1000.f;

	const float kMoveSpeed = 6.f;
	const float kChangeSpeed = 60.f / 4.f;

	switch (action)
	{
	case Action_NavigateUp:
		currentSelection->m_timeValue += dt * kMoveSpeed;
		if (currentSelection->m_timeValue >= 1.f)
		{
			currentSelection->m_timeValue = 0.f;
			if (currentSelection->m_prevSibling)
				currentSelection = currentSelection->m_prevSibling;
			else
			{
				while (currentSelection->m_nextSibling)
					currentSelection = currentSelection->m_nextSibling;
			}
		}
		break;
	case Action_NavigateDown:
		currentSelection->m_timeValue += dt * kMoveSpeed;
		if (currentSelection->m_timeValue >= 1.f)
		{
			currentSelection->m_timeValue = 0.f;
			if (currentSelection->m_nextSibling)
				currentSelection = currentSelection->m_nextSibling;
			else
			{
				while (currentSelection->m_prevSibling)
					currentSelection = currentSelection->m_prevSibling;
			}
		}
		break;
	case Action_NavigateSelect:
		if (currentSelection->m_menuItem == 0)
			m_currentNode = currentSelection;
		else
			Select(currentSelection->m_menuItem);
		break;
	case Action_NavigateBack:
		if (m_currentNode->m_parent)
			m_currentNode = m_currentNode->m_parent;
		break;
	case Action_ValueIncrement:
		currentSelection->m_timeValue += dt * kChangeSpeed;
		if (currentSelection->m_timeValue >= 1.f)
		{
			currentSelection->m_timeValue = 0.f;
			if (currentSelection->m_menuItem)
				Increment(currentSelection->m_menuItem);
		}
		break;
	case Action_ValueDecrement:
		currentSelection->m_timeValue += dt * kChangeSpeed;
		if (currentSelection->m_timeValue >= 1.f)
		{
			currentSelection->m_timeValue = 0.f;
			if (currentSelection->m_menuItem)
				Decrement(currentSelection->m_menuItem);
		}
		break;
	}

	if (!m_hasActionsPrevUpdate && !m_hasActionsCurrUpdate)
	{
		if (m_currentNode->m_currentSelection)
			m_currentNode->m_currentSelection->m_timeValue = -1.f;
	}

	m_hasActionsCurrUpdate = true;
}

void MultiLevelMenuBase::Update()
{
	m_hasActionsPrevUpdate = m_hasActionsCurrUpdate;
	m_hasActionsCurrUpdate = false;
}
