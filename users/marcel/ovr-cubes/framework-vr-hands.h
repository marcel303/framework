#pragma once

#include "gx_mesh.h"
#include "Mat4x4.h"
#include <vector>

enum VrHands
{
	VrHand_Left,
	VrHand_Right,
	VrHand_COUNT
};

enum VrFingers
{
	VrFinger_Thumb,
	VrFinger_Index,
	VrFinger_Middle,
	VrFinger_Ring,
	VrFinger_Pinky,
	VrFinger_COUNT
};

struct VrHandBase
{
	bool hasSkeleton = false;
	struct
	{
		int numBones = 0;

		std::vector<int> boneParentIndices; // bone hierarchy

		std::vector<Mat4x4> localBoneTransforms;  // bind pose in local space
		std::vector<Mat4x4> globalBoneTransforms; // bind pose in global space

		int fingerNameToBoneIndex[VrFinger_COUNT];
	} skeleton;

	bool hasDeform = false;
	struct
	{
		std::vector<Mat4x4> localBoneTransforms;
		std::vector<Mat4x4> globalBoneTransforms;
	} deform;

	Mat4x4 rootPose = Mat4x4(true);

	struct
	{
		bool isPinching = false;
	} fingers[VrFinger_COUNT];

	VrHandBase();

	virtual void init(VrHands hand) = 0;
	virtual void shut() = 0;

	virtual void updateInputState() = 0;
	virtual void updateSkinningMatrices() const = 0;
	virtual ShaderBuffer & getSkinningMatrices(const bool update) const = 0;
	virtual void drawMesh() const = 0;

	Mat4x4 getTransform(const Vec3 worldOffset) const;

	/**
	 * Returns the world-space pointer transform for a finger.
	 * @param finger The finger for which to get the transform.
	 * @return The world-space pointer transform.
	 */
	bool getFingerTransform(const VrFingers finger, const Vec3 worldOffset, Mat4x4 & out_transform) const;

	void resizeSkeleton(const int numBones);

	static void localPoseToGlobal(
		const Mat4x4 * __restrict localMatrices,
		const int * __restrict parentIndices,
		const int numMatrices,
		Mat4x4 * __restrict globalMatrices);

	void calculateGlobalBindPose();
	void calculateGlobalDeformPose();
	void calculateSkinnningMatrices(Mat4x4 * skinningMatrices, const int maxBones) const;
};

class VrHand : public VrHandBase
{
	VrHands hand;

	GxMesh mesh;
	GxVertexBuffer vb;
	GxIndexBuffer ib;

	mutable ShaderBuffer skinningData;

public:
	virtual void init(VrHands in_hand) override;
	virtual void shut() override;

	virtual void updateInputState() override;

	virtual void updateSkinningMatrices() const override;
	virtual ShaderBuffer & getSkinningMatrices(const bool update) const override;

	virtual void drawMesh() const override;
};
