#import <UIKit/UIKit.h>

@interface CrapButton : UIView 
{
	UIImageView* imageView;
	//declare a system sound id
	SystemSoundID soundID;
	NSTimer* timer;
}

-(void)show:(bool)enabled;
-(void)handleButton;
-(void)handlePop;

@end
