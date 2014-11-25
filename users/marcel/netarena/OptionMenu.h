#pragma once

#include <string>

class OptionBase;

class OptionMenu
{
	struct Node
	{
		Node();

		std::string m_name;

		Node * m_parent;
		Node * m_prevSibling;
		Node * m_nextSibling;
		Node * m_firstChild;
		OptionBase * m_option;

		Node * FindChild(const std::string & name) const;
	};

	void Create();
	void Destroy(Node * node);

	Node * m_root;
	Node * m_currentNode;

public:
	enum Action
	{
		Action_NavigateUp,
		Action_NavigateDown,
		Action_NavigateSelect,
		Action_NavigateBack,
		Action_ValueIncrement,
		Action_ValueDecrement,
	};

	OptionMenu();
	~OptionMenu();

	bool HasNavParent() const;
	void HandleAction(Action action);

	void Draw(int x, int y, int sx, int sy);
};
