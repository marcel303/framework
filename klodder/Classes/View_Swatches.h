#pragma once
#import <UIKit/UIView.h>
#import "View_Swatch.h"

@interface View_Swatches : UIView
{
	id<SwatchDelegate> delegate;
}

-(id)initWithFrame:(CGRect)frame delegate:(id<SwatchDelegate>)delegate;
-(void)updateUi;

@end
