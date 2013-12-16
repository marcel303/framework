#import <Foundation/Foundation.h>
#import "puzzle.h"

@interface OpenFeintHelper : NSObject 
{
}

+(void)rankTime:(PuzzleMode)mode size:(int)size time:(int)time;
+(void)rankMoves:(PuzzleMode)mode size:(int)size moveCount:(int)moveCount;
+(void)achieveTime:(PuzzleMode)mode size:(int)size time:(int)time;
+(void)achieveMoves:(PuzzleMode)mode size:(int)size moveCount:(int)moveCount;
+(void)achievePlayTime:(int)seconds;
+(void)achievePlayCount:(int)playCount;

@end
