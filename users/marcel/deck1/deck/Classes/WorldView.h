#import <UIKit/UIKit.h>
#import "libiphone_forward.h"

@interface WorldView : UIView 
{
	NSTimer* updateTimer;
	OpenGLState* gl;
}

-(void)updateBegin;
-(void)updateEnd;
-(void)updateElapsed;

@end
