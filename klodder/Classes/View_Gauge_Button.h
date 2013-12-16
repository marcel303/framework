#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"

@interface View_Gauge_Button : UIView 
{
	View_Gauge* gauge;
	int direction;
}

-(id)initWithFrame:(CGRect)frame gauge:(View_Gauge*)gauge direction:(int)direction;

@end
