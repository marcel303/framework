#include "Calc.h"
#include "enemy_path.h"
#include "Parse.h"
#include "render.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"

#ifndef DEPLOYMENT
#include <vector>
#endif

EnemyPathNode::EnemyPathNode()
{
	distance = 0.0f;
}

EnemyPath::EnemyPath()
{
	mNodes = 0;
	mNodeCount = 0;
}

EnemyPath::~EnemyPath()
{
	delete[] mNodes;
	mNodes = 0;
	mNodeCount = 0;
}

void EnemyPath::Load(Stream* stream)
{
	std::vector<Vec2F> points;
	
//#ifndef DEPLOYMENT
#if 0
	points.push_back(Vec2F(0.0f, 120.0f));
	points.push_back(Vec2F(240.0f, 120.0f));
	points.push_back(Vec2F(240.0f, 360.0f));
	points.push_back(Vec2F(100.0f, 360.0f));
	points.push_back(Vec2F(100.0f, 0.0f));
#else
	StreamReader reader(stream, false);
	
	while (!stream->EOF_get())
	{
		std::string line = String::Trim(reader.ReadLine());
		
		if (line.empty())
			continue;
		
		std::vector<std::string> temp = String::Split(line, ' ');
		
		if (temp.size() != 2)
			throw ExceptionVA("invalid path");
		
		int x = Parse::Int32(temp[0]);
		int y = Parse::Int32(temp[1]);
		
		points.push_back(Vec2F(x, y));
	}
#endif
	
	mNodeCount = points.size();
	mNodes = new EnemyPathNode[mNodeCount];
	
	for (size_t i = 0; i < points.size(); ++i)
	{
		Vec2F p1 = points[i];
		Vec2F p2 = p1;
		
		if (i + 1 < points.size())
			p2 = points[i + 1];
		
		float distance = p1.DistanceTo(p2);
		
		mNodes[i].position[0] = p1[0];
		mNodes[i].position[1] = p1[1];
		mNodes[i].distance = distance;
	}
}

void EnemyPath::Save(Stream* stream)
{
	StreamWriter writer(stream, false);
	
	writer.WriteInt32(mNodeCount);
	
	for (int i = 0; i < mNodeCount; ++i)
	{
		writer.WriteFloat(mNodes[i].position[0]);
		writer.WriteFloat(mNodes[i].position[1]);
		writer.WriteFloat(mNodes[i].distance);
	}
}

void EnemyPath::Update(float dt)
{
}

void EnemyPath::Render()
{
	for (int i = 0; i < mNodeCount - 1; ++i)
	{
		EnemyPathNode& node1 = mNodes[i + 0];
		EnemyPathNode& node2 = mNodes[i + 1];
		
		gRender->Line(node1.position[0], node1.position[1], node2.position[0], node2.position[1], Color(1.0f, 1.0f, 1.0f));
	}
}

int EnemyPath::NodeCount_get() const
{
	return mNodeCount;
}

EnemyPathNode* EnemyPath::Node_get(int index)
{
	if (index < 0 || index >= mNodeCount)
		return 0;
	
	return &mNodes[index];
}

float EnemyPath::CalculateDistance(Vec2F location)
{
	float minDistanceSq = -1.0f;
	
	// calculate distance to points
	
	for (int i = 0; i < mNodeCount; ++i)
	{
		float distanceSq = mNodes[i].position.DistanceSq(location);
		
		if (distanceSq < minDistanceSq || minDistanceSq < 0.0f)
			minDistanceSq = distanceSq;
	}
	
	// calculate distance to line segments
	
	for (int i = 0; i < mNodeCount - 1; ++i)
	{
		EnemyPathNode* node1 = mNodes + i + 0;
		EnemyPathNode* node2 = mNodes + i + 1;
		
		Vec2F p1 = node1->position;
		Vec2F p2 = node2->position;
		
		Vec2F delta = p2 - p1;
		
		float length = delta.Length_get();
		
		if (length == 0.0f)
			continue;
		
		Vec2F normal1 = delta.Normal();
		Vec2F normal2 = -normal1;
		
		float d1 = p1 * normal1;
		float d2 = p2 * normal2;
		
		if (location * normal1 - d1 < 0.0f)
			continue;
		if (location * normal2 - d2 < 0.0f)
			continue;
		
		Vec2F normal = Vec2F::FromAngle(normal1.ToAngle() + Calc::mPI2);
		
		float d = p1 * normal;
		
		float distance = Calc::Abs(location * normal - d);
		float distanceSq = distance * distance;
		
		if (distanceSq < minDistanceSq || minDistanceSq < 0.0f)
			minDistanceSq = distanceSq;
	}
	
	return Calc::Sqrt(minDistanceSq);
}
