#import "klodder_forward_objc.h"
#import "View_MagicButton.h"
#import "ViewControllerBase.h"

@interface View_EditingMgr : ViewControllerBase <MagicDelegate, UIPopoverControllerDelegate>
{
	float lastZoom;
	NSTimer* menuTimer;
	bool doneRequested;

	UIBarButtonItem* item_Color;
	UIBarButtonItem* item_Tool;
	UIBarButtonItem* item_Layers;
	
#ifdef IPAD
	UIPopoverController* popoverController;
#endif
}

-(id)initWithApp:(AppDelegate*)app;

-(void)handleColorPicker;
-(void)handleToolSettings;
-(void)handleLayers;
-(void)handleUndo;
-(void)handleRedo;
-(void)handleDoneAnimated;
-(void)handleDone;
-(void)saveBegin;
-(void)saveEnd;
-(void)handleBrushSize;
-(void)handleZoomToggle:(Vec2F)pan;
-(void)handleImageAnimationEnd;

-(void)newPainting;

-(View_Editing*)getView;
-(View_EditingImage*)getImageView;

-(void)setEyedropperEnabled:(BOOL)enabled;
-(void)setEyedropperLocation:(CGPoint)location color:(UIColor*)color;

-(void)setMagicMenuEnabled:(BOOL)enabled;
-(void)handleMagicAction:(MagicAction)action;

-(UIImage*)provideMagicImage:(MagicAction)action;
-(void)showMenu:(BOOL)visible;
-(void)hideMenu;
-(void)menuHideStart;
-(void)menuHideStop;

-(void)updateUi;

// UIPopoverControllerDelegate
-(void)popoverControllerDidDismissPopover:(UIPopoverController*)popoverController;
-(BOOL)popoverControllerShouldDismissPopover:(UIPopoverController*)popoverController;

-(void)DBG_handleTestBezierTraveller;

@end
