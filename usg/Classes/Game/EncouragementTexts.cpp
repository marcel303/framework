#include "Calc.h"
#include "EncouragementTexts.h"

namespace GameText
{
	// --------------------
	// Positive: on level progression
	// --------------------
	
	const char* EncPositive[] = 
	{
		"SLAUGHTER!!!",
		"HAAAAAAAAX!!!",
		"You might've beaten my last incarnation, he was weak, I will not be so easy!",
		"I am called Infecto, and I will be the end of you!",
		"With each evolution your inevitable doom draws closer!",
		"I love you! Come a little closer!",
		"On today's menu: Triangles, Squares and more Triangles and Squares...",
		"If you got a penny for every square killed...",
		"Did you know? The number one on the charts is actually me.",
		"Did you know? Most people give up after level 5...",
		"It's useless, why bother, soon I will be INVINCIBLE!!",
		"If you manage to get a high score, I'll just erase it!",
		"You have no chance to survive make your time.",
		"Next round, I'll bring cake with cherries on top.",
		"I come in peace."
	#if defined(IPHONEOS)
		, "It wasn't always like this you know, I used to be a peaceful iDevice."
		, "This apple is gonna need some juice soon."
	#endif
	#if defined(BBOS)
		, "It wasn't always like this you know, this used to be a peaceful place."
	#endif
	#if defined(IPONEOS) || defined(BBOS) // touch screen
		, "Please stop, you're smudging up my screen."
	#endif
	};

	// --------------------
	// Game Over hints
	// --------------------

	const char* EncHint[] = 
	{
		"Use pinching to zoom out and find mines",
		"Use Shockwave on Magneto and big groups",
		"Special attack rockets pick random targets",
		"Watch out for drug powerups!",
		"Mines do permanent damage, take them out quickly!",
		"Mines have a heartbeat, faster means explosion imminent",
		"Small pickups help to get upgrades quicker",
		"The big boss grows and also gains more health each level",
		"Use the borders to escape quickly",
		"Use the borders alot? Get the reset upgrade",
		"Extra lives are very rare, don't count on them",
		"If you're not number 1, you haven't played enough!",
	};

	
	void GetEncouragementTextArray(bool positive, const char*** out_Array, int& out_ArraySize)
	{
		if (positive)
		{
			*out_Array = EncPositive;
			out_ArraySize = sizeof(EncPositive) / sizeof(char*);
		}
		else
		{
			*out_Array = EncHint;
			out_ArraySize = sizeof(EncHint) / sizeof(char*);
		}
	}
	
	void GetEncouragementText(bool positive, const char** out_Text)
	{
		const char** encArray;
		int encArraySize;
		GameText::GetEncouragementTextArray(positive, &encArray, encArraySize);
		int index = Calc::Random(encArraySize);
		*out_Text = encArray[index];
	}
}
