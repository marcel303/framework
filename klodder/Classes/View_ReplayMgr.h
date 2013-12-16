#pragma once
#import <UIKit/UIView.h>
#import "ImageId.h"
#import "ViewControllerBase.h"

@interface View_ReplayMgr : ViewControllerBase
{
	ImageId imageId;
	bool paused;
}

-(id)initWithApp:(AppDelegate *)app imageId:(ImageId)imageId;
-(void)handleBack;
-(void)handleRestart;
-(void)pause;
-(void)resume;
-(void)start;
-(void)stop;

@end
