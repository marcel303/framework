#pragma once
#import <UIKit/UIView.h>
#import "Types.h"

@interface View_BrushRetina : UIView 
{
	bool visible;
}

-(id)initWithRadius:(float)radius;
-(void)setLocation:(Vec2F)location;

@end
