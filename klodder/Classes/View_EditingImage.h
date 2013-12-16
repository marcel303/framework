#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "TouchMgrV2.h"
#import "TouchZoom.h"
#import "Types.h"
#import "ViewBase.h"

// todo: add shortcut buttons for
// - color
// - undo
// - ??

@interface View_EditingImage : ViewBase <TouchListenerV2>
{
	@private
	
	AppDelegate* mAppDelegate;
	View_EditingMgr* controller;
	View_Editing* parent;
	
	TouchMgrV2 mTouchMgr;
	TouchZoom mTouchZoom;
	float mZoomStart;
	Vec2F mPanStart;
	Vec2F doubletapLocationScreen1;
	UITouch* doubletapLocationScreen1t;
	Vec2F doubletapLocationScreen1a;
	Vec2F doubletapLocationScreen1b;
	Vec2F doubletapLocationScreen2;
	int doubletapState;
	NSTimer* doubletapTimer;
	NSTimer* eyedropperTimer;
	NSTimer* transformTimer;
	RectF undoArea;
	RectF redoArea;
	RectF sizeArea;
	
	@public

	Vec2F pan;
	float zoom;
	bool mirrorX;
	
	Vec2F targetPan;
	float targetZoom;
}

@property (assign) Vec2F pan;
@property (readonly, assign) Vec2F targetPan;
//@property (assign) Vec2F focus;
@property (assign) float zoom;
@property (readonly, assign) float targetZoom;
@property (assign) bool mirrorX;

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app controller:(View_EditingMgr*)controller parent:(View_Editing*)parent;
-(void)handleFocus;
//-(void)handleFocusLost;

-(void)setPan_Direct:(Vec2F)pan;
-(void)setPan:(Vec2F)pan;
-(void)setPan:(Vec2F)pan animated:(BOOL)animated;
-(void)setZoom_Direct:(float)zoom;
-(void)setZoom:(float)zoom;
-(void)setZoom:(float)zoom animated:(BOOL)animated;
-(void)transformToCenter:(bool)animated;

-(void)touchBegin:(TouchInfoV2*)ti;
-(void)touchEnd:(TouchInfoV2*)ti;
-(void)touchMove:(TouchInfoV2*)ti;
-(void)handleTouchZoomStateChange:(TouchZoomEvent)e;
-(void)handleZoomChange:(TouchZoomEvent)e;
-(RectF)transformedBounds;
-(void)handleDoubleTap:(Vec2F)location;

-(void)swipe:(Vec2F)delta;

-(Vec2F)touchToView:(UITouch*)touch;
-(Vec2F)touchToScreen:(UITouch*)touch;
-(Vec2F)viewToImage:(Vec2F)location;
-(Vec2F)imageToView:(Vec2F)location;
-(Vec2F)viewToScreen:(Vec2F)location;
-(Vec2F)screenToView:(Vec2F)location;

-(void)updatePaint;
-(void)updateTransform;

// Double-tap behaviour
-(void)doubletapTimerBegin;
-(void)doubletapTimerEnd;
-(void)doubletapTimerElapsed;
-(void)doubletapFlush:(bool)includeEnd;

// Eyedropper behaviour
-(void)updateEyedropperLocationAndColor:(Vec2F)touchLocation;
-(void)eyedropperTimerBegin;
-(void)eyedropperTimerEnd;
-(void)eyedropperTimerElapsed;

-(void)handleTouchBegin:(UITouch*)touch location1:(Vec2F)location1 location2:(Vec2F)location2;
-(void)handleTouchEnd:(UITouch*)touch;

-(Vec2F)snapPan:(Vec2F)pan;
-(float)snapZoom:(float)zoom;

// Canvas pan/zoom animation
-(void)transformAnimationBegin;
-(void)transformAnimationEnd;
-(void)transformAnimationUpdate;
-(bool)transformAnimationCheck;

-(void)drawRect:(CGRect)rect withContext:(CGContextRef)ctx;

-(void)handleLayersChanged:(NSNotification*)notification;

@end
