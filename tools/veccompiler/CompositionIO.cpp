#include "CompositionIO.h"
#include "Exception.h"
#include "Parse.h"
#include "StreamReader.h"
#include "StringEx.h"

enum CompSection
{
	CompSection_None,
	CompSection_Link
};

enum LibSection
{
	LibSection_None,
	LibSection_Item
};

void CompositionIO::Load_Library(Stream* stream, ShapeLib& out_Library)
{
	StreamReader reader(stream, false);
	
	LibSection section = LibSection_None;
	
	ShapeLibItem* item = 0;
	
	while (!reader.EOF_get())
	{
		std::string line = reader.ReadLine();
		
		std::string lineTrimmed = String::Trim(line);
		
		if (lineTrimmed == String::Empty)
			continue;
		if (String::StartsWith(lineTrimmed, "#"))
			continue;
			
		std::vector<std::string> lines = String::Split(lineTrimmed, " \t");
		
		if (String::StartsWith(line, "\t"))
		{
			switch (section)
			{
				case LibSection_None:
					throw ExceptionVA("properties start before section");
				
				case LibSection_Item:
				{
					if (lines[0] == "name")
					{
						item->m_Name = lines[1];
					}
					else if (lines[0] == "path")
					{
						item->m_Path = lines[1];
					}
					break;
				}
			}
		}
		else
		{
			if (line == "item")
			{
				out_Library.m_Items.push_back(ShapeLibItem());
				
				item = &out_Library.m_Items.back();
				
				section = LibSection_Item;
			}
			else
			{
				throw ExceptionVA("unknown section: %s", line.c_str());
			}
		}
	}
}

void CompositionIO::Load_Composition(Stream* stream, Composition& out_Composition)
{
	StreamReader reader(stream, false);
	
	CompSection section = CompSection_None;
	
	Link* link = 0;
	
	while (!reader.EOF_get())
	{
		std::string line = reader.ReadLine();
		
		std::string lineTrimmed = String::Trim(line);
		
		if (lineTrimmed == String::Empty)
			continue;
		if (String::StartsWith(lineTrimmed, "#"))
			continue;
			
		std::vector<std::string> lines = String::Split(lineTrimmed, " \t");
		
		if (String::StartsWith(line, "\t"))
		{
			switch (section)
			{
				case CompSection_None:
					throw ExceptionVA("properties start before section");
				
				case CompSection_Link:
				{
					if (lines[0] == "name")
					{
						link->m_Name = lines[1];
						
						//printf("link: name: %s\n", link->m_Name.c_str());
					}
					else if (lines[0] == "parent")
					{
						link->m_ParentName = lines[1];
					}
					else if (lines[0] == "shape")
					{
						link->m_ShapeName = lines[1];
					}
					else if (lines[0] == "position")
					{
						link->m_Location[0] = Parse::Float(lines[1]);
						link->m_Location[1] = Parse::Float(lines[2]);
					}
					else if (lines[0] == "base_angle")
					{
						link->m_AngleBase = Parse::Float(lines[1]);
					}
					else if (lines[0] == "angle")
					{
						link->m_Angle = Parse::Float(lines[1]);
					}
					else if (lines[0] == "min_angle")
					{
						link->m_AngleMin = Parse::Float(lines[1]);
					}
					else if (lines[0] == "max_angle")
					{
						link->m_AngleMax = Parse::Float(lines[1]);
					}
					else if (lines[0] == "angle_speed")
					{
						link->m_AngleSpeed = Parse::Float(lines[1]);
					}
					else if (lines[0] == "level" || lines[0] == "min_level")
					{
						link->m_LevelMin = Parse::Int32(lines[1]);
					}
					else if (lines[0] == "max_level")
					{
						link->m_LevelMax = Parse::Int32(lines[1]);
					}
					else if (lines[0] == "flags")
					{
						link->m_Flags = Parse::Int32(lines[1]);
					}
					else
					{
						throw ExceptionVA("unknown parameter: %s", lines[0].c_str());
					}
					break;
				}
			}
		}
		else
		{
			if (line == "link")
			{
				out_Composition.m_Links.push_back(Link());
				
				link = &out_Composition.m_Links.back();
				
				section = CompSection_Link;
			}
			else
			{
				throw ExceptionVA("unknown section: %s", line.c_str());
			}
		}
	}
}

void CompositionIO::Load(Stream* libStream, Stream* compStream, Composition& out_Composition)
{
	Load_Library(libStream, out_Composition.m_Library);
	Load_Composition(compStream, out_Composition);
}
