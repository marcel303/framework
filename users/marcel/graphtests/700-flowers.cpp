#include "framework.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

struct Socket
{
	std::string name;
};

struct Node
{
	std::vector<std::shared_ptr<Socket>> inputs;
	std::vector<std::shared_ptr<Socket>> outputs;
	
	Vec2 position;
};

struct Link
{
	std::weak_ptr<Socket> src;
	std::weak_ptr<Socket> dst;
};

struct Graph
{
	std::vector<std::shared_ptr<Node>> nodes;
	std::vector<std::shared_ptr<Link>> links;
};

struct GraphEditorInterface
{
	virtual bool isSelected(const Node * node) const = 0;
	virtual bool isHovered(const Node * node) const = 0;
	
	virtual bool isSelected(const Socket * socket) const = 0;
	virtual bool isHovered(const Socket * socket) const = 0;
};

struct GraphHittestResult
{
	std::weak_ptr<Node> node;
	std::weak_ptr<Socket> socket;
};

template <class T>
bool operator<(const std::weak_ptr<T> & a, const std::weak_ptr<T> & b)
{
    return a.lock().get() < b.lock().get();
}

struct GraphVisualizer
{
	float kNodeRadius = 40.f;
	Color kColor_NodeIdle = Color(40, 40, 40);
	Color kColor_NodeHovered = Color(60, 60, 60);
	Color kColor_NodeSelected = Color(60, 60, 100);
	Color kColor_SocketIdle = Color(200, 200, 200);
	Color kColor_SocketHovered = Color(255, 255, 200);
	Color kColor_SocketSelected = Color(200, 200, 255);
	float kSocketRadius = 6.f;
	float kSocketForceBegin = 4.f;
	
	struct SocketState
	{
		bool valid = false;
		Vec2 position;
		Vec2 force;
		Vec2 speed;
	};
	
	std::shared_ptr<Graph> graph;
	
	std::map<std::weak_ptr<Node>, int> nodes;
	std::map<std::weak_ptr<Socket>, SocketState> sockets;
	
	GraphHittestResult hittest(Vec2Arg position, std::weak_ptr<Socket> socketToExclude) const
	{
		GraphHittestResult result;
		
		for (auto & socket_state_itr : sockets)
		{
			if (socket_state_itr.first.lock().get() == socketToExclude.lock().get())
				continue;
			
			auto & socket_state = socket_state_itr.second;
			
			const Vec2 delta = socket_state.position - position;
			
			if (delta.CalcSize() <= kSocketRadius)
			{
				result.socket = socket_state_itr.first;
				return result;
			}
		}
		
		for (auto & node_itr : graph->nodes)
		{
			auto * node = node_itr.get();
			
			Vec2 delta = node->position - position;
			
			if (delta.CalcSize() <= kNodeRadius)
			{
				result.node = node_itr;
				return result;
			}
		}
		
		return result;
	}
	
	void tick(const float dt)
	{
		if (graph.get() == nullptr)
		{
			// todo : clean up state
			return;
		}
		
		// erase expired node and socket states
		
		for (auto node_itr = nodes.begin(); node_itr != nodes.end(); )
		{
			auto & node = node_itr->first;
			
			if (node.expired())
				node_itr = nodes.erase(node_itr);
			else
			{
				++node_itr;
			}
		}
		
		for (auto socket_itr = sockets.begin(); socket_itr != sockets.end(); )
		{
			auto & socket = socket_itr->first;
			
			if (socket.expired())
				socket_itr = sockets.erase(socket_itr);
			else
			{
				++socket_itr;
			}
		}
		
		// ensure node and socket states exist
		
		for (auto & node : graph->nodes)
		{
			for (auto & socket_itr : node->inputs)
			{
				auto & socket_state = sockets[socket_itr];
				
				if (!socket_state.valid)
				{
					socket_state.position = node->position;
					socket_state.valid = true;
				}
			}
		}
		
		// physics: forces: sockets pushing away
		
	#if true
		for (auto & node : graph->nodes)
		{
			for (auto & socket_1 : node->inputs)
			{
			for (auto & socket_2 : node->inputs)
			{
				if (socket_1.get() == socket_2.get())
					continue;
				
				auto socket_itr_1 = sockets.find(socket_1);
				auto socket_itr_2 = sockets.find(socket_2);
				
				auto & socket_state_1 = socket_itr_1->second;
				auto & socket_state_2 = socket_itr_2->second;
	#else
		for (auto & socket_itr_1 : sockets)
		{
			for (auto & socket_itr_2 : sockets)
			{
				if (socket_itr_2.first.lock().get() == socket_itr_1.first.lock().get())
					continue;
	
				auto & socket_state_1 = socket_itr_1.second;
				auto & socket_state_2 = socket_itr_2.second;
	#endif
				const Vec2 delta = socket_state_2.position - socket_state_1.position;
				const float distance = delta.CalcSize();
				
				if (distance <= kSocketForceBegin)
				{
					socket_state_1.force += Vec2(random<float>(-1.f, +1.f), random<float>(-1.f, +1.f)) * 1000.f;
				}
				else
				{
					socket_state_1.force -= delta / (distance * distance) * 10000.f;
				}
			}
		}
		}
		
		// physics: forces: sockets stay near node
		
		for (auto & node : graph->nodes)
		{
			for (auto & socket_itr : node->inputs)
			{
				auto & socket_state = sockets[socket_itr];
				
				const Vec2 delta_1 = node->position - socket_state.position;
				if (delta_1.CalcSize() <= 1.f)
					continue;
				
				const Vec2 ring_position = node->position - delta_1.CalcNormalized() * kNodeRadius;
				const Vec2 delta = ring_position - socket_state.position;
				const float distance = delta.CalcSize();
				
				socket_state.force += delta.CalcNormalized() * distance * 100.f;
			}
		}
		
		// physics: forces: links pull sockets
		
		for (auto & link : graph->links)
		{
			auto socket_itr_1 = sockets.find(link->src);
			auto socket_itr_2 = sockets.find(link->dst);
			
			auto & socket_state_1 = socket_itr_1->second;
			auto & socket_state_2 = socket_itr_2->second;

			const Vec2 delta = socket_state_2.position - socket_state_1.position;
			const float distance = delta.CalcSize();
		
			socket_state_1.force += delta * distance * .1f;
			socket_state_2.force -= delta * distance * .1f;
		}
		
		// physics: integrate forces and speeds
		
		for (auto & socket_itr : sockets)
		{
			auto & socket = socket_itr.second;
			
			socket.speed += socket.force * dt;
			socket.position += socket.speed * dt;
			
			socket.force.SetZero();
			socket.speed *= powf(.5f, dt * 10.f); // todo : add constant
		}
	}
	
	void draw(const GraphEditorInterface & graphEditor) const
	{
		if (graph.get() == nullptr)
			return;
		
		hqBegin(HQ_LINES);
		for (auto & link_itr : graph->links)
		{
			auto * link = link_itr.get();
			
			auto socket_itr_1 = sockets.find(link->src);
			auto socket_itr_2 = sockets.find(link->dst);
			
			auto & socket_state_1 = socket_itr_1->second;
			auto & socket_state_2 = socket_itr_2->second;
			
			setColor(40, 40, 40);
			hqLine(
				socket_state_1.position[0],
				socket_state_1.position[1],
				3.f,
				socket_state_2.position[0],
				socket_state_2.position[1],
				3.f);
		}
		hqEnd();
		
		hqBegin(HQ_FILLED_CIRCLES);
		for (auto & node_itr : graph->nodes)
		{
			auto * node = node_itr.get();
			
			for (auto & socket_itr : node->inputs)
			{
				auto * socket = socket_itr.get();
				
				auto socket_state_itr = sockets.find(socket_itr);
				
				if (socket_state_itr != sockets.end())
				{
					auto & socket_state = socket_state_itr->second;
					
					Color color = kColor_SocketIdle;
					
					if (graphEditor.isHovered(socket))
						color = kColor_SocketHovered;
					else if (graphEditor.isSelected(socket))
						color = kColor_SocketSelected;
					
					setColor(color);
					hqFillCircle(socket_state.position[0], socket_state.position[1], kSocketRadius);
				}
			}
		}
		hqEnd();
		
		hqBegin(HQ_FILLED_CIRCLES);
		for (auto & node_itr : graph->nodes)
		{
			auto * node = node_itr.get();
			
			Color color = kColor_NodeIdle;
			
			if (graphEditor.isHovered(node))
				color = kColor_NodeHovered;
			else if (graphEditor.isSelected(node))
				color = kColor_NodeSelected;
			
			setColor(color);
			hqFillCircle(node->position[0], node->position[1], kNodeRadius);
		}
		hqEnd();
	}
	
	//
	
	void moveSocketTo(std::weak_ptr<Socket> socket, Vec2Arg position)
	{
		auto socket_state_itr = sockets.find(socket);
		
		if (socket_state_itr != sockets.end())
		{
			auto & socket_state = socket_state_itr->second;
			
			const Vec2 delta = position - socket_state.position;
			const float distance = delta.CalcSize();
			
			socket_state.force += delta.CalcNormalized() * distance * 100.f;
		}
	}
};

struct GraphEditor : GraphEditorInterface
{
	float kNodeMoveThreshold = 4.f;
	float kSocketMoveThreshold = 4.f;
	
	enum State
	{
		kState_Idle,
		kState_LinkConnect,
		kState_NodeMoveDecide,
		kState_NodeMove,
		kState_SocketMoveDecide,
		kState_SocketMove
	};
	
	struct
	{
		std::weak_ptr<Node> node;
		Vec2 initialPosition;
	} nodeMoveDecide;
	
	struct
	{
		std::weak_ptr<Node> node;
	} nodeMove;
	
	struct
	{
		std::weak_ptr<Socket> socket;
		Vec2 initialPosition;
	} socketMoveDecide;
	
	struct
	{
		std::weak_ptr<Socket> socket;
	} socketMove;
	
	State state = kState_Idle;
	
	std::shared_ptr<Graph> graph;
	std::shared_ptr<GraphVisualizer> graphVisualizer;
	
	std::weak_ptr<Node> selectedNode;
	std::weak_ptr<Socket> selectedSocket;
	
	void tick()
	{
		const Vec2 mousePosition(mouse.x, mouse.y);
		
		switch (state)
		{
		case kState_Idle:
			{
				if (mouse.wentDown(BUTTON_LEFT))
				{
					GraphHittestResult result = graphVisualizer->hittest(mousePosition, std::weak_ptr<Socket>());
					
					if (result.node.expired() == false)
					{
						selectedNode = result.node;
						
						state = kState_NodeMoveDecide;
						nodeMoveDecide.node = result.node;
						nodeMoveDecide.initialPosition = mousePosition;
					}
					else if (result.socket.expired() == false)
					{
						selectedSocket = result.socket;
						
						state = kState_SocketMoveDecide;
						socketMoveDecide.socket = result.socket;
						socketMoveDecide.initialPosition = mousePosition;
					}
					else
					{
						std::shared_ptr<Node> newNode = std::make_shared<Node>();
						newNode->position = mousePosition;
						
						for (int i = 0; i < 6; ++i)
						{
							std::shared_ptr<Socket> socket = std::make_shared<Socket>();
							newNode->inputs.emplace_back(socket);
						}
						
						graph->nodes.emplace_back(newNode);
					}
				}
			}
			break;
		
		case kState_NodeMoveDecide:
			{
				const Vec2 delta = mousePosition - nodeMoveDecide.initialPosition;
				
				if (nodeMoveDecide.node.expired())
				{
					state = kState_Idle;
				}
				else if (mouse.wentUp(BUTTON_LEFT))
				{
					state = kState_Idle;
				}
				else
				{
					if (delta.CalcSize() >= kNodeMoveThreshold)
					{
						state = kState_NodeMove;
						nodeMove.node = nodeMoveDecide.node;
					}
				}
			}
			break;
			
		case kState_NodeMove:
			{
				if (nodeMove.node.expired())
				{
					state = kState_Idle;
				}
				else if (mouse.wentUp(BUTTON_LEFT))
				{
					state = kState_Idle;
				}
				else
				{
					nodeMove.node.lock()->position += Vec2(mouse.dx, mouse.dy);
				}
			}
			break;
			
		case kState_SocketMoveDecide:
			{
				const Vec2 delta = mousePosition - socketMoveDecide.initialPosition;
				
				if (socketMoveDecide.socket.expired())
				{
					state = kState_Idle;
				}
				else if (mouse.wentUp(BUTTON_LEFT))
				{
					state = kState_Idle;
				}
				else
				{
					if (delta.CalcSize() >= kSocketMoveThreshold)
					{
						state = kState_SocketMove;
						socketMove.socket = socketMoveDecide.socket;
					}
				}
			}
			break;
			
		case kState_SocketMove:
			{
				if (socketMove.socket.expired())
				{
					state = kState_Idle;
				}
				else if (mouse.wentUp(BUTTON_LEFT))
				{
					auto result = graphVisualizer->hittest(Vec2(mouse.x, mouse.y), socketMove.socket);
					
					if (result.socket.expired() == false)
					{
						std::shared_ptr<Link> link = std::make_shared<Link>();
						link->src = socketMove.socket;
						link->dst = result.socket;
						
						graph->links.emplace_back(link);
					}
					
					state = kState_Idle;
				}
				else
				{
					graphVisualizer->moveSocketTo(socketMove.socket, Vec2(mouse.x, mouse.y));
				}
			}
			break;
			
		case kState_LinkConnect:
			{
			
			}
			break;
		}
	}
	
	void draw() const
	{
		graphVisualizer->draw(*this);
	}
	
	// -- GraphEditorInterface --
	
	virtual bool isSelected(const Node * node) const
	{
		return selectedNode.lock().get() == node;
	}
	
	virtual bool isHovered(const Node * node) const
	{
		return false;
	}
	
	virtual bool isSelected(const Socket * socket) const
	{
		return selectedSocket.lock().get() == socket;
	}
	
	virtual bool isHovered(const Socket * socket) const
	{
		return false;
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;
	
	std::shared_ptr<Graph> graph = std::make_shared<Graph>();
	
	std::shared_ptr<GraphVisualizer> graphVisualizer = std::make_shared<GraphVisualizer>();
	graphVisualizer->graph = graph;
	
	std::unique_ptr<GraphEditor> editor(new GraphEditor());
	editor->graph = graph;
	editor->graphVisualizer = graphVisualizer;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		editor->tick();
		
		graphVisualizer->tick(framework.timeStep);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			editor->draw();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
