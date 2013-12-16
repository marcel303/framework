#pragma once
#import <UIKit/UIView.h>
#import "ViewControllerBase.h"
#import "View_Swatches.h"

@interface View_SwatchesMgr : ViewControllerBase <SwatchDelegate>
{

}

-(Rgba)swatchColorProvide:(int)index;
-(void)swatchColorSelect:(Rgba)color;

@end
