#import <UIKit/UIView.h>

@interface View_EditingImageBorder : UIView 
{
	UIImageView* images[3][3];
}

-(void)updateLayers:(RectF)rect;

@end
