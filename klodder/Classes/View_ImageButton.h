#import <UIKit/UIView.h>

@interface View_ImageButton : UIView 
{
	NSString* name;
	id delegate;
	SEL clicked;
}

-(id)initWithFrame:(CGRect)frame andImage:(NSString*)name andDelegate:(id)delegate andClicked:(SEL)clicked;

@end
