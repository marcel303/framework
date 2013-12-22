#include <assert.h>
#include <ctime>
#include <stdio.h>
#include <string.h>

template <typename T>
class RedBlackNode
{
private:
	typedef RedBlackNode<T> Self;
	
	T m_v;
	char m_c;
	Self * m_p;
	Self * m_l;
	Self * m_r;
	
public:
	inline const T & Value() const { return m_v; }
	inline int C() const { return m_c; }
	inline Self * P() const { return m_p; }
	inline Self * L() const { return m_l; }
	inline Self * R() const { return m_r; }
	
	inline void SetValue(const T & v) { m_v = v; }
	inline void SetC(int c) { m_c = c; }
	inline void SetP(Self * n) { m_p = n; }
	inline void SetL(Self * n) { m_l = n; }
	inline void SetR(Self * n) { m_r = n; }
};

template <typename T, typename N = RedBlackNode<T> >
class RedBlackTree
{
public:
	typedef N Node;
	
	class iterator
	{
	public:
		iterator(Node * n)
		{	
			if (n != 0)
			{
				while (n->L() != 0)
					n = n->L();
			}
			
			m_n = n;
		}
		
		inline bool operator==(const iterator & i) const { return m_n == i.m_n; }
		inline bool operator!=(const iterator & i) const { return m_n != i.m_n; }
		inline void operator++() { Next(); }
		inline const T & get_value() const { return m_n->Value(); }
		
	private:
		void Next()
		{
			Node * r = m_n->R();
			
			if (r != 0)
			{
				Node * n = r;
				
				for (Node * l = n->L(); l != 0; l = l->L())
					n = l;
				
				m_n = n;
			}
			else
			{
				Node * n;
				do
				{
					n = m_n;
					m_n = m_n->P();
				} while (m_n != 0 && m_n->R() == n);
			}
		}
		
		Node * m_n;
	};
	
	void insert(Node * n) { Insert(n); }
	void clear() { r = 0; }
	iterator begin() { return iterator(r); }
	iterator end() { return iterator(0); }
	
private:
	static const int RED = 0;
	static const int BLACK = 1;
	
	Node * r;
	
	void Insert(Node * n)
	{
		if (r == 0)
		{
			r = n;
		}
		else
		{
			Insert_Leaf(r, n);
		}
		
		Insert_CheckColor(n);
	}

	void Insert_Leaf(Node * __restrict p, Node * __restrict n)
	{
		while (true)
		{
			if (n->Value() < p->Value())
			{
				Node * __restrict l = p->L();
				
				if (l != 0)
				{
					p = l;
				}
				else
				{
					p->SetL(n);
					n->SetP(p);
					break;
				}
			}
			else
			{
				Node * __restrict r = p->R();
				
				if (r != 0)
				{
					p = r;
				}
				else
				{
					p->SetR(n);
					n->SetP(p);
					break;
				}
			}
		}
	}
	
	void Insert_CheckColor(Node * n)
	{
		Node * p = n->P();
		
		if (p == 0)
		{
			n->SetC(BLACK);
		}
		else if (p->C() == BLACK)
		{
			return;
		}
		else
		{
			Insert_Colorize(n);
		}
	}
	
	void Insert_Colorize(Node * n)
	{
		Node * __restrict u = Uncle(n);
		
		if (u != 0 && u->C() == RED)
		{
			Node * __restrict p = n->P();
			Node * __restrict g = p->P();
			
			p->SetC(BLACK);
			u->SetC(BLACK);
			g->SetC(RED);
			
			Insert_CheckColor(g);
		}
		else
		{
			Insert_CheckBalance(n);
		}
	}
	
	void Insert_CheckBalance(Node * n)
	{
		Node * p = n->P();
		Node * g = p->P();
		
		if (n == p->R() && p == g->L())
		{
			RotateLeft(p);
			n = n->L();
		}
		else if (n == p->L() && p == g->R())
		{
			RotateRight(p);
			n = n->R();
		}
		
		Insert_Rebalance(n);
	}
	
	void Insert_Rebalance(Node * n)
	{
		Node * __restrict p = n->P();
		Node * __restrict g = p->P();
		
		p->SetC(BLACK);
		g->SetC(RED);
		
		if (n == p->L())
		{
			RotateRight(g);
		}
		else
		{
			RotateLeft(g);
		}
	}
	
	//
	
	void RotateLeft(Node * __restrict n)
	{
		Node * __restrict x = n->R();
		Node * __restrict p = n->P();
		
		if (p != 0)
		{
			if (n == p->L())
				p->SetL(x);
			else
				p->SetR(x);
		}
		
		{
			n->SetR(x->L());
			if (x->L() != 0)
				x->L()->SetP(n);
			x->SetL(n);
			x->SetP(n->P());
		}
		
		n->SetP(x);
		if (n == r)
			r = x;
	}
	
	void RotateRight(Node * __restrict n)
	{
		Node * __restrict x = n->L();
		Node * __restrict p = n->P();
		
		if (p != 0)
		{
			if (n == p->R())
				p->SetR(x);
			else
				p->SetL(x);
		}

		{
			n->SetL(x->R());
			if (x->R() != 0)
				x->R()->SetP(n);
			x->SetR(n);
			x->SetP(n->P());
		}
		
		n->SetP(x);
		if (n == r)
			r = x;
	}
	
	//
	
	inline static Node * GrandParent(const Node * n)
	{
		const Node * p = n->P();
		if (p != 0)
			return p->P();
		else
			return 0;
	}
	
	inline static Node * Uncle(const Node * n)
	{
		const Node * g = GrandParent(n);
		
		if (g != 0)
		{
			Node * l = g->L();
			Node * r = g->R();
			if (l == n->P())
				return r;
			else
				return l;
		}
		else
		{
			return 0;
		}
	}
};

//

extern void testRadixSort();

//

int main (int argc, const char * argv[])
{
	testRadixSort();
	
	typedef RedBlackTree<unsigned short> Tree;
	
	Tree t;
	
	int n = 1 << 16;
	
	Tree::Node * nodes = new Tree::Node[n];
	int * nv = new int[n];
	for (int i = 0; i < n; ++i)
		//nv[i] = rand() % (i + 1);
		nv[i] = rand();
		//nv[i] = rand() % n;

	clock_t time = 0;
	
	for (int i = 0; i < 1; ++i)
	{
		memset(nodes, 0, sizeof(Tree::Node) * n);
		
		t.clear();
		
		time -= clock();
		
		for (int i = 0; i < n; ++i)
		{
			int v = nv[i];
			//int v = i % 10;
			//int v = rand() % n;
			
			Tree::Node & n = nodes[i];
			
			n.SetValue(v);
			
			t.insert(&n);
		}
		
		time += clock();
	}
	
#if 1
	printf("time = %gms\n", float(time) / CLOCKS_PER_SEC * 1000.f);
#endif
	
#if 0
	int c = 0;
	
	for (Tree::iterator i = t.begin(); i != t.end(); ++i)
	{
		int v = i.get_value();
		printf("v: %d\n", v);
		c++;
	}
	
	printf("%d elems total\n", c);
#endif
	
	delete[] nodes;
	nodes = 0;
	
	delete[] nv;
	nv = 0;
	
	return 0;
}
