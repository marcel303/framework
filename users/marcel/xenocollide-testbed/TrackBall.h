/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "Math/Math.h"

//////////////////////////////////////////////////////////////////////////////
// TrackBall is part of the testbed framework.  It keeps track of the
// camera's orientation in the scene.

class TrackBall
{

public:

	TrackBall(void);
	~TrackBall(void);

	void Roll(float x, float y);
	void SetMagnitudeScale(float scale);
	float GetMagnitudeScale(void);

	// Quaternion-style accessors
	Quat GetRotation(void);
	void SetRotation(const Quat& rot);

	// Euler-style accessors
	void GetYawPitchRoll(float32* yaw, float32* pitch, float* roll);
	void SetYawPitchRoll(float32 yaw, float32 pitch, float32 roll);

	float m_magnitudeScale;
	Quat m_rotation;

};
