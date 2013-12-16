#pragma once
#import <UIKit/UIView.h>
#import <vector>
#import "Types.h"

/*
 - On touch, displays 3 additional buttons
 - Gesture decides action:
 	- Left, right, top, middle
 */

enum MagicAction
{
	MagicAction_None,
	MagicAction_Left,
	MagicAction_Right,
	MagicAction_Top,
	MagicAction_Middle
};

@protocol MagicDelegate

-(void)handleMagicAction:(MagicAction)action;
-(UIImage*)provideMagicImage:(MagicAction)action;

@end

class MagicButtonInfo
{
public:
	MagicAction action;
	UIImageView* view;
	Vec2F location;
};

@interface View_MagicButton : UIView 
{
	id<MagicDelegate> delegate;
	UIView* glow;
	std::vector<MagicButtonInfo> buttons;
	Vec2F touchLocation1;
	Vec2F touchLocation2;
	
	float fade;
	float fadeTarget;
	NSTimer* animationTimer;
}

@property (nonatomic, retain) NSTimer* animationTimer;

-(id)initWithFrame:(CGRect)frame delegate:(id<MagicDelegate>)delegate;
-(MagicAction)decideAction;
-(void)updateGlow:(MagicAction)action;
-(void)updateButtonVisibility:(BOOL)visible;
-(void)updateFade:(float)fade;
-(void)setVisible:(BOOL)visible;

-(void)animationBegin;
-(void)animationEnd;
-(void)animationUpdate;
-(void)animationCheck;

@end
