#pragma once

#include "ui-capture.h"

#include "Mat4x4.h"

#include <functional>
#include <string>
#include <vector>

struct SceneEditor;

struct InteractiveRing
{
	struct SegmentItem
	{
		typedef std::function<void(SceneEditor & sceneEditor)> ActionHandler;
		typedef std::function<bool(const SceneEditor & sceneEditor)> IsEnabledCallback;
		
		std::string text;
		ActionHandler action;
		IsEnabledCallback isEnabled;
		float hoverAnim = 0.f;
		
		SegmentItem(const char * in_text, const ActionHandler & in_action, const IsEnabledCallback & in_isEnabled)
			: text(in_text)
			, action(in_action)
			, isEnabled(in_isEnabled)
		{
		}
	};
	
	struct Segment
	{
		std::vector<SegmentItem> items;
		
		// animation
		
		float hoverAnim = 0.f;
		
		Segment(const std::initializer_list<SegmentItem> & in_items)
			: items(in_items)
		{
		}
	};
	
	Mat4x4 transform;
	Mat4x4 rotation;

	UiCaptureElem captureElem;
	float openAnim = 0.f;
	
	std::vector<Segment> segments;
	
	int hoverSegmentIndex = -1;
	int hoverItemIndex = -1;
	
	struct
	{
		float angle = 0.f;
	} debug;
	
	InteractiveRing();
	
	void show(const Mat4x4 & transform);
	void hide();
	
	void tick(
		SceneEditor & sceneEditor,
		Vec3Arg pointerOrigin_world,
		Vec3Arg pointerDirection_world,
		const Mat4x4 & viewMatrix,
		const bool pointerIsActive,
		bool & inputIsCaptured,
		const float dt);
		
	void tickInteraction(
		SceneEditor & sceneEditor,
		Vec3Arg pointerOrigin_world,
		Vec3Arg pointerDirection_world,
		const bool pointerIsActive,
		bool & inputIsCaptured);
		
	void tickAnimation(
		const Mat4x4 & viewMatrix,
		const float dt);
	
	void drawOpaque(
		const SceneEditor & sceneEditor) const;
	
	void calculateTransform(Mat4x4 & out_transform) const;
};
