#import <UIKit/UIView.h>
#import "Bitmap.h"

@interface View_BarButtonItem_Color : UIView
{
	@private
	
	id delegate;
	SEL action;
	Rgba color;
}

-(id)initWithDelegate:(id)delegate action:(SEL)action color:(Rgba)color opacity:(float)opacity;

@end
