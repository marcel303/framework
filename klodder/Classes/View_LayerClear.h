#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"

@interface View_LayerClear : UIView 
{
	IBOutlet View_LayerClearMgr* controller;
	
	IBOutlet UISlider* sliderR;
	IBOutlet UISlider* sliderG;
	IBOutlet UISlider* sliderB;
	IBOutlet UISlider* sliderA;
	
	IBOutlet View_LayerClearPreview* preview;
}

@property (nonatomic, assign) View_LayerClearMgr* controller;
@property (nonatomic, assign) UISlider* sliderR;
@property (nonatomic, assign) UISlider* sliderG;
@property (nonatomic, assign) UISlider* sliderB;
@property (nonatomic, assign) UISlider* sliderA;
@property (nonatomic, assign) View_LayerClearPreview* preview;

-(void)updateUi;
-(IBAction)handleColorChanged:(id)sender;

@end
