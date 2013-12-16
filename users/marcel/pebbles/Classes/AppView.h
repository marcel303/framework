#import <UIKit/UIKit.h>

class PlayerInfoHud
{
public:
	PlayerInfoHud(int x, int y, int score, int maxScore, int playerIdx)
	{
		mX = x;
		mY = y;
		mScore = score;
		mMaxScore = maxScore;
		mPlayerIdx = playerIdx;
	}
	
	void Update(float dt)
	{
	}
	
	void Render();
	
	int mX;
	int mY;
	int mScore;
	int mMaxScore;
	int mPlayerIdx;
};

@interface AppView : UIView 
{
	class OpenGLState* glState;
	NSTimer* updateTimer;
	class Thingy* activeThingy;
	float touchX;
	float touchY;
	bool touchActive;
	class PlayerInfoHud* playerHud[2];
}

-(void)updateBegin;
-(void)updateEnd;
-(void)update;

@end
