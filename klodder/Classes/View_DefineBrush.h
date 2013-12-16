#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "ViewBase.h"

@interface View_DefineBrush : ViewBase
{
	AppDelegate* mAppDelegate;

	// fixme
	UIImage* mBack;
	UIImage* mCrosshair[3];
	
	CGImageRef mImage;
	Vec2I mOffset;
	
	int size;
	Vec2I location;
}

@property (assign) int size;
@property (assign) Vec2I location;

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app;
-(void)handleFocus;
-(void)handleFocusLost;

-(void)sizeSmall;
-(void)sizeMedium;
-(void)sizeLarge;
-(void)save;
-(void)cancel;

-(void)setSize:(int)size;
-(void)setLocation:(Vec2I)location;
-(void)clampLocation;
-(RectI)calcRect;

@end
