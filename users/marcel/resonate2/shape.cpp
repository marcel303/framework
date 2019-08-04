#include "framework.h"
#include "Log.h"
#include "Parse.h"
#include "shape.h"
#include "TextIO.h"
#include <limits>
#include <vector>

extern void splitString(const std::string & str, std::vector<std::string> & result);

bool ShapeDefinition::loadFromFile(const char * filename)
{
	numPlanes = 0;
	
	//
	
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (TextIO::load(filename, lines, lineEndings) == false)
	{
		LOG_ERR("failed to load text from file", 0);
		return false;
	}
	else
	{
		for (auto & line : lines)
		{
			std::vector<std::string> parts;
			
			splitString(line, parts);
			
			if (parts.empty())
				continue;
			
			if (parts[0] == "plane")
			{
				// plane definition
				
				if (numPlanes == kMaxPlanes)
				{
					LOG_ERR("shape defines too many planes. maximum count is %d", kMaxPlanes);
					return false;
				}
				else
				{
					auto & plane = planes[numPlanes++];
					
					if (parts.size() - 1 >= 4)
					{
						plane.normal[0] = Parse::Float(parts[1]);
						plane.normal[1] = Parse::Float(parts[2]);
						plane.normal[2] = Parse::Float(parts[3]);
						
						plane.offset = Parse::Float(parts[4]);
					}
				}
			}
			else
			{
				LOG_ERR("syntax error: %s", line.c_str());
				return false;
			}
		}
	}
	
	return true;
}

void ShapeDefinition::makeRandomShape(const int in_numPlanes)
{
	Assert(in_numPlanes <= kMaxPlanes);
	numPlanes = in_numPlanes;
	
	for (int i = 0; i < numPlanes; ++i)
	{
		const float dx = random(-1.f, +1.f);
		const float dy = random(-1.f, +1.f);
		const float dz = random(-1.f, +1.f);
		
		const Vec3 normal = Vec3(dx, dy, dz).CalcNormalized();
		
		const float offset = .5f;
		
		planes[i].normal = normal;
		planes[i].offset = offset;
	}
}

float ShapeDefinition::intersectRay_directional(Vec3Arg rayDirection, int & planeIndex) const
{
	float t = std::numeric_limits<float>::infinity();
	
	planeIndex = -1;
	
	for (int i = 0; i < numPlanes; ++i)
	{
		const float dd = planes[i].normal * rayDirection;
		
		if (dd > 0.f)
		{
			const float d1 = planes[i].offset;
			const float pt = d1 / dd;
			
			Assert(pt >= 0.f);
			
			if (pt < t)
			{
				t = pt;
				planeIndex = i;
			}
		}
	}
	
	Assert(planeIndex != -1);
	
	return t;
}

float ShapeDefinition::intersectRay(Vec3Arg rayOrigin, Vec3Arg rayDirection) const
{
	float t = std::numeric_limits<float>::infinity();
	
	for (int i = 0; i < numPlanes; ++i)
	{
		const float dd = planes[i].normal * rayDirection;
		
		if (dd > 0.f)
		{
			const float d1 = planes[i].offset - planes[i].normal * rayOrigin;
			const float pt = d1 / dd;
			
			if (pt >= 0.f && pt < t)
				t = pt;
		}
	}
	
	return t;
}
