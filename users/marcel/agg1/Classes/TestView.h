#import <UIKit/UIKit.h>
#import "klodder_forward.h"
#import "Resample.h"

@interface TestView : UIView 
{
	MacImage* srcImage;
	MacImage* dstImage;
}

-(void)update;

@end
