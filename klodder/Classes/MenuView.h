#if 0

#import <UIKit/UIKit.h>
#import "klodder_forward_objc.h"

@interface MenuView : UIView 
{
	AppDelegate* mAppDelegate;
}

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app;
-(void)handleZoomIn;
-(void)handleZoomOut;
-(void)handleColorPicker;
-(void)handleEditing;

@end

#endif
