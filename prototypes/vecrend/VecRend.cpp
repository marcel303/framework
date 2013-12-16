#include "Precompiled.h"
#include <stdint.h>
#include "CompiledShape.h"
#include "Scanner.h"
#include "ShapeCompiler.h"
#include "StreamWriter.h"
#include "StringEx.h"
#include "Triangulate.h"
#include "Types.h"
#include "VecRend.h"
#include "VecRend_Bezier.h"

static void Prepare(Shape& shape, int quality, BBoxI& out_BBox, int& out_Sx, int& out_Sy)
{
	shape.Finalize(1);

	out_BBox = shape.m_BBWithStroke;

	int scale = quality;

	shape.Normalize(1);
	shape.Scale(scale);
	shape.Offset((scale - 1) / 2, (scale - 1) / 2);
	shape.Finalize(scale);

	out_Sx = out_BBox.ToRect().m_Size.x * scale;
	out_Sy = out_BBox.ToRect().m_Size.y * scale;
}

// todo: postpone downscaling, create 1 render pass instead

static void DrawFill(const ShapeItem& item, Buffer* buffer, int scale, DrawMode drawMode)
{
	const int sx = buffer->m_Sx;
	const int sy = buffer->m_Sy;

	LOG_DBG("DrawFill: size: %dx%d", sx, sy);
	
	ScanFill* scan = new ScanFill(sx, sy);

	float rgb[4];
	
	if (drawMode == DrawMode_Regular)
	{
		rgb[0] = item.m_RGB[0] / 255.0f * item.m_FillColor / 255.0f;
		rgb[1] = item.m_RGB[1] / 255.0f * item.m_FillColor / 255.0f;
		rgb[2] = item.m_RGB[2] / 255.0f * item.m_FillColor / 255.0f;
		rgb[3] = item.m_FillOpacity;
	}
	
	if (drawMode == DrawMode_Stencil)
	{
		rgb[0] = 1.0f;
		rgb[1] = 1.0f;
		rgb[2] = 1.0f;
		rgb[3] = item.m_FillOpacity;
	}

	switch (item.m_Type)
	{
	case ShapeType_Poly:
		{
			for (size_t i = 0; i < item.m_Poly.m_Triangles.size(); ++i)
			{
				const Triangle& triangle = item.m_Poly.m_Triangles[i];

				scan->Triangle(
					triangle.m_Points[0],
					triangle.m_Points[1],
					triangle.m_Points[2],
					1.0f,
					1.0f,
					1.0f,
					1.0f);
			}
		}
		break;

	case ShapeType_Circle:
		{
			const Circle& circle = item.m_Circle;

			scan->Circle(
				PointI(circle.x, circle.y),
				circle.r,
				1.0f,
				1.0f,
				1.0f,
				1.0f);
		}
		break;
	
	case ShapeType_Curve:
		{
			VecRend_RenderBezier(
				scan->Buffer_get(),
				&item.m_Curve.m_Points[0],
				item.m_Curve.m_Points.size() / 4,
				item.m_Stroke * scale,
				item.m_Hardness);
		}
		break;
			
	case ShapeType_Rectangle:
		{
			// nop
			//scan->Rect
		}
		break;
		
	case ShapeType_Picture:
		{
			// nop
		}
		break;
	
	case ShapeType_Tag:
	case ShapeType_Undefined:
		break;
	}

	//

	Buffer* scanBuffer = scan->Buffer_get();

	scanBuffer->Modulate(rgb);
//	printf("MOD ALPHA: %f / %d (%s)\n", rgb[3], item.m_RGB[3], item.m_Name.c_str());
	scanBuffer->ModulateAlpha(rgb[3]);

	//

	buffer->Combine(scanBuffer);

	//

	delete scan;
}

static void DrawPicture(const ShapeItem& item, Buffer* buffer, int scale, DrawMode drawMode)
{
	Buffer* temp = new Buffer();

	LOG_DBG("picture: scale=%d", item.m_Picture.m_Scale);
	LOG_DBG("picture: content_scale=%f", item.m_Picture.m_ContentScale);
	LOG_DBG("picture: quality=%d", scale);
	
	const Image& image = item.m_Picture.m_Image;
	
	if (item.m_Picture.m_ContentScale == 0.0f)
		throw ExceptionVA("invalid content scale: %f", item.m_Picture.m_ContentScale);
	
	//const float actualScale = item.m_Picture.m_Scale / (float)scale / item.m_Picture.m_ContentScale;
	const float actualScale = item.m_Picture.m_Scale / item.m_Picture.m_ContentScale;
	
	LOG_DBG("picture: actual_scale=%f", actualScale);
	
	const int scaledImageSx = static_cast<int>(image.m_Sx * actualScale);
	const int scaledImageSy = static_cast<int>(image.m_Sy * actualScale);
	
	if (scaledImageSx == 0 || scaledImageSy == 0)
		throw ExceptionVA("scaled image area is 0: %dx%d", scaledImageSx, scaledImageSy);

	LOG_DBG("scaled image size: %dx%d", scaledImageSx, scaledImageSy);
	
	temp->SetSize(scaledImageSx, scaledImageSy);
	
	const float rgbaScale = 1.0f / 255.0f;
	
	if (drawMode == DrawMode_Regular)
	{
		#if 1
#pragma message("warning: use proper resampling..")
		for (int y = 0; y < temp->m_Sy; ++y)
		{
			float* dline = temp->GetLine(y);
			
			for (int x = 0; x < temp->m_Sx; ++x)
			{
				int imageX = image.m_Sx * x / scaledImageSx;
				int imageY = image.m_Sy * y / scaledImageSy;
				
				if (imageX < 0 || imageX >= image.m_Sx ||
					imageY < 0 || imageY >= image.m_Sy)
				{
					dline[0] = 0.0f;
					dline[1] = 0.0f;
					dline[2] = 0.0f;
					dline[3] = 0.0f;
				}
				else
				{
					const ImagePixel* spixel = image.GetLine(imageY) + imageX;
					
					dline[0] = spixel->r * rgbaScale;
					dline[1] = spixel->g * rgbaScale;
					dline[2] = spixel->b * rgbaScale;
					dline[3] = spixel->a * rgbaScale;
				}
				
				dline += 4;
			}
		}
		#else
		for (int y = 0; y < image.m_Sy; ++y)
		{
			const ImagePixel* sline = image.GetLine(y);
			float* dline = temp->GetLine(y);

			for (int x = 0; x < image.m_Sx; ++x)
			{
				dline[0] = sline->r * rgbaScale;
				dline[1] = sline->g * rgbaScale;
				dline[2] = sline->b * rgbaScale;
				dline[3] = sline->a * rgbaScale;

				sline += 1;
				dline += 4;
			}
		}
		#endif
	}

	if (drawMode == DrawMode_Stencil)
	{
		#if 1
#pragma message("warning: use proper resampling..")
		for (int y = 0; y < temp->m_Sy; ++y)
		{
			float* dline = temp->GetLine(y);
			
			for (int x = 0; x < temp->m_Sx; ++x)
			{
				int imageX = image.m_Sx * x / scaledImageSx;
				int imageY = image.m_Sy * y / scaledImageSy;
				
				if (imageX < 0 || imageX >= image.m_Sx ||
					imageY < 0 || imageY >= image.m_Sy)
				{
					dline[0] = 1.0f;
					dline[1] = 1.0f;
					dline[2] = 1.0f;
					dline[3] = 0.0f;
				}
				else
				{
					const ImagePixel* spixel = image.GetLine(imageY) + imageX;
					
					dline[0] = 1.0f;
					dline[1] = 1.0f;
					dline[2] = 1.0f;
					dline[3] = spixel->a * rgbaScale;
				}
				
				dline += 4;
			}
		}
		#else
		for (int y = 0; y < image.m_Sy; ++y)
		{
			const ImagePixel* sline = image.GetLine(y);
			float* dline = temp->GetLine(y);

			for (int x = 0; x < image.m_Sx; ++x)
			{
				dline[0] = 1.0f;
				dline[1] = 1.0f;
				dline[2] = 1.0f;
				dline[3] = sline->a * rgbaScale;

				sline += 1;
				dline += 4;
			}
		}
		#endif
	}
	
	temp->ModulateAlpha(item.m_FillOpacity);
	
	//Buffer* temp2 = temp->Scale(scale);
	Buffer* temp2 = temp;

	buffer->Combine(temp2, item.m_Picture.m_Location.x - (scale - 1) / 2, item.m_Picture.m_Location.y - (scale - 1) / 2);
	
	//delete temp2;
	delete temp;
}

static float CalcDistance(const ShapeItem& item, const PointF& point)
{
	if (item.m_Type == ShapeType_Poly)
		return item.m_Poly.m_Hull.CalcDistance(point);
	if (item.m_Type == ShapeType_Circle)
		return item.m_Circle.CalcDistance(point);

	return 1000000.0f;
}

#if 0
static float CalcMinDistance(const Shape& shape, const PointF& point)
{
	float min = 1000000.0f;

	for (size_t i = 0; i < shape.m_Items.size(); ++i)
	{
		const ShapeItem& item = shape.m_Items[i];

		if (!item.m_BBWithStroke.IsInside((int)point.x, (int)point.y))
			continue;

		float d = CalcDistance(item, point);

		if (d < min)
			min = d;
	}

	return min;
}
#endif

// note: quality should be uneven for best results

Buffer* VecRend_CreateBuffer(Shape shape, int quality, DrawMode drawMode)
{
	BBoxI bbox;
	int sx, sy;

	Prepare(shape, quality, bbox, sx, sy);

	int scale = quality;

	//

	for (size_t i = 0; i < shape.m_Items.size(); ++i)
	{
		ShapeItem& item = shape.m_Items[i];

		if (item.m_Type == ShapeType_Poly)
		{
			item.m_Poly.m_Hull.Construct(item.m_Poly.m_Poly.m_Points);

			Vector2dVector outline;
			Vector2dVector triangles;

			for (size_t i = 0; i < item.m_Poly.m_Poly.m_Points.size(); ++i)
			{
				PointF point = PointF(
					(float)item.m_Poly.m_Poly.m_Points[i].x,
					(float)item.m_Poly.m_Poly.m_Points[i].y);

				outline.push_back(point);
			}

			Triangulate::Process(outline, triangles);

			for (size_t i = 0; i < triangles.size() / 3; ++i)
			{
				Triangle triangle;

				triangle.m_Points[0] = PointF(triangles[i * 3 + 0].x, triangles[i * 3 + 0].y);
				triangle.m_Points[1] = PointF(triangles[i * 3 + 1].x, triangles[i * 3 + 1].y);
				triangle.m_Points[2] = PointF(triangles[i * 3 + 2].x, triangles[i * 3 + 2].y);

				item.m_Poly.m_Triangles.push_back(triangle);
			}
		}
	}

	//

	Buffer* buffer = new Buffer();

	buffer->SetSize(sx, sy);
	buffer->Clear(0.0f);

	for (size_t i = 0; i < shape.m_Items.size(); ++i)
	{
		const ShapeItem& item = shape.m_Items[i];

		if (!item.m_Visible)
			continue;

		if (drawMode == DrawMode_Regular || drawMode == DrawMode_Stencil)
		{
			if (item.m_Filled)
				DrawFill(item, buffer, scale, drawMode);

			if (item.m_Type == ShapeType_Picture)
			{
				// todo: differentiate between picture/curve and vector types

				DrawPicture(item, buffer, scale, drawMode);
			}
		}

		if (item.m_Type == ShapeType_Picture)
			continue;
		
		if ((drawMode == DrawMode_Regular || drawMode == DrawMode_Stencil) && item.m_Stroke != 0.0f)
		{
			int bx1 = (int)floorf(item.m_BBWithStroke.m_Min.x / (float)scale);
			int by1 = (int)floorf(item.m_BBWithStroke.m_Min.y / (float)scale);
			int bx2 = (int)ceilf(item.m_BBWithStroke.m_Max.x / (float)scale);
			int by2 = (int)ceilf(item.m_BBWithStroke.m_Max.y / (float)scale);

			if (bx1 < 0)
				bx1 = 0;
			if (by1 < 0)
				by1 = 0;
			if (bx2 >= sx / scale)
				bx2 = sx / scale - 1;
			if (by2 >= sy / scale)
				by2 = sy / scale - 1;

			//

			const float stroke = item.m_Stroke;

			const float strokeRcp = 1.0f / (stroke * scale);

			const float rgbScale = 1.0f / 255.0f / 255.0f;

			//

			for (int y = by1; y <= by2; ++y)
			{
				const int y1 = (y + 0) * scale;
				const int y2 = (y + 1) * scale;

				for (int x = bx1; x <= bx2; ++x)
				{
					const int x1 = (x + 0) * scale;
					const int x2 = (x + 1) * scale;

					PointF temp;

					temp.x = (float)x1;
					temp.y = (float)y1;

					//float min = CalcMinDistance(shape, temp);
					float min = CalcDistance(item, temp);

					if (min <= (stroke + 1.0f) * scale)
					{
						for (int ys = y1; ys < y2; ++ys)
						{
							float* line = buffer->GetLine(ys) + x1 * 4;

							for (int xs = x1; xs < x2; ++xs)
							{
								const PointF point((float)xs, (float)ys);

								const float d = CalcDistance(item, point);

								//

								float f = d * strokeRcp;

								f = pow(f, item.m_Hardness);
								//f = pow(f, 0.5f);
								//f = pow(f, 2.0f);
								//f = pow(f, 3.0f);
								//f = pow(f, 4.0f);

								float c = 1.0f - f;

								if (c < 0.0f)
									c = 0.0f;
								if (c > 1.0f)
									c = 1.0f;

								//
								
								c = c * item.m_LineOpacity;
								
								//

								const float t1 = 1.0f - c;
								const float t2 = c;

								for (int i = 0; i < 3; ++i)
									line[i] = line[i] * t1 + t2 * rgbScale * item.m_RGB[i] * item.m_LineColor;

								if (t2 > line[3])
									line[3] = t2;

								//

								line += 4;
							}
						}
					}
				}
			}
		}
	}
	
	if (drawMode == DrawMode_Stencil)
	{
		for (int y = 0; y < buffer->m_Sy; ++y)
		{
			float* line = buffer->GetLine(y);
			
			for (int x = 0; x < buffer->m_Sx; ++x)
			{
				line[0] = line[1] = line[2] = 1.0f;
				
				line += 4;
			}
		}
	}
	
#if 0
	float size = 50.0f;

	PointF points[4] =
	{
		PointF(0.0f * scale, 0.0f * scale),
		PointF(size * 2.0f * scale, 0.0f * scale),
		PointF(size * 3.0f * scale, size * scale),
		PointF(size * 4.0f * scale, size * scale)
	};

	for (int i = 0; i < 4; ++i)
	{
		points[i].x += (scale - 1) / 2;
		points[i].y += (scale - 1) / 2;
	}

	Buffer* temp1 = CreateBuffer(buffer->m_Sx, buffer->m_Sy, scale);

	VecRend_RenderBezier(temp1, points, 1, 2.0f * scale);

	Buffer* temp2 = temp1->DownScale(scale);

	buffer->Combine(temp2);

	delete temp1;
	delete temp2;
#endif

	Buffer* result = buffer->DownScale(scale);

	delete buffer;

	return result;
	//return buffer;
}

void VecCompile_Compile(Shape shape, std::string texFileName, std::string silFileName, Stream* stream)
{
	VRCS::CompiledShape compiledShape;
	
	VRCS::ShapeCompiler::Compile(shape, texFileName, silFileName, compiledShape);
	
	compiledShape.Save(stream);
}
