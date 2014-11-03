#include "Exception.h"
#include "Image.h"

Image::Image()
{
	m_Sx = 0;
	m_Sy = 0;
	m_Pixels = 0;
}

Image::Image(const Image& image)
{
	m_Sx = 0;
	m_Sy = 0;
	m_Pixels = 0;

	CopyFrom(image);
}

Image::~Image()
{
	SetSize(0, 0);
}

void Image::SetSize(int sx, int sy)
{
	m_Sx = 0;
	m_Sy = 0;
	delete[] m_Pixels;
	m_Pixels = 0;

	//

	if (sx * sy == 0)
		return;

	m_Sx = sx;
	m_Sy = sy;
	m_Pixels = new ImagePixel[sx * sy];
}

ImagePixel* Image::GetLine(int y)
{
	return m_Pixels + y * m_Sx;
}

const ImagePixel* Image::GetLine(int y) const
{
	return m_Pixels + y * m_Sx;
}

ImagePixel Image::GetPixel(int x, int y) const
{
	return GetLine(y)[x];
}

void Image::SetPixel(int x, int y, ImagePixel c)
{
	if (x < 0 || x > m_Sx - 1 || y < 0 || y > m_Sy - 1)
		return;

	GetLine(y)[x] = c;
}

void Image::Blit(Image* dst, int dstX, int dstY) const
{
	for (int y = 0; y < m_Sy; ++y)
		for (int x = 0; x < m_Sx; ++x)
			dst->SetPixel(dstX + x, dstY + y, GetLine(y)[x]);
}

void Image::Blit(Image* dst, int srcX, int srcY, int srcSx, int srcSy, int dstX, int dstY) const
{
	for (int y = 0; y < srcSy; ++y)
		for (int x = 0; x < srcSx; ++x)
			dst->SetPixel(dstX + x, dstY + y, GetLine(srcY + y)[srcX + x]);
}

void Image::RectFill(int x1, int y1, int x2, int y2, ImagePixel c)
{
	for (int y = y1; y <= y2; ++y)
	{
		ImagePixel* line = GetLine(y);

		for (int x = x1; x <= x2; ++x)
			line[x] = c;
	}
}

void Image::CopyFrom(const Image& image)
{
	SetSize(image.m_Sx, image.m_Sy);

	image.Blit(this, 0, 0);
}

void Image::DownscaleTo(Image& image, int scale) const
{
	if (scale <= 0)
		throw ExceptionVA("scale <= 0");
	
	if (scale == 1)
	{
		image.SetSize(m_Sx, m_Sy);
		
		Blit(&image, 0, 0);
	}
	else
	{
		int newSx = m_Sx / scale;
		int newSy = m_Sy / scale;
		
		if (newSx * scale != m_Sx)
			throw ExceptionVA("image size-x not a multiple of scale");
		if (newSy * scale != m_Sy)
			throw ExceptionVA("image size-y not a multiple of scale");
		
		image.SetSize(newSx, newSy);
		
		for (int y = 0; y < newSy; ++y)
		{
			for (int x = 0; x < newSx; ++x)
			{
				int rgba[4] = { 0, 0, 0, 0 };
				
				for (int y2 = y * scale; y2 < (y + 1) * scale; ++y2)
				{
					for (int x2 = x * scale; x2 < (x + 1) * scale; ++x2)
					{
						const ImagePixel src = GetPixel(x2, y2);
						
						rgba[0] += (int)src.rgba[0];
						rgba[1] += (int)src.rgba[1];
						rgba[2] += (int)src.rgba[2];
						rgba[3] += (int)src.rgba[3];
					}
				}
				
				ImagePixel dst;
				
				dst.rgba[0] = (ImageComponent)(rgba[0] / (scale * scale));
				dst.rgba[1] = (ImageComponent)(rgba[1] / (scale * scale));
				dst.rgba[2] = (ImageComponent)(rgba[2] / (scale * scale));
				dst.rgba[3] = (ImageComponent)(rgba[3] / (scale * scale));
				
				image.SetPixel(x, y, dst);
			}
		}
	}
}

void Image::DemultiplyAlpha()
{
	for (int y = 0; y < m_Sy; ++y)
	{
		ImagePixel* line = GetLine(y);
		
		for (int x = 0; x < m_Sx; ++x)
		{
			if (line[x].rgba[3] == 0)
				continue;
			
			for (int i = 0; i < 3; ++i)
			{
				int c = line[x].rgba[i] * 255 / line[x].rgba[3];
				
				if (c < 0)
					c = 0;
				else if (c > 255)
					c = 255;
				
				line[x].rgba[i] = c;
			}
		}
	}
}

Image& Image::operator=(const Image& image)
{
	CopyFrom(image);

	return *this;
}
