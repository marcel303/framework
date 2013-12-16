#import <UIKit/UIKit.h>
#import "Types.h"

@interface MandelbrotView : UIView 
{
	float zoom;
	Vec2F pan;
}

-(RectF)displayRect;

@end
