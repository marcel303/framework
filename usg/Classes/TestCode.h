#import <UIKit/UIKit.h>
#import "GameView.h"
#import "ParticleEffect.h"
#import "SoundEffect.h"
#import "Types.h"
#import "WorldGrid.h"

@interface TestCode : NSObject 
{
	// todo: copy stuff from GameView
	
	GameView* m_GameView;
	
	Res* m_SoundEffect1;
	Res* m_SoundEffect2;
	
	ParticleEffect* m_ParticleEffect;
	
	Game::WorldGrid* m_WorldGrid;
	Vec2F* m_WorldGridEntities[4];
	
	ResIndex* m_ResIndex;
}

- (void)initWithGameView:(GameView*)gameView;

- (void)initUserInterface;
- (void)loopUserInterface;

- (void)initTextureAtlas;
- (void)loopTextureAtlas;

- (void)initSoundPlayer;
- (void)loopSoundPlayer;

- (void)initParticleEffect;
- (void)loopParticleEffect;

- (void)initVectorShape;
- (void)loopVectorShape;

- (void)initHashMap;
- (void)loopHashMap;

- (void)initResIndex;
- (void)loopResIndex;

- (void)initWorldGrid;
- (void)loopWorldGrid;

@end
