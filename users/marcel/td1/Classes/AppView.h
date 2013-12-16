#import <UIKit/UIKit.h>
#import "libiphone_forward.h"

#import "tower.h"
#import "Types.h"
#import "WaterGrid.h"

@class AppViewMgr;

class PlacementState
{
public:
	PlacementState()
	{
		isActive = false;
		type = TowerType_Undefined;
		cost = 0.0f;
		radius = 0.0f;
		isFree = false;
	}
	
	bool isActive;
	TowerType type;
	float cost;
	float radius;
	Vec2F location;
	bool isFree;
};

@interface AppView : UIView
{
	OpenGLState* glState;
	
	PlacementState* placementState;
	
	AppViewMgr* delegate;
	
	WaterGrid* waterGrid;
}

@property (nonatomic, retain) IBOutlet AppViewMgr* delegate;

-(void)render;
-(void)placementSet:(Vec2F)location;
-(void)handleExplosion:(Vec2F)location strength:(float)strength;

@end
