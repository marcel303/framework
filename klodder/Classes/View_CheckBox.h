#pragma once

#import <UIKit/UIView.h>

@interface View_CheckBox : UIView 
{
	@private
	
	id mDelegate;
	SEL mChanged;

	@public
	
	bool checked;
}

@property (nonatomic, assign) bool checked;

-(id)initWithLocation:(CGPoint)location andDelegate:(id)delegate andCallback:(SEL)changed;

@end
