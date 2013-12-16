#import <UIKit/UIKit.h>

@interface AppViewMgr : UIViewController 
{
	NSString* path;
	UILabel* livesLabel;
	UILabel* scoreLabel;
	UILabel* moneyLabel;
	UILabel* levelLabel;
	UIButton* towerAButton;
	UIButton* towerBButton;
	UIButton* towerCButton;
	UIButton* towerDButton;
	UIButton* towerSellButton;
	UIButton* towerUpgradeButton;
}

@property (nonatomic, retain) IBOutlet UILabel* livesLabel;
@property (nonatomic, retain) IBOutlet UILabel* scoreLabel;
@property (nonatomic, retain) IBOutlet UILabel* moneyLabel;
@property (nonatomic, retain) IBOutlet UILabel* levelLabel;
@property (nonatomic, retain) IBOutlet UIButton* towerAButton;
@property (nonatomic, retain) IBOutlet UIButton* towerBButton;
@property (nonatomic, retain) IBOutlet UIButton* towerCButton;
@property (nonatomic, retain) IBOutlet UIButton* towerDButton;
@property (nonatomic, retain) IBOutlet UIButton* towerSellButton;
@property (nonatomic, retain) IBOutlet UIButton* towerUpgradeButton;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil level:(NSString*)path;
-(void)updateUi;

-(IBAction)handleTowerSelectA:(id)sender;
-(IBAction)handleTowerSelectB:(id)sender;
-(IBAction)handleTowerSelectC:(id)sender;
-(IBAction)handleTowerSelectD:(id)sender;
-(IBAction)habdleTowerSell:(id)sender;
-(IBAction)habdleTowerUpgrade:(id)sender;

@end
