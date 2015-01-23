#pragma once

#include <string>
#include <vector>

class MultiLevelMenuBase
{
protected:
	struct Node
	{
		Node();

		std::string m_name;

		Node * m_parent;
		Node * m_prevSibling;
		Node * m_nextSibling;
		Node * m_firstChild;
		Node * m_currentSelection;
		void * m_menuItem;
		float m_timeValue; // used for 'key repeat'

		Node * FindChild(const std::string & name) const;
	};

	void Create();
	void Destroy(Node * node);

	virtual std::vector<void*> GetMenuItems() = 0;
	virtual const char * GetPath(void * menuItem) = 0;
	virtual void Select(void * menuItem) { }
	virtual void Increment(void * menuItem) { }
	virtual void Decrement(void * menuItem) { }

	Node * m_root;
	Node * m_currentNode;

	bool m_hasActionsPrevUpdate;
	bool m_hasActionsCurrUpdate;

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

	MultiLevelMenuBase();
	virtual ~MultiLevelMenuBase();

	bool HasNavParent() const;
	void HandleAction(Action action, float dt = 1.f);

	void Update();
};
