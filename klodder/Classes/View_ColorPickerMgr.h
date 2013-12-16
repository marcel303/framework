#import "ColorPickerState.h"
#import "ViewControllerBase.h"

@interface View_ColorPickerMgr : ViewControllerBase <PickerDelegate>
{
}

-(id)initWithApp:(AppDelegate *)app;
-(void)handleDone;
-(void)applyColor;

// PickerDelegate
-(PickerState*)colorPickerState;
-(void)colorChanged;
-(void)openingSwatches;

@end
