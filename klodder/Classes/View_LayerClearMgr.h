#pragma once
#import <UIKit/UIView.h>
#import "ColorPickerState.h"
#import "ViewControllerBase.h"

@interface View_LayerClearMgr : ViewControllerBase
{
	int index;
	UIColor* color;
}

@property (nonatomic, retain) UIColor* color;

-(id)initWithApp:(AppDelegate*)app index:(int)index;
-(void)colorChanged;
-(void)clearLayer;
-(IBAction)handleDone:(id)sender;

@end
