#pragma once

#include "resource.h"

class Quat;

struct CameraController : Resource<CameraController>
{
	virtual void getProjectionProperties(
		float & fovY,
		float & nearDistance,
		float & farDistance) = 0;
};

//

struct CameraControllerTest : CameraController
{
	virtual void getProjectionProperties(
		float & fovY,
		float & nearDistance,
		float & farDistance) override final
	{
		fovY = 60.f;
		nearDistance = .01f;
		farDistance = 100.f;
	}
};

//

struct TransformController : Resource<TransformController>
{
	virtual void getTransform(
		Vec3 & translation,
		Quat & orientation) = 0;
};

//

#include "Quat.h"

struct TransformControllerTest : TransformController
{
	virtual void getTransform(
		Vec3 & translation,
		Quat & orientation) override final
	{
		translation.Set(0.f, 0.f, 0.f);
		orientation.makeIdentity();
	}
};
