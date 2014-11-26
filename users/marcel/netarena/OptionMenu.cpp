#include "framework.h"
#include "OptionMenu.h"
#include "Options.h"

OptionMenu::Node::Node()
	: m_parent(0)
	, m_prevSibling(0)
	, m_nextSibling(0)
	, m_firstChild(0)
	, m_currentSelection(0)
	, m_option(0)
	, m_timeValue(0.f)
{
}

OptionMenu::Node * OptionMenu::Node::FindChild(const std::string & name) const
{
	for (Node * node = m_firstChild; node != 0; node = node->m_nextSibling)
	{
		if (node->m_name == name)
			return node;
	}

	return 0;
}

//

void OptionMenu::Create()
{
	Destroy(m_root);
	m_root = new Node();

	for (OptionBase * option = g_optionManager.m_head; option != 0; option = option->GetNext())
	{
		Node * node = m_root;

		std::string path = option->GetPath();

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
				leaf->m_option = option;

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
}

void OptionMenu::Destroy(Node * node)
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

OptionMenu::OptionMenu()
	: m_root(0)
	, m_currentNode(0)
	, m_hasActionsPrevUpdate(false)
	, m_hasActionsCurrUpdate(false)
{
	Create();

	m_currentNode = m_root;
}

OptionMenu::~OptionMenu()
{
	m_currentNode = 0;

	Destroy(m_root);
	m_root = 0;
}

bool OptionMenu::HasNavParent() const
{
	return (m_currentNode->m_parent != 0);
}

void OptionMenu::HandleAction(Action action, float dt)
{
	Node *& currentSelection = m_currentNode->m_currentSelection;

	if (currentSelection == 0)
		return; // empty tree

	// make sure the action triggers if it isn't a repeat
	if (!m_hasActionsPrevUpdate && !m_hasActionsCurrUpdate)
		dt = 1000.f;

	const float kMoveSpeed = 6.f;
	const float kChangeSpeed = 6.f;

	switch (action)
	{
	case Action_NavigateUp:
		currentSelection->m_timeValue += dt * kMoveSpeed;
		if (currentSelection->m_timeValue >= 1.f)
		{
			currentSelection->m_timeValue = 0.f;
			if (currentSelection->m_prevSibling)
				currentSelection = currentSelection->m_prevSibling;
		}
		break;
	case Action_NavigateDown:
		currentSelection->m_timeValue += dt * kMoveSpeed;
		if (currentSelection->m_timeValue >= 1.f)
		{
			currentSelection->m_timeValue = 0.f;
			if (currentSelection->m_nextSibling)
				currentSelection = currentSelection->m_nextSibling;
		}
		break;
	case Action_NavigateSelect:
		if (currentSelection->m_option == 0)
			m_currentNode = currentSelection;
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
			if (currentSelection->m_option)
				currentSelection->m_option->Increment();
		}
		break;
	case Action_ValueDecrement:
		currentSelection->m_timeValue += dt * kChangeSpeed;
		if (currentSelection->m_timeValue >= 1.f)
		{
			currentSelection->m_timeValue = 0.f;
			if (currentSelection->m_option)
				currentSelection->m_option->Decrement();
		}
		break;
	}

	if (!m_hasActionsPrevUpdate && !m_hasActionsCurrUpdate)
	{
		if (m_currentNode->m_currentSelection)
			m_currentNode->m_currentSelection->m_timeValue = 0.f;
	}

	m_hasActionsCurrUpdate = true;
}

void OptionMenu::Update()
{
	m_hasActionsPrevUpdate = m_hasActionsCurrUpdate;
	m_hasActionsCurrUpdate = false;
}

void OptionMenu::Draw(int x, int y, int sx, int sy)
{
	if (m_currentNode->m_currentSelection == 0)
		return; // empty tree

	const int fontSize = 30;
	const int lineSize = 40;

	int numNodes = 0;

	for (Node * node = m_currentNode->m_firstChild; node != 0; node = node->m_nextSibling)
		numNodes++;

	setColor(0, 0, 0, 127);
	drawRect(x, y, x + sx, y + numNodes * lineSize);

	Font font("calibri.ttf");
	setFont(font);

	int index = 0;
	for (Node * node = m_currentNode->m_firstChild; node != 0; node = node->m_nextSibling, index++)
	{
		if (node == m_currentNode->m_currentSelection)
		{
			setColor(127, 227, 255, 127);
			drawRect(x, y + index * lineSize, x + sx, y + (index + 1) * lineSize);
			setColor(0, 0, 0);
		}
		else
			setColor(127, 227, 255);
		drawText(x + 2, y + index * lineSize, fontSize, +1.f, +1.f, node->m_name.c_str());
		if (node->m_option)
		{
			const int bufferSize = 64;
			char buffer[bufferSize];
			node->m_option->ToString(buffer, bufferSize);
			drawText(x + sx - 1 - 2, y + index * lineSize, fontSize, -1.f, +1.f, buffer);
		}
	}

	setColor(colorWhite);
}
