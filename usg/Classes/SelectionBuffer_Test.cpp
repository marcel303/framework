#include <string.h>
#include "Hash.h"
#include "OpenGLCompat.h"
#include "OpenGLUtil.h"
#include "SelectionBuffer_Test.h"

void SelectionBuffer_ToTexture(SelectionBuffer* buffer, int offsetX, int offsetY, int textureSx, int textureSy)
{
	const int area = textureSx * textureSy;
	
	GLubyte* bytes = new GLubyte[area * 4];
	
	memset(bytes, 0x00, area * 4);
	
	int sx = buffer->Sx_get() - offsetX;
	int sy = buffer->Sy_get() - offsetY;
	
	if (sx > textureSx)
		sx = textureSx;
	if (sy > textureSy)
		sy = textureSy;
	
	for (int y = 0; y < sy; ++y)
	{
#if 0
		const CD_TYPE* sline = buffer->GetLine(offsetY + y) + offsetX;
		GLubyte* dline = bytes + y * textureSx * 4;
		
		for (int x = 0; x < sx; ++x)
		{
			if (*sline)
			{
				for (int i = 0; i < 4; ++i)
					dline[i] = 255;
			}
			else
			{
				for (int i = 0; i < 4; ++i)
					dline[i] = 0;
			}
			
			sline++;
			dline += 4;
		}
#else
		const int count = buffer->DBG_GetSBuffer()->DBG_GetSpanCount(y);
		
		const SelectionSpan* span = buffer->DBG_GetSBuffer()->DBG_GetSpanRoot(y);
		
		for (int i = 0; i < count; ++i, span = buffer->DBG_GetSBuffer()->DBG_GetNextSpan(span))
		{
			int x1 = span->x1;
			int x2 = span->x2;
			
			if (x1 < 0)
				x1 = 0;
			if (x2 > textureSx - 1)
				x2 = textureSx - 1;

			Hash hash = HashFunc::Hash_FNV1(&span->index, sizeof(CD_TYPE));
			
			GLubyte c[4] = { hash >> 0, hash >> 8, hash >> 16, 255 };
			
			GLubyte* dline = bytes + (x1 + y * textureSx) * 4;
			
			for (int x = x1; x <= x2; ++x)
			{
				dline[0] = c[0];
				dline[1] = c[1];
				dline[2] = c[2];
				dline[3] = c[3];
				
				dline += 4;
			}
		}
#endif
	}
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSx, textureSy, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
	GL_CHECKERROR();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GL_CHECKERROR();
	
	delete[] bytes;
}
