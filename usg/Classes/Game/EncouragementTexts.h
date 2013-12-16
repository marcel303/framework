#pragma once

namespace GameText
{
	extern void GetEncouragementTextArray(bool positive, const char*** out_Array, int& out_ArraySize);
	extern void GetEncouragementText(bool positive, const char** out_Text);
}
