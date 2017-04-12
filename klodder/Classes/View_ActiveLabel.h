#import <UIKit/UIView.h>

@interface View_ActiveLabel : UIView 
{
	id delegate;
	SEL clicked;
}

-(id)initWithFrame:(CGRect)frame andText:(NSString*)text andFont:(NSString*)font ofSize:(CGFloat)size andAlignment:(NSTextAlignment)alignment andColor:(UIColor*)color andDelegate:(id)delegate andClicked:(SEL)clicked;

@end
