#include "framework-vr-hands.h"
#include <math.h>

VrHand::VrHand()
{
	for (auto & index : skeleton.fingerNameToBoneIndex)
		index = -1;
}

Mat4x4 VrHand::getTransform(const Vec3 worldOffset) const
{
	return Mat4x4(true)
		.Translate(worldOffset)
		.Mul(rootPose);
}

bool VrHand::getFingerTransform(const VrFingers finger, const Vec3 worldOffset, Mat4x4 & out_transform) const
{
	if (!hasSkeleton || !hasDeform)
		return false;

	const int boneIndex = skeleton.fingerNameToBoneIndex[finger];
	if (boneIndex < 0)
		return false;

// fixme : rotate by 90 degrees here is ovr-mobile specific. move it over to ovr-mobile implementation
	out_transform = Mat4x4(true)
		.Translate(worldOffset)
		.Mul(rootPose)
		.Mul(deform.globalBoneTransforms[boneIndex])
		.RotateY(side == VrSide_Left ? +float(M_PI)/2.f : -float(M_PI)/2.f);
	return true;
}

void VrHand::resizeSkeleton(const int numBones)
{
	skeleton.numBones = numBones;
	skeleton.boneParentIndices.resize(numBones);
	skeleton.localBoneTransforms.resize(numBones);
	skeleton.globalBoneTransforms.resize(numBones);

	deform.localBoneTransforms.resize(numBones);
	deform.globalBoneTransforms.resize(numBones);
}

void VrHand::localPoseToGlobal(
	const Mat4x4 * __restrict localMatrices,
	const int * __restrict parentIndices,
	const int numMatrices,
	Mat4x4 * __restrict globalMatrices)
{
	for (int i = 0; i < numMatrices; ++i)
	{
		const Mat4x4 & transform = localMatrices[i];
		const int parentIndex = parentIndices[i];
		if (parentIndex < 0)
			globalMatrices[i] = transform;
		else
			globalMatrices[i] = globalMatrices[parentIndex] * transform;
	}
}

void VrHand::calculateGlobalBindPose()
{
	// Calculate global bone transforms (bind pose).
	localPoseToGlobal(
		skeleton.localBoneTransforms.data(),
		skeleton.boneParentIndices.data(),
		skeleton.numBones,
		skeleton.globalBoneTransforms.data());
}

void VrHand::calculateGlobalDeformPose()
{
	// Calculate global bone transforms (animated).
	localPoseToGlobal(
		deform.localBoneTransforms.data(),
		skeleton.boneParentIndices.data(),
		skeleton.numBones,
		deform.globalBoneTransforms.data());
}

void VrHand::calculateSkinnningMatrices(Mat4x4 * skinningMatrices, const int maxBones) const
{
	// Calculate skinning matrices. Skinning matrices will first transform a vertex into 'bind space',
	// by applying the inverse of the initial pose transform. It will then apply the global bone transform.
	for (int i = 0; i < skeleton.numBones && i < maxBones; ++i)
	{
		const Mat4x4 objectToBind = skeleton.globalBoneTransforms[i].CalcInv();
		const Mat4x4 & bindToDeform = deform.globalBoneTransforms[i];
		skinningMatrices[i] = bindToDeform * objectToBind;
	}
}

#if FRAMEWORK_USE_OVR_MOBILE

#include "data/engine/ShaderCommon.txt" // VS_ constants
#include "framework-ovr.h"
#include "gx_mesh.h"
#include <VrApi_Helpers.h>
#include <VrApi_Input.h>

struct VrHandData
{
	GxMesh mesh;
	GxVertexBuffer vb;
	GxIndexBuffer ib;

	ShaderBuffer skinningData;
};

void VrHand::init(VrSide in_side)
{
	Assert(data == nullptr);
	
	data = new VrHandData();
	
	side = in_side;

	const auto ovrHand = (side == VrSide_Left) ? VRAPI_HAND_LEFT : VRAPI_HAND_RIGHT;

	ovrHandMesh handMesh;
	handMesh.Header.Version = ovrHandVersion_1;
	if (vrapi_GetHandMesh(frameworkOvr.Ovr, ovrHand, &handMesh.Header) == ovrSuccess)
	{
		GxVertexInput vsInputs[] =
		{
			{ VS_POSITION,      3, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexPositions), sizeof(ovrVector3f) },
			{ VS_NORMAL,        3, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexNormals),   sizeof(ovrVector3f) },
			//{ VS_COLOR,         3, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexNormals),   sizeof(ovrVector3f) },
			{ VS_TEXCOORD0,     2, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexUV0),       sizeof(ovrVector2f) },
			{ VS_BLEND_INDICES, 4, GX_ELEMENT_UINT16,  0, offsetof(ovrHandMesh, BlendIndices),    sizeof(ovrVector4s) },
			{ VS_BLEND_WEIGHTS, 4, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, BlendWeights),    sizeof(ovrVector4f) }
		};
		const int numVsInputs = sizeof(vsInputs) / sizeof(vsInputs[0]);

		data->vb.alloc(&handMesh, sizeof(handMesh));
		data->ib.alloc(handMesh.Indices, sizeof(handMesh.Indices) / sizeof(handMesh.Indices[0]), GX_INDEX_16);
		data->mesh.setVertexBuffer(&data->vb, vsInputs, numVsInputs, 0);
		data->mesh.setIndexBuffer(&data->ib);
		data->mesh.addPrim(GX_TRIANGLES, handMesh.NumIndices, true);
	}

	data->skinningData.alloc(sizeof(Mat4x4) * 32);
}

void VrHand::shut()
{
	side = VrSide_Undefined;

	if (data != nullptr)
	{
		data->mesh.clear();
		data->vb.free();
		data->ib.free();

		data->skinningData.free();
		
		delete data;
		data = nullptr;
	}
}

void VrHand::updateInputState()
{
	for (int i = 0; i < VrFinger_COUNT; ++i)
		fingers[i].isPinching = false;
	hasSkeleton = false;
	hasDeform = false;

	// loop through input device and find our hand

	auto * ovr = frameworkOvr.Ovr;

	uint32_t deviceIndex = 0;

	for (;;)
	{
		ovrInputCapabilityHeader header;
		if (vrapi_EnumerateInputDevices(ovr, deviceIndex++, &header) < 0)
			break;

		// Is it a hand?
		if (header.Type != ovrControllerType_Hand)
			continue;

		// Describe hand.
		ovrInputHandCapabilities handCapabilities;
		handCapabilities.Header = header;
		if (vrapi_GetInputDeviceCapabilities(ovr, &handCapabilities.Header) != ovrSuccess)
			continue;

		// Is it our hand?
		if (side == VrSide_Left && (handCapabilities.HandCapabilities & ovrHandCaps_LeftHand) == 0)
			continue;
		if (side == VrSide_Right && (handCapabilities.HandCapabilities & ovrHandCaps_RightHand) == 0)
			continue;

		for (;;)
		{
			// Fetch the input state for the hand.
			ovrInputStateHand hand;
			hand.Header.ControllerType = ovrControllerType_Hand;
			if (vrapi_GetCurrentInputState(ovr, header.DeviceID, &hand.Header) != ovrSuccess)
				break;

			// Update pinching state for all five fingers.
			
			const int ovrHandStatus[] =
			{
				//ovrInputStateHandStatus_ThumbPinching,
				ovrInputStateHandStatus_IndexPinching,
				ovrInputStateHandStatus_MiddlePinching,
				ovrInputStateHandStatus_RingPinching,
				ovrInputStateHandStatus_PinkyPinching
			};
			
			const int ovrPinchStrength[] =
			{
				//ovrHandPinchStrength_Thumb,
				ovrHandPinchStrength_Index,
				ovrHandPinchStrength_Middle,
				ovrHandPinchStrength_Ring,
				ovrHandPinchStrength_Pinky
			};
			
			const VrFingers vrFinger[] =
			{
				//VrFinger_Thumb,
				VrFinger_Index,
				VrFinger_Middle,
				VrFinger_Ring,
				VrFinger_Pinky
			};
	
			for (int i = 0; i < sizeof(ovrHandStatus) / sizeof(ovrHandStatus[0]); ++i)
			{
				if ((hand.InputStateStatus & ovrHandStatus[i]) && hand.PinchStrength[ovrPinchStrength[i]] >= .5f)
				{
					fingers[vrFinger[i]].isPinching = true;
				}
			}
			
			break;
		}

		for (;;)
		{
			// Fetch the initial pose for the hand.
			// We will need to combine it with the current pose, which only defines the new orientations for the bones.
			// And we will need it to perform skinning as well.
			ovrHandSkeleton handSkeleton;
			handSkeleton.Header.Version = ovrHandVersion_1;
			const auto ovrHand = (side == VrSide_Left) ? VRAPI_HAND_LEFT : VRAPI_HAND_RIGHT;
			if (vrapi_GetHandSkeleton(ovr, ovrHand, &handSkeleton.Header) != ovrSuccess)
				break;

			hasSkeleton = true;

			resizeSkeleton(handSkeleton.NumBones);

			// Fill in bone hierarchy.
			for (int i = 0; i < handSkeleton.NumBones; ++i)
				skeleton.boneParentIndices[i] = handSkeleton.BoneParentIndices[i];

			// Fill in bone indices for finger tips (for pointer positions and directions).
			skeleton.fingerNameToBoneIndex[VrFinger_Thumb]  = ovrHandBone_ThumbTip;
			skeleton.fingerNameToBoneIndex[VrFinger_Index]  = ovrHandBone_IndexTip;
			skeleton.fingerNameToBoneIndex[VrFinger_Middle] = ovrHandBone_MiddleTip;
			skeleton.fingerNameToBoneIndex[VrFinger_Ring]   = ovrHandBone_RingTip;
			skeleton.fingerNameToBoneIndex[VrFinger_Pinky]  = ovrHandBone_PinkyTip;

			// Fill in local bone transforms (bind pose).
			for (int i = 0; i < handSkeleton.NumBones; ++i)
			{
				const ovrPosef & pose = handSkeleton.BonePoses[i];
				const ovrMatrix4f transform = vrapi_GetTransformFromPose(&pose);
				const ovrMatrix4f transform_transposed = ovrMatrix4f_Transpose(&transform);
				memcpy(skeleton.localBoneTransforms[i].m_v, transform_transposed.M, sizeof(Mat4x4));
			}

			// Calculate global bind pose.
			calculateGlobalBindPose();

			// Fetch the current pose for the hand.
			ovrHandPose handPose;
			handPose.Header.Version = ovrHandVersion_1;
			if (vrapi_GetHandPose(ovr, header.DeviceID, frameworkOvr.PredictedDisplayTime, &handPose.Header) != ovrSuccess)
				break;

			// Is the hand being tracked, with high confidence?
			if (handPose.Status != ovrHandTrackingStatus_Tracked ||
				handPose.HandConfidence != ovrConfidence_HIGH)
				break;

			hasDeform = true;

			// Update local bone poses.
			for (int i = 0; i < handSkeleton.NumBones; ++i)
			{
				ovrPosef pose = handSkeleton.BonePoses[i];
				pose.Orientation = handPose.BoneRotations[i];
				const ovrMatrix4f transform = vrapi_GetTransformFromPose(&pose);
				const ovrMatrix4f transform_transposed = ovrMatrix4f_Transpose(&transform);

				memcpy(deform.localBoneTransforms[i].m_v, transform_transposed.M, sizeof(Mat4x4));
			}

			calculateGlobalDeformPose();

			ovrMatrix4f ovrRootPose = vrapi_GetTransformFromPose(&handPose.RootPose);
			ovrRootPose = ovrMatrix4f_Transpose(&ovrRootPose);
			memcpy(rootPose.m_v, ovrRootPose.M, sizeof(Mat4x4));
			rootPose = rootPose.Scale(handPose.HandScale);
			rootPose = Mat4x4(true).Scale(1, 1, -1).Mul(rootPose); // convert from a right-handed coordinate system to a left-handed one

			break;
		}
	}
}

void VrHand::updateSkinningMatrices() const
{
	Mat4x4 skinningTransforms[32];
	calculateSkinnningMatrices(skinningTransforms, 32);

	data->skinningData.setData(skinningTransforms, sizeof(skinningTransforms));
}

ShaderBuffer & VrHand::getSkinningMatrices(const bool update) const
{
	if (update)
		updateSkinningMatrices();

	return data->skinningData;
}

void VrHand::drawMesh() const
{
	data->mesh.draw();
}

#else

void VrHand::init(VrSide side)
{
}

void VrHand::shut()
{
}

void VrHand::updateInputState()
{
}

void VrHand::updateSkinningMatrices() const
{
}

ShaderBuffer & VrHand::getSkinningMatrices(const bool update) const
{
	ShaderBuffer * result = nullptr;
	return *result;
}

void VrHand::drawMesh() const
{
}

#endif
