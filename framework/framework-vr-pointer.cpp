#include "framework-vr-pointer.h"

Mat4x4 VrPointerBase::getTransform(Vec3Arg vrOrigin) const
{
	return Mat4x4(true).Translate(vrOrigin).Mul(transform);
}

#if FRAMEWORK_USE_OVR_MOBILE

#include <VrApi_Helpers.h>
#include <VrApi_Input.h>
#include "framework-ovr.h"

void VrPointer::init(VrSide in_side)
{
	side = in_side;
}

void VrPointer::shut()
{
}

void VrPointer::updateInputState()
{
	hasTransform = false;
	
	DeviceID = -1;
	
	for (int i = 0; i < VrButton_COUNT; ++i)
		m_hasChanged[i] = false;

	ovrMobile * ovr = frameworkOvr.Ovr;
	
	int index = 0;

	for (;;)
	{
		ovrInputCapabilityHeader header;

		if (vrapi_EnumerateInputDevices(ovr, index++, &header) < 0)
			break;

		if (header.Type == ovrControllerType_TrackedRemote)
		{
			ovrInputStateTrackedRemote state;
			state.Header.ControllerType = ovrControllerType_TrackedRemote;
			if (vrapi_GetCurrentInputState(ovr, header.DeviceID, &state.Header) >= 0)
			{
				VrSide controllerSide = VrSide_Undefined;

				ovrInputTrackedRemoteCapabilities remoteCaps;
				remoteCaps.Header.Type = ovrControllerType_TrackedRemote;
				remoteCaps.Header.DeviceID = header.DeviceID;
				if (vrapi_GetInputDeviceCapabilities(ovr, &remoteCaps.Header) != ovrSuccess)
					continue;
				
				if (remoteCaps.ControllerCapabilities & ovrControllerCaps_LeftHand)
					controllerSide = VrSide_Left;
				if (remoteCaps.ControllerCapabilities & ovrControllerCaps_RightHand)
					controllerSide = VrSide_Right;

				if (controllerSide != side)
					continue;
				
				DeviceID = header.DeviceID;

				ovrTracking tracking;
				if (vrapi_GetInputTrackingState(
					ovr,
					header.DeviceID,
					frameworkOvr.PredictedDisplayTime,
					&tracking) != ovrSuccess)
				{
					tracking.Status = 0;
				}
				
				if (tracking.Status & VRAPI_TRACKING_STATUS_POSITION_VALID)
				{
					ovrMatrix4f ovr_transform = vrapi_GetTransformFromPose(&tracking.HeadPose.Pose);
					ovr_transform = ovrMatrix4f_Transpose(&ovr_transform);

					memcpy(&transform, (float*)ovr_transform.M, sizeof(Mat4x4));
					transform = Mat4x4(true).Scale(1, 1, -1).Mul(transform).Scale(1, 1, -1); // convert from a right-handed coordinate system to a left-handed one
					hasTransform = true;
				}
				else
				{
					transform.MakeIdentity();
					hasTransform = false;
				}

				const int buttonMasks[VrButton_COUNT] =
					{
						ovrButton_Trigger,
						ovrButton_GripTrigger,
						0,
						0
					};

				for (int i = 0; i < VrButton_COUNT; ++i)
				{
					if (buttonMasks[i] == 0)
						continue;
					
					const bool wasDown = m_isDown[i];

					if (state.Buttons & buttonMasks[i])
						m_isDown[i] = true;
					else
						m_isDown[i] = false;

					if (m_isDown[i] != wasDown)
					{
						m_hasChanged[i] = true;
					}
				}
			}
		}
	}
}

void VrPointer::updateHaptics()
{
	if (DeviceID != -1)
	{
		ovrMobile * ovr = frameworkOvr.Ovr;
		
		vrapi_SetHapticVibrationSimple(ovr, DeviceID, wantsToVibrate ? .5f : 0.f);
	}
	
	wantsToVibrate = false;
}

#endif
