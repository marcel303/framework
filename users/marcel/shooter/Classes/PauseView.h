#import <UIKit/UIKit.h>

@interface PauseView : UIView <UIAlertViewDelegate>
{
	UIButton* resumeButton;
	UIButton* calibrateButton;
	UIButton* restartButton;
	UISlider* sensitivitySlider;
	
	UIAlertView* restartAlert;
}

-(void)updateUi;

-(void)handleResume:(id)sender;
-(void)handleCalibrate:(id)sender;
-(void)handleRestart:(id)sender;
-(void)handleTiltSensitivityChanged:(id)sender;

// UIAlertViewDelegate
-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;

@end
