#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"

@interface View_ImagePlacement : UIView 
{
	View_ImagePlacementMgr* controller;
	UIImageView* image;
	UIImageView* pivotView;
	
#ifdef DEBUG
	UIImageView* debugView;
#endif
}

@property (nonatomic, retain) UIImageView* image;
@property (nonatomic, retain) UIImageView* pivotView;

-(id)initWithFrame:(CGRect)frame controller:(View_ImagePlacementMgr*)controller;
-(void)updateUi;

@end
