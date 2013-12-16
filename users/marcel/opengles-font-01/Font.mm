#import <UIKit/UIKit.h>
#import "Font.h"

void FontHelper::DrawString(GLuint& texture, char* text)
{
	if (texture != 0)
		glDeleteTextures(1, &texture);
	
	const int texture_sx = 128;
	const int texture_sy = 128;
	const int byteCount = texture_sx * texture_sy;
	
	uint8* bytes = (uint8*)malloc(byteCount);
	memset(bytes, 0x00, byteCount);
	
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
	CGContextRef ctx = CGBitmapContextCreate(bytes, texture_sx, texture_sy, 8, texture_sx * 1, colorSpace, kCGImageAlphaNone);
	CGColorSpaceRelease(colorSpace);
	
	CGContextSetGrayFillColor(ctx, 1.0f, 1.0f);
	CGContextTranslateCTM(ctx, 0.0, texture_sy);
	CGContextScaleCTM(ctx, 1.0, -1.0);
	
	UIGraphicsPushContext(ctx);
	{
		UIFont* font = [UIFont systemFontOfSize:16.0f];
		
		NSString* string = [NSString stringWithCString:text];
		
		[string drawInRect:CGRectMake(0, 0, texture_sx, texture_sy)
			withFont:font
			lineBreakMode:UILineBreakModeWordWrap
			alignment:UITextAlignmentLeft];
		
		[string release];
	}
	UIGraphicsPopContext();
	
	CGContextRelease(ctx);

	// Expand into RGBA.
	// TODO: Use alpha-based rendering for text, instead of expanding to RGBA.
	
	uint8* temp = (uint8*)malloc(texture_sx * texture_sy * 4);
	
	int index1 = 0;
	int index2 = 0;
	
	for (int y = 0; y < texture_sy; ++y)
	{
		for (int x = 0; x < texture_sx; ++x)
		{
			temp[index1++] = bytes[index2];
			temp[index1++] = bytes[index2];
			temp[index1++] = bytes[index2];
			temp[index1++] = 255;
			index2++;
		}
	}
	
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_sx, texture_sy, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_sx, texture_sy, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	free(temp);
	free(bytes);
}
