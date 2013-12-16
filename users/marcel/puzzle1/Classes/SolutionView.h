#import <UIKit/UIKit.h>

@interface SolutionView : UIView 
{
	UIImage* image;
	NSTimer* animationTimer;
	float animationCounter;
}

-(id)initWithFrame:(CGRect)frame image:(UIImage*)image;
-(void)animationBegin;
-(void)animationEnd;
-(void)animationUpdate;

@end
