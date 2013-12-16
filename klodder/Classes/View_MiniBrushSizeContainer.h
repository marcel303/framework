#import <UIKit/UIKit.h>
#import "klodder_forward_objc.h"

@interface View_MiniBrushSizeContainer : UIView 
{
	View_MiniBrushSize* brushSize;
}

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)app;

-(void)show;
-(void)hide;

@end
