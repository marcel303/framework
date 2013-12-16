#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"

#define IMG_GAUGE_BACK [UIImage imageNamed:@IMG("gauge_back")]

@interface View_Gauge_ActiveRegion : UIView 
{
	View_Gauge* gauge;
}

-(id)initWithFrame:(CGRect)frame gauge:(View_Gauge*)gauge;

@end
