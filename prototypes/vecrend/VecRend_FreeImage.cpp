#include "Precompiled.h"
#include <FreeImage.h>
#include <string>
#include "Buffer.h"
#include "Image.h"
#include "Types.h"
#include "VecRend_FreeImage.h"

void VecRend_Export(const Buffer* buffer, const char* fileName, IImageLoader* loader)
{
	Image image;
	
	image.SetSize(buffer->m_Sx, buffer->m_Sy);
	
	for (int y = 0; y < buffer->m_Sy; ++y)
	{
		const float* sline = buffer->GetLine(y);
		ImagePixel* dline = image.GetLine(y);
		
		for (int x = 0; x < buffer->m_Sx; ++x)
		{
			dline->r = (int)(sline[0] * 255.0f);
			dline->g = (int)(sline[1] * 255.0f);
			dline->b = (int)(sline[2] * 255.0f);
			dline->a = (int)(sline[3] * 255.0f);
			
			sline += 4;
			//dline += 4;
			dline++;
		}
	}
	
	loader->Save(image, fileName);
}
