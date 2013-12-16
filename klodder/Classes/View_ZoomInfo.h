#import <UIKit/UIView.h>

@interface View_ZoomInfo : UIView 
{
	UILabel* label;
	float zoom;
}

@property (assign) float zoom;

-(void)setZoom:(float)zoom;

@end
