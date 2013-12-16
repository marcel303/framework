#import <UIKit/UIKit.h>
#import "TouchMgr.h"

@interface GameView : UIView <UIAccelerometerDelegate> {

	TouchMgr mTouchMgr;
	
	float mAcceleration[3];
}

@end
