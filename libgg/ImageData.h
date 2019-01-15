#pragma once

typedef short ImageComponent;

class ImagePixel
{
public:
	inline ImagePixel()
	{
		r = g = b = a = 0;
	}

	union
	{
		struct
		{
			ImageComponent r;
			ImageComponent g;
			ImageComponent b;
			ImageComponent a;
		};
		ImageComponent rgba[4];
	};
};

class Image
{
public:
	Image();
	Image(const Image& image);
	~Image();

	void SetSize(int sx, int sy);

	ImagePixel* GetLine(int y);
	const ImagePixel* GetLine(int y) const;
	ImagePixel GetPixel(int x, int y) const;
	void SetPixel(int x, int y, ImagePixel c);
	void Blit(Image* dst, int dstX, int dstY) const;
	void Blit(Image* dst, int srcX, int srcY, int srcSx, int srcSy, int dstX, int dstY) const;
	void RectFill(int x1, int y1, int x2, int y2, ImagePixel c);
	void CopyFrom(const Image& image);
	void DownscaleTo(Image& image, int scale) const;
	void DemultiplyAlpha();

	Image& operator=(const Image& image);

	int m_Sx;
	int m_Sy;
	ImagePixel* m_Pixels;
};
