#pragma once
#import <UIKit/UIView.h>
#import "ImageId.h"
#import "klodder_forward_objc.h"
#import "libgg_forward.h"

@interface View_Replay : UIView 
{
	AppDelegate* app;
	View_ReplayMgr* controller;
	ImageId imageId;
	bool replayActive;
	NSTimer* paintTimer;
	Stream* commandStream;
	int commandStreamPosition;
}

@property (nonatomic, assign) bool replayActive;

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)app controller:(View_ReplayMgr*)controller imageId:(ImageId)imageId;
-(void)replayBegin;
-(void)replayEnd;
-(void)paintTimerStart;
-(void)paintTimerStop;
-(void)paintTimerUpdate;
-(void)updatePaint;

@end
