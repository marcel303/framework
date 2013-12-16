#import <UIKit/UIKit.h>
#import <vector>
#import "libiphone_forward.h"
#import "OpenGLCompat.h"
#import "render.h"

class ButtonDef
{
public:
	ButtonDef() : c(0.0f, 0.0f, 0.0f)
	{
	}
	
	ButtonDef(float _x, float _y, float _r, Color _c, int _axis, int _direction, int _location) : c(0.0f, 0.0f, 0.0f)
	{
		x = _x;
		y = _y;
		r = _r;
		c = _c;
		axis = _axis;
		direction = _direction;
		location = _location;
	}
	
	inline bool IsInside(float _x, float _y) const
	{
		return
			_x >= x - r && _x <= x + r &&
			_y >= y - r && _y <= y + r;
	}
	
	float x;
	float y;
	float r;
	Color c;
	int axis;
	int direction;
	int location;
};

@interface BoardView : UIView 
{
	OpenGLState* glState;
	NSTimer* animationTimer;
	std::vector<ButtonDef> buttons;
	ButtonDef* activeButton;
	bool activeButtonFocus;
	GLuint buttonTextureId;
}

-(void)handleShuffle:(id)sender;
-(void)performShuffle;
-(void)handleWin;
-(void)handleAnimationTrigger;

-(void)render;

-(void)animationBegin;
-(void)animationEnd;
-(void)animationUpdate;

@end
