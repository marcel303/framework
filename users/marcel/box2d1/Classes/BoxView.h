#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <UIKit/UIKit.h>
#import "Forward.h"

@interface BoxView : UIView 
{
	EAGLContext* context;
	
	//
	
	GLuint frameBuffer;
	int frameBufferSx;
	int frameBufferSy;
	GLuint renderBuffer;
	
	//
	
	NSTimer* animationTimer;
	SpriteGfx* gfx;
	Sim* sim;
}

@property (retain) NSTimer* animationTimer;
@property (assign) Sim* sim;

-(void)render;

-(BOOL)createFramebuffer;
-(void)destroyFramebuffer;

-(void)startAnimation;
-(void)stopAnimation;

@end
