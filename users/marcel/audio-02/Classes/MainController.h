#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

@interface MainController : UIViewController {
	IBOutlet UIButton* btnPlaySystemSound;
	
	IBOutlet UILabel* lblStatus;
	
	AVAudioPlayer* m_AudioPlayer;
}

-(IBAction)touchPlaySoundSound:(id)sender;
-(IBAction)touchPlaySoundSoundCustom:(id)sender;
-(IBAction)touchPlayStreamHW:(id)sender;
-(IBAction)touchPlayStreamSW:(id)sender;
-(IBAction)touchPlayStreamSW_Note:(id)sender;//:(UIEvent*)withEvent;
-(IBAction)touchOpenAL:(id)sender;

-(IBAction)touchNote1:(id)sender;
-(IBAction)touchNote2:(id)sender;
-(IBAction)touchNote3:(id)sender;
-(IBAction)touchNote4:(id)sender;
-(IBAction)touchNote5:(id)sender;
-(IBAction)touchNote6:(id)sender;

-(void)testAudioPlayer;
-(void)testAudioStream;

@end
