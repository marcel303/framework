#import "AppViewMgr.h"
#import "game.h"
#import "level.h"
#import "Log.h"

@implementation AppViewMgr

@synthesize livesLabel;
@synthesize scoreLabel;
@synthesize moneyLabel;
@synthesize levelLabel;
@synthesize towerAButton;
@synthesize towerBButton;
@synthesize towerCButton;
@synthesize towerDButton;
@synthesize towerSellButton;
@synthesize towerUpgradeButton;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil level:(NSString*)_path
{
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
		path = _path;
    }
    return self;
}

-(void)viewDidLoad
{
	// todo: load level
	
	gGame->Begin([path cStringUsingEncoding:NSASCIIStringEncoding]);
}

-(void)updateUi
{
	// update # lives
	[livesLabel setText:@"lives: x0"]; // todo: lives
	
	// update score
	[scoreLabel setText:@"score: 1000"]; // todo: score, dots
	
	// update money
	[moneyLabel setText:[NSString stringWithFormat:@"$%d", (int)gGame->Level_get()->BuildMoney_get()]]; // todo: dots
	
	// update tower level
	Tower* tower = gGame->SelectedTower_get();
	if (tower)
		[levelLabel setText:[NSString stringWithFormat:@"%d/%d", tower->Level_get() + 1, 5]]; // todo: max level
	else
		[levelLabel setText:@"N/A"];
	
	// update tower buttons
	[towerAButton setEnabled:gGame->Level_get()->BuildMoney_get() >= Tower::GetCost(TowerType_Vulcan, 0)];
	[towerBButton setEnabled:gGame->Level_get()->BuildMoney_get() >= Tower::GetCost(TowerType_Vulcan, 0)];
	[towerCButton setEnabled:gGame->Level_get()->BuildMoney_get() >= Tower::GetCost(TowerType_Slow, 0)];
	[towerDButton setEnabled:gGame->Level_get()->BuildMoney_get() >= Tower::GetCost(TowerType_Rocket, 0)];
	
	// update tower sell buttom
	[towerSellButton setEnabled:gGame->SelectedTower_get() != 0];
	
	// update tower upgrade button
	[towerUpgradeButton setEnabled:gGame->SelectedTower_get() != 0 && gGame->SelectedTower_get()->Level_get() < 4 && gGame->Level_get()->BuildMoney_get() >= Tower::GetCost(gGame->SelectedTower_get()->TowerType_get(), gGame->SelectedTower_get()->Level_get() + 1)]; // todo: max level
}

-(IBAction)handleTowerSelectA:(id)sender
{
	LOG_DBG("build: select: A", 0);
	
	gGame->TowerPlacementType_set(TowerType_Vulcan);
}

-(IBAction)handleTowerSelectB:(id)sender
{
	LOG_DBG("build: select: B", 0);
	
	gGame->TowerPlacementType_set(TowerType_Vulcan);
}

-(IBAction)handleTowerSelectC:(id)sender
{
	LOG_DBG("build: select: C", 0);
	
	gGame->TowerPlacementType_set(TowerType_Slow);
}

-(IBAction)handleTowerSelectD:(id)sender
{
	LOG_DBG("build: select: D", 0);
	
	gGame->TowerPlacementType_set(TowerType_Rocket);
}

-(IBAction)habdleTowerSell:(id)sender
{
	LOG_DBG("tower: sell", 0);
	
	Tower* tower = gGame->SelectedTower_get();
	
	Assert(tower);
	
	if (!tower)
		return;
	
	float cost = Tower::GetCost(tower->TowerType_get(), tower->Level_get());
	float refund = cost * 0.5f;
	
	gGame->Level_get()->BuildMoney_increase(refund);
	
	gGame->Level_get()->RemoveTower(tower);
}

-(IBAction)habdleTowerUpgrade:(id)sender
{
	LOG_DBG("tower: upgrade", 0);
	
	Assert(gGame->Level_get()->BuildMoney_get() >= Tower::GetCost(gGame->SelectedTower_get()->TowerType_get(), gGame->SelectedTower_get()->Level_get() + 1));
	
	if (!gGame->SelectedTower_get())
		return;
	
	float cost = Tower::GetCost(gGame->SelectedTower_get()->TowerType_get(), gGame->SelectedTower_get()->Level_get() + 1);
	
	if (gGame->Level_get()->BuildMoney_get() < cost)
		return;
	
	gGame->Level_get()->BuildMoney_decrease(cost);
	
	gGame->SelectedTower_get()->Upgrade();
}

-(void)viewDidUnload 
{
	LOG_DBG("viewDidUnload", 0);
	
    [super viewDidUnload];
	
	self.livesLabel = nil;
	self.scoreLabel = nil;
	self.moneyLabel = nil;
	self.levelLabel = nil;
	self.towerAButton = nil;
	self.towerBButton = nil;
	self.towerCButton = nil;
	self.towerDButton = nil;
	self.towerSellButton = nil;
	self.towerUpgradeButton = nil;
}

-(void)dealloc 
{
	LOG_DBG("dealloc: AppViewMgr", 0);
	
    [super dealloc];
}

@end
