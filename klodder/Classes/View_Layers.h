#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "ViewBase.h"

@interface View_Layers : ViewBase
{
	AppDelegate* app;
	View_LayersMgr* controller;
	View_LayerManager* layerMgr;
	View_Gauge* opacityGauge;
	
	UIToolbar* toolBar;
	UINavigationBar* navigationBar;
}

@property (assign) View_LayerManager* layerMgr;
@property (nonatomic, retain) UIToolbar* toolBar;
@property (nonatomic, retain) UINavigationBar* navigationBar;

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app controller:(View_LayersMgr*)controller;
-(void)handleFocus;
-(void)handleBack;
-(void)updateLayerOrder;
-(void)updatePreviewPictures;
-(void)updateUi;
-(void)handleOpacityChanged:(NSNumber*)value;

@end
