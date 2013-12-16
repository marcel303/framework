#import <UIKit/UIKit.h>
#import "Application.h"
#import "ToolView.h"

@interface PaintView : UIView
{
//	CGContextRef ctxRef;
//	CGImageRef imageRef;
//	uint8_t* bytes;
	
	NSTimer* timer;
	
	UIImageView* imageView;
	
	Paint::Application* application;
	
	ToolView* toolView; // todo: move to AppView or smt.
	UIButton* toolButton;
}

- (void)loadImage:(UIImage*)image;
- (void)emitDrawArea;

@end
