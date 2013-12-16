#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "TouchMgrV2.h"
#import "Types.h"
#import "ViewBase.h"

@interface View_Editing : ViewBase <UIScrollViewDelegate>
{
	@private
	
	AppDelegate* app;
	View_EditingMgr* controller;
	View_MagicButton* magicButton;
	UIView* activityIndicatorBack;
	UIActivityIndicatorView* activityIndicator;
	
	@public
	
	View_EditingImage* image;
	View_EditingImageBorder* imageBorder;
	View_Eyedropper* eyeDropper;
	View_MiniBrushSizeContainer* miniBrushSize;
	View_ZoomInfo* zoomIndicator;
	View_BrushRetina* brushRetina;
	bool brushRetinaVisible;
	float brushRetinaRadius;
	
	BOOL eyeDropperVisible;
}

@property (nonatomic, retain) View_EditingImage* image;
@property (nonatomic, retain) View_Eyedropper* eyeDropper;
@property (nonatomic, retain) View_ZoomInfo* zoomIndicator;

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app controller:(View_EditingMgr*)controller;
-(void)handleFocus;
-(void)handleFocusLost;
-(void)setEyeDropperVisible:(BOOL)visible;
-(void)setEyedropperLocation:(CGPoint)location color:(UIColor*)color;
-(void)setMagicMenuVisible:(BOOL)visible;
-(void)updateImageBorder;
-(void)showMiniBrushSize;
-(void)zoomIndicatorShow;
-(void)zoomIndicatorHide:(BOOL)animated;
-(void)brushRetinaShow:(Vec2F)location zoom:(float)zoom;
-(void)brushRetinaHide;
-(void)brushRetinaUpdate:(Vec2F)location zoom:(float)zoom;
-(void)activityIndicatorShow;
-(void)activityIndicatorHide;

@end
