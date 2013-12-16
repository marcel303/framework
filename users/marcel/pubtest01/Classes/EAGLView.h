#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import "ESRenderer.h"
#import "Mesh.h"
#import "TouchDLG.h"
#import "TouchMgr.h"

class AppState
{
public:
	AppState();
	
	void Update(float dt);
	void Render();
	
private:
	TouchDelegator mTouchDlg;
	
	enum TouchState
	{
		TouchState_None,
		TouchState_ColorCycle,
		TouchState_Rotate,
		TouchState_Zoom
	};
	
	TouchState mTouchState;
	float mZoom;
	Vec2F mZoomPosition[2];
	Mesh mMesh;
	Vec2F mTouchPosition[5];
	
	void DecideTouchState(int touchCount);
	
	static bool TouchBegin(void* obj, const TouchInfo& ti);
	static bool TouchEnd(void* obj, const TouchInfo& ti);
	static bool TouchMove(void* obj, const TouchInfo& ti);
	
public:
	static void HandleTouchBegin(void* obj, void* arg);
	static void HandleTouchEnd(void* obj, void* arg);
	static void HandleTouchMove(void* obj, void* arg);
};

extern AppState* gAppState;

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface EAGLView : UIView
{    
@private
	Renderer* renderer;
	
	BOOL animating;
	BOOL displayLinkSupported;
	NSInteger animationFrameInterval;
	// Use of the CADisplayLink class is the preferred method for controlling your animation timing.
	// CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
	// The NSTimer class is used only as fallback when running on a pre 3.1 device where CADisplayLink
	// isn't available.
	id displayLink;
    NSTimer *animationTimer;
	TouchMgr mTouchMgr;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

- (void) startAnimation;
- (void) stopAnimation;
- (void) drawView:(id)sender;

@end
