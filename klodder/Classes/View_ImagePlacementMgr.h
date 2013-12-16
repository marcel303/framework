#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "Types.h"
#import "ViewControllerBase.h"

@protocol ImagePlacementDelegate

-(void)placementFinished:(UIImage*)image dataLayer:(int)dataLayer transform:(BlitTransform*)transform controller:(View_ImagePlacementMgr*)controller;

@end

@interface View_ImagePlacementMgr : ViewControllerBase 
{
	UIImage* image;
	Vec2I size;
	int dataLayer;
	id<ImagePlacementDelegate> delegate;
	Vec2I location;
	//int angleIndex;
	float angle;
	float zoom;
	UISlider* slider;
}

@property (nonatomic, retain) UIImage* image;
@property (assign, readonly) int dataLayer;
@property (assign) Vec2I location;

-(id)initWithImage:(UIImage*)image size:(Vec2I)size dataLayer:(int)dalatLayer app:(AppDelegate*)app delegate:(id<ImagePlacementDelegate>)delegate;

-(void)handleRotateLeft;
-(void)handleRotateRight;
-(void)handleZoomChanged;
-(void)handleDone;
-(void)handleAdjustmentBegin;
-(void)handleAdjustmentEnd;

-(void)setLocation:(Vec2I)location;
-(float)angle;
-(float)scale;
-(Vec2I)snapPan;
-(CGAffineTransform)imageTransform;
-(CGAffineTransform)pivotTransform;
-(void)blitTransform:(BlitTransform&)out_Transform;

-(UIImage*)DBG_renderFinalImage;

@end
