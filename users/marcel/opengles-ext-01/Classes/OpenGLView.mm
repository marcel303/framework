//
//  OpenGLView.m
//  opengles-ext-01
//
//  Created by Marcel Smit on 12-05-09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import "OpenGLView.h"
#import "TexturePVR.h"

@implementation OpenGLView

static GLuint g_Texture = 0;

#define CheckError() DoCheckError(__FUNCTION__, __LINE__)

static void DoCheckError(const char* func, int line)
{
	GLenum error = glGetError();
	
	if (error == GL_NO_ERROR)
		return;
	
	NSString* errorString;
	
	switch (error)
	{
		case GL_INVALID_ENUM:
			errorString = @"OpenGL: Invalid enum";
			break;
			
		case GL_INVALID_VALUE:
			errorString = @"OpenGL: Invalid value";
			break;
			
		case GL_INVALID_OPERATION:
			errorString = @"OpenGL: Invalid operation";
			break;
			
		default:
			errorString = @"OpenGL: Unknown error";
			break;
	}
	
	NSString* logString = [NSString stringWithFormat:@"%@: %d: %@", [NSString stringWithCString:func], line, errorString];
	
	NSLog(logString);
}
	
-(id)initWithFrame:(CGRect)frame
{
	if (self = [super initWithFrame:frame])
	{
		CAEAGLLayer* layer = (CAEAGLLayer*)self.layer;
		
		layer.opaque = YES;
		layer.drawableProperties =
			[NSDictionary dictionaryWithObjectsAndKeys:
	//				[NSNumber numberWithBool:NO],
				[NSNumber numberWithBool:YES],
				kEAGLDrawablePropertyRetainedBacking,
				kEAGLColorFormatRGBA8,
				kEAGLDrawablePropertyColorFormat,
				nil];
		
		glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
		
		if (!glContext)
		{
			[self release];
			return nil;
		}
		
		if (![EAGLContext setCurrentContext:glContext])
		{
			[self release];
			return nil;
		}
		
		NSLog(@"OpenGL: Vendor: %s", glGetString(GL_VENDOR));
		NSLog(@"OpenGL: Renderer: %s", glGetString(GL_RENDERER));
		NSLog(@"OpenGL: Version: %s", glGetString(GL_VERSION));
		NSLog(@"OpenGL: Extensions: %s", glGetString(GL_EXTENSIONS));
		
		[self loadTextures_BySize];
		[self loadTextures_ByFormatRegular];
		[self loadTextures_ByFormatPVRT];
		
		[NSTimer scheduledTimerWithTimeInterval:0.01f target:self selector:@selector(handleTimer) userInfo:nil repeats:YES];
	}

	return self;
}

-(void)layoutSubviews
{
	[EAGLContext setCurrentContext:glContext];
	[self destroyFrameBuffer];
	[self createFrameBuffer];
	[self drawView];
}

-(BOOL)createFrameBuffer
{
	glGenFramebuffersOES(1, &frameBuffer);
	CheckError();
	glGenRenderbuffersOES(1, &renderBufferColor);
	CheckError();
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, frameBuffer);
	CheckError();
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, renderBufferColor);
	CheckError();
	[glContext renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)self.layer];
	CheckError();
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, renderBufferColor);
	CheckError();
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &bbSx);
	CheckError();
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &bbSy);
	CheckError();
	
	glGenRenderbuffersOES(1, &renderBufferDepth);
	CheckError();
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, renderBufferDepth);
	CheckError();
	glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, bbSx, bbSy);
	CheckError();
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, renderBufferDepth);
	CheckError();
	
	if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
	{
		NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		return NO;
	}
	
	return YES;
}

-(BOOL)destroyFrameBuffer
{  
	glDeleteRenderbuffersOES(1, &renderBufferColor);
	CheckError();
	renderBufferColor = 0;
	
	glDeleteRenderbuffersOES(1, &renderBufferDepth);
	CheckError();
	renderBufferDepth = 0;
	
	glDeleteFramebuffersOES(1, &frameBuffer);
	CheckError();
	frameBuffer = 0;
	
	return YES;
}

static void TestSkinning()
{
	glMatrixMode(GL_MATRIX_PALETTE_OES);
	CheckError();

	for (int i = 0; i < 4; ++i)
	{
		glCurrentPaletteMatrixOES(i);
		CheckError();
		
		glLoadIdentity();
		static float angle = 0.0f;
		glRotatef(angle * i, 0.0f, 0.0f, 1.0f);
//		glTranslatef(angle, 0.0f, 0.0f);
		angle += 1.0f;
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	GLfloat vertices[] =
	{
		-0.2f, -0.2f,
		+0.2f, -0.2f,
		-0.2f, +0.2f,
		+0.2f, +0.2f
	};
	
	GLubyte indices[] =
	{
		0, 1,
		0, 1,
		0, 1,
		0, 1
	};
	
	GLfloat weights[] =
	{
		0.5f, 0.5f,
		0.5f, 0.5f,
		0.5f, 0.5f,
		0.5f, 0.5f
	};
	
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	CheckError();
	glEnableClientState(GL_VERTEX_ARRAY);
	CheckError();
	glMatrixIndexPointerOES(2, GL_UNSIGNED_BYTE, 0, indices);
	CheckError();
	glEnableClientState(GL_MATRIX_INDEX_ARRAY_OES);
	CheckError();
	glWeightPointerOES(2, GL_FLOAT, 0, weights);
	CheckError();
	glEnableClientState(GL_WEIGHT_ARRAY_OES);
	CheckError();
	
	glEnable(GL_MATRIX_PALETTE_OES);
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	CheckError();
	
	glDisable(GL_MATRIX_PALETTE_OES);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	CheckError();
	glDisableClientState(GL_WEIGHT_ARRAY_OES);
	CheckError();
	glDisableClientState(GL_MATRIX_INDEX_ARRAY_OES);
	CheckError();
}

-(void)drawView
{
	const GLfloat squareVertices[] =
	{
		-0.5f, -0.5f,
		0.5f,  -0.5f,
		-0.5f,  0.5f,
		0.5f,   0.5f,
	};
	const GLubyte squareColors[] =
	{
		255, 255,   0, 255,
		0,   255, 255, 255,
		0,     0,   0,   0,
		255,   0, 255, 255,
	};
	const GLfloat textureCoords[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};
    
	[EAGLContext setCurrentContext:glContext];
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, frameBuffer);
	CheckError();
	glViewport(0, 0, bbSx, bbSy);
	CheckError();
	
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	CheckError();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(-1.0f, 1.0f, -1.5f, 1.5f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	CheckError();
    
	glVertexPointer(2, GL_FLOAT, 0, squareVertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	CheckError();
	if (g_Texture == 0)
	{
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, squareColors);
		glEnableClientState(GL_COLOR_ARRAY);
		CheckError();
	}
	glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	CheckError();
	
#if 1
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glScalef(10.0f, 10.0f, 10.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(0.0f, 0.0f, 0.0f, 0.02f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
	CheckError();
#endif
	
	static float angle = 0.0f;
	glRotatef(angle, 0.0f, 0.0f, 1.0f);
	glRotatef(angle / 1.111f, 0.0f, 1.0f, 0.0f);
	glRotatef(angle / 1.222f, 1.0f, 0.0f, 0.0f);
	angle += 4.0f;
	float scale = cos(angle / 360.0f) + 1.0f;
	glScalef(scale, scale, scale);
	CheckError();
	
	glBindTexture(GL_TEXTURE_2D, g_Texture);
	glEnable(GL_TEXTURE_2D);
	CheckError();

#if 1
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	CheckError();

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	CheckError();
	
	glDisable(GL_BLEND);
#endif
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	CheckError();
	
#if 1 // point sprites
	glDisable(GL_TEXTURE_2D);
	glVertexPointer(2, GL_FLOAT, 0, squareVertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	GLfloat pointSizes[] = { 8.0f, 4.0f, 2.0f, 1.0f };
	glPointSizePointerOES(GL_FLOAT, 0, pointSizes);
	glEnableClientState(GL_POINT_SIZE_ARRAY_OES);
	glEnable(GL_POINT_SPRITE_OES);
	glTexEnvi(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
	glDrawArrays(GL_POINTS, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_POINT_SIZE_ARRAY_OES);
#endif
	
#if 1 // matrix palette skinning
	TestSkinning();
#endif
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, 320.0f, 480.0f, 0.0f, 0.1f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	CheckError();
	
#if 1 // draw tex
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	CheckError();
	
	int s = 64;
	GLint rect[] = { 0, 0, s, s };
	
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, rect);
	CheckError();
	glDrawTexfOES(100.0f, 100.0f, 0.0f, s, s);
//	glDrawTexfOES(0.0f, 0.0f, 0.5f, 1.0f, 1.0f);
	CheckError();
#endif
    
	glDisable(GL_TEXTURE_2D);
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, renderBufferColor);
	[glContext presentRenderbuffer:GL_RENDERBUFFER_OES];
}

-(void)handleTimer
{
	[self drawView];
}

static NSString* MakeBundleFileName(NSString* fileName)
{
	NSString* name = [fileName stringByDeletingPathExtension];
	NSString* extension = [fileName pathExtension];
	
	return [[NSBundle mainBundle] pathForResource:name ofType:extension];
}

static GLuint LoadTexture(NSString* fileName, GLuint format, bool isCompressed)
{
	// Fix file name.
	
	fileName = MakeBundleFileName(fileName);
	
	// Load texture data.
	
	GLuint result = 0;
	
	if (isCompressed)
	{
		NSData* data = [NSData dataWithContentsOfFile:fileName];
		
		TexturePVR texture;
		
		if (!texture.Load((uint8_t*)[data bytes]))
		{
			// todo: log.
			
			return 0;
		}
		
		// Create texture.
		
		glGenTextures(1, &result);
		glBindTexture(GL_TEXTURE_2D, result);
		CheckError();
		
		// Cycle through available MIP maps and load them to the GPU.
		// todo: does the PVR extension allow uploading all MIP map levels in one shot?
		
		for (int i = 0; i < texture.m_Levels.size(); ++i)
		{
	        glCompressedTexImage2D(
				GL_TEXTURE_2D,
				i,
				format,
				texture.m_Levels[i]->m_Sx,
				texture.m_Levels[i]->m_Sy,
				0,
				texture.m_Levels[i]->m_DataSize,
				texture.m_Levels[i]->m_Data);
			CheckError();
		}
	}
	else
	{
		// Load PNG through UIImage.
		
		UIImage* uiImage = [UIImage imageWithContentsOfFile:fileName];
		CGImageRef cgImage = uiImage.CGImage;
		
		// Calculate and create storage.
		
		int sx = CGImageGetWidth(cgImage);
		int sy = CGImageGetHeight(cgImage);
		
		uint8_t* bytes = new uint8_t[sx * sy * 4];
		
#if 0
		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				{
					int index = (x + y * sx) * 4;
					
					float alpha = (sin((x + y * y / 10.0) / 10.0) + 1.0f) / 2.0f;
					
					bytes[index + 0] = (x + 0) & 255;
					bytes[index + 1] = (0 + y) & 255;
					bytes[index + 2] = (x + y) & 255;
					bytes[index + 3] = alpha * 255.0f;
				}
			}
		}
#endif
		
		// Use a CG context to blit image into our array of bytes.
		
		CGContextRef context = CGBitmapContextCreate(bytes, sx, sy, 8, sx * 4, CGImageGetColorSpace(cgImage), kCGImageAlphaPremultipliedLast);
		CGContextDrawImage(context, CGRectMake(0.0, 0.0, (CGFloat)sx, (CGFloat)sy), cgImage);
		CGContextRelease(context);
		
		// Create texture.
		
		glGenTextures(1, &result);
		glBindTexture(GL_TEXTURE_2D, result);
		CheckError();
		
		// Upload texture.
		
		glTexImage2D(GL_TEXTURE_2D, 0, format, sx, sy, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
		CheckError();
		
		delete[] bytes;
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	CheckError();
	
	return result;
}

-(void)loadTextures_BySize
{
	textureSize_P1Sq1 = LoadTexture(@"texture_size_p1sq1.png", GL_RGBA, false);
	textureSize_P1Sq0 = LoadTexture(@"texture_size_p1sq0.png", GL_RGBA, false);
	textureSize_P0Sq1 = LoadTexture(@"texture_size_p0sq1.png", GL_RGBA, false);
	textureSize_P0Sq0 = LoadTexture(@"texture_size_p0sq0.png", GL_RGBA, false);
	
//	g_Texture = textureSize_P1Sq1;
	g_Texture = textureSize_P1Sq0;
//	g_Texture = textureSize_P0Sq1;
//	g_Texture = textureSize_P0Sq0;
}

-(void)loadTextures_ByFormatRegular
{
	textureFormat_RGB8 = LoadTexture(@"texture_regular_rgb.png", GL_RGB, false);
	textureFormat_RGBA8 = LoadTexture(@"texture_regular_rgba.png", GL_RGBA, false);
	
	//g_Texture = textureFormat_RGB8;
}

-(void)loadTextures_ByFormatPVRT
{
#if 1
	textureFormat_PVRT_RGB2 = LoadTexture(@"texture_pvr_rgb2.pvr", GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG, true);
	textureFormat_PVRT_RGB4 = LoadTexture(@"texture_pvr_rgb4.pvr", GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG, true);
	textureFormat_PVRT_RGBA2 = LoadTexture(@"texture_pvr_rgba2.pvr", GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, true);
	textureFormat_PVRT_RGBA4 = LoadTexture(@"texture_pvr_rgba4.pvr", GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, true);
#endif
	
//	g_Texture = textureFormat_PVRT_RGB4;
}

+(Class)layerClass
{
	return [CAEAGLLayer class];
}

-(void)dealloc
{
	if ([EAGLContext currentContext] == glContext)
	{
		[EAGLContext setCurrentContext:nil];
	}
	
	[glContext release];  
	
	[super dealloc];
}

@end
