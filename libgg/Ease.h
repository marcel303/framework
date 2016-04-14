#pragma once

enum EaseType
{
	kEaseType_Linear,
	kEaseType_PowIn,
	kEaseType_PowOut,
	kEaseType_PowInOut,
	kEaseType_SineIn,
	kEaseType_SineOut,
	kEaseType_SineInOut,
	kEaseType_BackIn,
	kEaseType_BackOut,
	kEaseType_BackInOut,
	kEaseType_BounceIn,
	kEaseType_BounceOut,
	kEaseType_BounceInOut,
	kEaseType_Count
};

float EvalEase(float t, EaseType type, float param1);
