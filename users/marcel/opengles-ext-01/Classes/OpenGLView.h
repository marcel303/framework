//
//  OpenGLView.h
//  opengles-ext-01
//
//  Created by Marcel Smit on 12-05-09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <UIKit/UIKit.h>

@interface OpenGLView : UIView
{
@private
	EAGLContext* glContext;
	
	int bbSx;
	int bbSy;

	GLuint renderBufferColor;
	GLuint renderBufferDepth;
	GLuint frameBuffer;
	
	// Textures of varying sizes.
	GLuint textureSize_P1Sq1; // Square power of 2 sized texture.
	GLuint textureSize_P1Sq0; // Non square power of 2 sized texture.
	GLuint textureSize_P0Sq1; // Square non power of 2 sized texture.
	GLuint textureSize_P0Sq0; // Non square non power of 2 sized texture.
	// TODO: Implement texture loading.
	// TODO: Implement 3D texturing tests for each texture size.
	// TODO: Implement 2D texturing tests (glDrawText) for each texture size.
	// TODO: Test MIP map generation.
	// glDrawTexfOES (GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height);
	// glDrawTexfvOES (const GLfloat *coords);
	// GL_TEXTURE_CROP_RECT_OES
	// GL_GENERATE_MIPMAP

	// Texture formats: Regular texture formats.
	GLuint textureFormat_RGB8;
	GLuint textureFormat_RGBA8;
	// TODO: Implement RGB texture loading.
	// TODO: Implement RGBA texture loading.
	
	// Texture formats: PowerVR compressed texture formats.
	GLuint textureFormat_PVRT_RGB2;
	GLuint textureFormat_PVRT_RGB4;
	GLuint textureFormat_PVRT_RGBA2;
	GLuint textureFormat_PVRT_RGBA4;
	// TODO: Implement 2BPP RGB texture loading.
	// TODO: Implement 4BPP RGB texture loading.
	// TODO: Implement 2BPP RGBA texture loading.
	// TODO: Implement 4BPP RGBA texture loading.

	// Matrix palette skinning.
	// - mesh, matrices, matrix palette, weights, indices.
	// TODO: Implement skinning test.
	// glCurrentPaletteMatrixOES (GLuint matrixpaletteindex);
	// glLoadPaletteFromModelViewMatrixOES (void);
	// glMatrixIndexPointerOES (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	// glWeightPointerOES (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	// GL_MAX_VERTEX_UNITS_OES
	// GL_MAX_PALETTE_MATRICES_OES
	// GL_MATRIX_PALETTE_OES
	// GL_MATRIX_INDEX_ARRAY_OES
	// GL_WEIGHT_ARRAY_OES
	// GL_CURRENT_PALETTE_MATRIX_OES
	
	// Point rendering.
	// - point array, point properties array.
	// TODO: Implement point rendering using point (property) arrays.
	// glPointSizePointerOES (GLenum type, GLsizei stride, const GLvoid *pointer);
	// glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
	// GL_POINT_SIZE_ARRAY_OES
	
	// TODO: Map buffer test (?)
	
	// Misc:
	// - Test sciccor functions.
	// - Test dithering (quality, speed).
	// - Test line smoothing (quality, speed).
	// - Test multi sampling.
	// - Test static versus dynamic buffer usage.
	
}

-(BOOL)createFrameBuffer;
-(BOOL)destroyFrameBuffer;
-(void)drawView;
-(void)handleTimer;
-(void)loadTextures_BySize;
-(void)loadTextures_ByFormatRegular;
-(void)loadTextures_ByFormatPVRT;

@end
