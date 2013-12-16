#import <UIKit/UIKit.h>

@interface AppViewMgr : UIViewController 
{
	UILabel* livesLabel;
	UILabel* scoreLabel;
	UILabel* moneyLabel;
}

@property (nonatomic, retain) IBOutlet UILabel* livesLabel;
@property (nonatomic, retain) IBOutlet UILabel* scoreLabel;
@property (nonatomic, retain) IBOutlet UILabel* moneyLabel;

-(IBAction)handleTowerSelectA:(id)sender;
-(IBAction)handleTowerSelectB:(id)sender;
-(IBAction)handleTowerSelectC:(id)sender;
-(IBAction)handleTowerSelectD:(id)sender;

@end
