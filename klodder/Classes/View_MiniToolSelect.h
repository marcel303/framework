#import <UIKit/UIView.h>

@interface View_MiniToolSelect : UIView 
{
	id delegate;
	SEL action;
}

-(id)initWithFrame:(CGRect)frame delegate:(id)delegate action:(SEL)action;
-(void)handleBrush;
-(void)handleSmudge;
-(void)handleEraser;

@end
