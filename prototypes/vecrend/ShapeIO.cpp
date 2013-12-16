#include "Precompiled.h"
#include "FileStream.h"
#include "Parse.h"
#include "ShapeIO.h"
#include "StringEx.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "Types.h"

/*

--------------------
VG File Format
--------------------

<element-type>
	name <name>
	group <group>
	color <red green blue> [alpha]
	stroke <width>
	fill <brightness>
	line <brightness>
	visible <boolean>
	filled <boolean>
	collision <boolean>

	[point] <x y>
	[position] <x y>
	[radius] <radius>

element-type = poly | circle | tag
poly = point[]
circle = position & radius
rect = position & size
tag = position
picture = position & path

<graphic>
	has_silhouette <boolean>
	has_sprite <boolean>
*/

enum SectionType
{
	SectionType_Undefined,
	SectionType_Item,
	SectionType_Graphic,
	SectionType_Editor
};

int ShapeIO::LoadVG(const char* fileName, Shape& shape, IImageLoader* imageLoader)
{
	shape = Shape();

	Stream* stream = new FileStream();
	stream->Open(fileName, OpenMode_Read);

	StreamReader* reader = new StreamReader(stream, false);

	SectionType sectionType = SectionType_Undefined;
	ShapeItem* item = 0;

	while (!reader->EOF_get())
	{
		std::string line = reader->ReadLine();
		
		//printf("line: %s\n", line.c_str());
		
		if (String::Trim(line).empty())
			continue;
		if (line[0] == '#')
			continue;

		if (String::StartsWith(line, "\t"))
		{
			line = String::TrimLeft(line);

			std::vector<std::string> lines = String::Split(line, ' ');

			if (sectionType == SectionType_Item)
			{
				if (lines[0] == "name")
				{
					item->m_Name = lines[1];
				}
				else if (lines[0] == "group")
				{
					item->m_Group = lines[1];
				}
				else if (lines[0] == "color")
				{
					item->m_RGB[0] = Parse::Int32(lines[1]);
					item->m_RGB[1] = Parse::Int32(lines[2]);
					item->m_RGB[2] = Parse::Int32(lines[3]);
					
					/*
					if (lines.size() >= 5)
						item->m_RGB[3] = Parse::Int32(lines[4]);
					else
						item->m_RGB[3] = 255;*/
				}
				else if (lines[0] == "stroke")
				{
					item->m_Stroke = Parse::Float(lines[1]);
					
					if (item->m_Stroke < 0.0f)
						throw ExceptionVA("stroke < 0");
				}
				else if (lines[0] == "hardness")
				{
					item->m_Hardness = Parse::Float(lines[1]);
					
					if (item->m_Hardness < 0.0f)
						throw ExceptionVA("hardness < 0");
				}
				else if (lines[0] == "fill")
				{
					item->m_FillColor = Parse::Int32(lines[1]);
					
					if (lines.size() >= 3)
						item->m_FillOpacity = Parse::Float(lines[2]);
						
					if (item->m_FillOpacity < 0.0f)
						throw ExceptionVA("fill opacity < 0");
					if (item->m_FillOpacity > 1.0f)
						throw ExceptionVA("fill opacity > 0");
				}
				else if (lines[0] == "line")
				{
					item->m_LineColor = Parse::Int32(lines[1]);
					
					if (lines.size() >= 3)
						item->m_LineOpacity = Parse::Float(lines[2]);
						
					if (item->m_FillOpacity < 0.0f)
						throw ExceptionVA("line opacity < 0");
					if (item->m_FillOpacity > 1.0f)
						throw ExceptionVA("line opacity > 0");
				}
				else if (lines[0] == "visible")
				{
					item->m_Visible = Parse::Bool(lines[1]);
				}
				else if (lines[0] == "filled")
				{
					item->m_Filled = Parse::Bool(lines[1]);
				}
				else if (lines[0] == "collision")
				{
					item->m_Collision = Parse::Bool(lines[1]);
				}
				else
				{
					switch (item->m_Type)
					{
					case ShapeType_Poly:
						{
							if (lines[0] == "point")
							{
								PointI point;
	
								point.x = Parse::Int32(lines[1]);
								point.y = Parse::Int32(lines[2]);
	
								item->m_Poly.m_Poly.m_Points.push_back(point);
							}
							else
							{
								throw ExceptionVA(String::Format("unknown parameter: %s", lines[0].c_str()).c_str());
							}
						}
						break;
	
					case ShapeType_Circle:
						{
							if (lines[0] == "position")
							{
								item->m_Circle.x = Parse::Int32(lines[1]);
								item->m_Circle.y = Parse::Int32(lines[2]);
							}
							else if (lines[0] == "radius")
							{
								item->m_Circle.r = Parse::Int32(lines[1]);
							}
							else
							{
								throw ExceptionVA(String::Format("unknown parameter: %s", lines[0].c_str()).c_str());
							}
						}
						break;
						
					case ShapeType_Curve:
						{
							if (lines[0] == "point")
							{
								CurvePoint point;

								point.x = Parse::Int32(lines[1]);
								point.y = Parse::Int32(lines[2]);

								if (lines.size() == 3)
								{
									point.tx1 = 0;
									point.ty1 = 0;
									point.tx2 = 0;
									point.ty2 = 0;
								}
								else if (lines.size() == 5)
								{
									point.tx1 = Parse::Int32(lines[3]);
									point.ty1 = Parse::Int32(lines[4]);
									point.tx2 = -point.tx1;
									point.ty2 = -point.ty1;
								}
								else if (lines.size() == 7)
								{
									point.tx1 = Parse::Int32(lines[3]);
									point.ty1 = Parse::Int32(lines[4]);
									point.tx2 = Parse::Int32(lines[5]);
									point.ty2 = Parse::Int32(lines[6]);
								}

								item->m_Curve.m_Curve.m_Points.push_back(point);
							}
							else if (lines[0] == "closed")
							{
								item->m_Curve.m_Curve.m_IsClosed = Parse::Bool(lines[1]);
							}
							else
							{
								throw ExceptionVA(String::Format("unknown parameter: %s", lines[0].c_str()).c_str());
							}
						}
						break;
	
					case ShapeType_Rectangle:
						{
							if (lines[0] == "position")
							{
								item->m_Rectangle.m_Location.x = static_cast<float>(Parse::Int32(lines[1]));
								item->m_Rectangle.m_Location.y = static_cast<float>(Parse::Int32(lines[2]));
							}
							else if (lines[0] == "size")
							{
								item->m_Rectangle.m_Size.x = static_cast<float>(Parse::Int32(lines[1]));
								item->m_Rectangle.m_Size.y = static_cast<float>(Parse::Int32(lines[2]));
							}
							else
							{
								throw ExceptionVA(String::Format("unknown parameter: %s", lines[0].c_str()).c_str());
							}
						}
						break;
							
					case ShapeType_Tag:
						{
							if (lines[0] == "position")
							{
								item->m_Tag.x = Parse::Int32(lines[1]);
								item->m_Tag.y = Parse::Int32(lines[2]);
							}
							else
							{
								throw ExceptionVA(String::Format("unknown parameter: %s", lines[0].c_str()).c_str());
							}
						}
						break;
						
					case ShapeType_Picture:
						{
							if (lines[0] == "position")
							{
								item->m_Picture.m_Location.x = Parse::Int32(lines[1]);
								item->m_Picture.m_Location.y = Parse::Int32(lines[2]);
							}
							else if (lines[0] == "path")
							{
								item->m_Picture.m_Path = lines[1];
								item->m_Picture.Load(imageLoader);
							}
							else if (lines[0] == "content_scale")
							{
								item->m_Picture.m_ContentScale = static_cast<float>(Parse::Int32(lines[1]));
							}
							else
							{
								throw ExceptionVA(String::Format("unknown parameter: %s", lines[0].c_str()).c_str());
							}
						}
						break;
						
					case ShapeType_Undefined:
						{
							throw ExceptionVA("unknown shape type: %d", item->m_Type);
						}
						break;
					}
				}
			}
			else if (sectionType == SectionType_Graphic)
			{
				if (lines[0] == "has_silhouette")
				{
					shape.m_HasShadow = Parse::Bool(lines[1]);
				}
				else if (lines[0] == "has_sprite")
				{
					shape.m_HasSprite = Parse::Bool(lines[1]);
				}
				else
				{
					throw ExceptionVA(String::Format("unknown parameter: %s", lines[0].c_str()).c_str());
				}
			}
			else if (sectionType == SectionType_Editor)
			{
				// nop. skip editor properties
			}
			else
			{
				throw ExceptionVA("parameters precede element");
			}
		}
		else
		{
			sectionType = SectionType_Undefined;
			
			ShapeType type = ShapeType_Undefined;

			item = 0;
			
			if (line == "poly")
			{
				sectionType = SectionType_Item;
				type = ShapeType_Poly;
			}
			else if (line == "circle")
			{
				sectionType = SectionType_Item;
				type = ShapeType_Circle;
			}
			else if (line == "curve")
			{
				sectionType = SectionType_Item;
				type = ShapeType_Curve;
			}
			else if (line == "rect")
			{
				sectionType = SectionType_Item;
				type = ShapeType_Rectangle;
			}
			else if (line == "tag")
			{
				sectionType = SectionType_Item;
				type = ShapeType_Tag;
			}
			else if (line == "picture")
			{
				sectionType = SectionType_Item;
				type = ShapeType_Picture;
			}
			else if (line == "graphic")
			{
				sectionType = SectionType_Graphic;
			}
			else if (line == "editor")
			{
				sectionType = SectionType_Editor;
			}
			
			if (sectionType == SectionType_Undefined)
			{
				throw ExceptionVA(String::Format("unknown section type: %s", line.c_str()).c_str());
			}
			else if (sectionType == SectionType_Graphic)
			{
				// todo: initialize graphic settings
			}
			else if (sectionType == SectionType_Editor)
			{
				// nop
			}
			else if (sectionType == SectionType_Item)
			{
				if (type == ShapeType_Undefined)
				{
					throw ExceptionVA(String::Format("unknown section type: %s", line.c_str()).c_str());
				}
				else
				{
					shape.m_Items.push_back(ShapeItem());
					item = &shape.m_Items.back();
					item->m_Type = type;
				}
			}
			else
			{
				throw ExceptionVA("unknown section type: %d", (int)sectionType);
			}
		}
	}

	delete reader;
	delete stream;

	return 1;
}

int ShapeIO::SaveVG(const Shape& shape, char* fileName)
{
	Stream* stream = new FileStream();
	stream->Open(fileName, OpenMode_Write);
	StreamWriter* writer = new StreamWriter(stream, false);

	// todo: write settings
	
	// write-out shape items
	
	for (size_t i = 0; i < shape.m_Items.size(); ++i)
	{
		const ShapeItem& item = shape.m_Items[i];

		switch (item.m_Type)
		{
		case ShapeType_Poly:
			writer->WriteLine("poly");
			for (size_t i = 0; i < item.m_Poly.m_Poly.m_Points.size(); ++i)
			{
				const PointI& point = item.m_Poly.m_Poly.m_Points[i];

				writer->WriteLine(String::Format("\tpoint %d %d", point.x, point.y));
			}
			break;

		case ShapeType_Circle:
			{
				const Circle& circle = item.m_Circle;
				writer->WriteLine("circle");
				writer->WriteLine(String::Format("\tposition %d %d", circle.x, circle.y));
				writer->WriteLine(String::Format("\tradius %d",  circle.r));
			}
			break;

		case ShapeType_Curve:
			{
				const Curve& curve = item.m_Curve.m_Curve;
				writer->WriteLine("curve");
				for (size_t i = 0; i < curve.m_Points.size(); ++i)
				{
					const CurvePoint& point = curve.m_Points[i];
					writer->WriteLine(String::Format("point %d %d %d %d %d",
						point.x,
						point.y,
						point.tx1,
						point.ty1,
						point.tx2,
						point.ty2));
				}
			}
			break;
				
		case ShapeType_Rectangle:
			{
				const DrawRectangle& rectangle = item.m_Rectangle;
				writer->WriteLine("rect");
				writer->WriteLine(String::Format("\tposition %d %d", rectangle.m_Location.x, rectangle.m_Location.y));
				writer->WriteLine(String::Format("\tsize %d %d", rectangle.m_Size.x, rectangle.m_Size.y));
			}
			break;

		case ShapeType_Tag:
			writer->WriteLine("tag");
			break;
			
		case ShapeType_Picture:
			writer->WriteLine("picture");
			break;
			
		case ShapeType_Undefined:
			throw ExceptionVA("shape type undefined");
			break;
		}

// todo: fill / line opacity

		writer->WriteLine(String::Format("\tname %s", item.m_Name.c_str()));
		if (item.m_Group != String::Empty)
			writer->WriteLine(String::Format("\tgroup %s", item.m_Group.c_str()));
		writer->WriteLine(String::Format("\tcolor %d %d %d %d", item.m_RGB[0], item.m_RGB[1], item.m_RGB[2], item.m_RGB[3]));
		writer->WriteLine(String::Format("\tstroke %f", item.m_Stroke));
		writer->WriteLine(String::Format("\thardness %f", item.m_Hardness));
		writer->WriteLine(String::Format("\tfill %d", item.m_FillColor));
		writer->WriteLine(String::Format("\tline %d", item.m_LineColor));
	}

	delete writer;
	delete stream;

	return 0;
}
