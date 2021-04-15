#include "interactiveRing.h"

// ecs-sceneedit includes
#include "sceneEditor.h"

// framework includes
#include "framework.h"

// libgg includes
#include "Calc.h"

static const float kInnerRadius = .3f;
static const float kFirstItemDistance = .75f;
static const float kItemSpacing = .5f;

static const float kRingOpenAnimTime = .2f;
static const float kRingCloseAnimTime = .1f;
static const float kSegmentAnimTime = .2f;
static const float kItemAnimTime = .1f;

static const Color kDefaultItemColor(200, 200, 200);
static const Color kActiveSegmentItemColor(220, 220, 220);
static const Color kHoverItemColor(255, 255, 255);

static const Color kDefaultItemColor_Disabled(100, 100, 100);
static const Color kActiveSegmentItemColor_Disabled(110, 110, 110);
static const Color kHoverItemColor_Disabled(127, 127, 127);

InteractiveRing::InteractiveRing()
{
	segments =
	{
		{ {
			{ "Copy", [](SceneEditor & sceneEditor) { sceneEditor.performAction_copy(true); }, [](const SceneEditor & sceneEditor) { return !sceneEditor.selection.selectedNodes.empty(); } },
			{ "Copy Single", [](SceneEditor & sceneEditor) { sceneEditor.performAction_copy(false); }, [](const SceneEditor & sceneEditor) { return !sceneEditor.selection.selectedNodes.empty(); } }
		} },
		{ {
			{ "Add", [](SceneEditor & sceneEditor) { sceneEditor.performAction_addChild(); }, [](const SceneEditor & sceneEditor) { return true; } },
			{ "Add Sibling", [](SceneEditor & sceneEditor) { }, [](const SceneEditor & sceneEditor) { return true; } },
			{ "Duplicate", [](SceneEditor & sceneEditor) { sceneEditor.performAction_duplicate(); }, [](const SceneEditor & sceneEditor) { return !sceneEditor.selection.selectedNodes.empty(); } }
		} },
		{ {
			{ "Paste", [](SceneEditor & sceneEditor) { sceneEditor.performAction_paste(sceneEditor.scene.rootNodeId); }, [](const SceneEditor & sceneEditor) { return sceneEditor.clipboardInfo.hasNode || sceneEditor.clipboardInfo.hasNodeTree; } },
			{ "Paste Child", [](SceneEditor & sceneEditor) { sceneEditor.performAction_pasteChild(); }, [](const SceneEditor & sceneEditor) { return sceneEditor.clipboardInfo.hasNode || sceneEditor.clipboardInfo.hasNodeTree; } },
			{ "Paste Sibling", [](SceneEditor & sceneEditor) { sceneEditor.performAction_pasteSibling(); }, [](const SceneEditor & sceneEditor) { return sceneEditor.clipboardInfo.hasNode || sceneEditor.clipboardInfo.hasNodeTree; } }
		} },
		{ {
			{ "Remove", [](SceneEditor & sceneEditor) { sceneEditor.performAction_remove(); }, [](const SceneEditor & sceneEditor) { return !sceneEditor.selection.selectedNodes.empty(); } }
		} }
	};
}

void InteractiveRing::show(const Mat4x4 & in_transform)
{
	Assert(!captureElem.hasCapture);

	transform = in_transform;
	
	captureElem.capture();
}

void InteractiveRing::hide()
{
	captureElem.discard();
}

void InteractiveRing::tick(
	SceneEditor & sceneEditor,
	Vec3Arg pointerOrigin_world,
	Vec3Arg pointerDirection_world,
	const Mat4x4 & viewMatrix,
	const bool pointerIsActive,
	bool & inputIsCaptured,
	const float dt)
{
	tickInteraction(
		sceneEditor,
		pointerOrigin_world,
		pointerDirection_world,
		pointerIsActive,
		inputIsCaptured);
	
	tickAnimation(
		viewMatrix,
		dt);
}

void InteractiveRing::tickInteraction(
	SceneEditor & sceneEditor,
	Vec3Arg pointerOrigin_world,
	Vec3Arg pointerDirection_world,
	const bool pointerIsActive,
	bool & inputIsCaptured)
{
	if (captureElem.hasCapture &&
		pointerIsActive &&
		captureElem.capture())
	{
		Assert(inputIsCaptured);
		
		if (openAnim == 1.f)
		{
			// determine the hover segment and hover item (if any)
			
			Mat4x4 objectToWorld;
			calculateTransform(objectToWorld);
			
			const Mat4x4 worldToObject = objectToWorld.CalcInv();
			
			const Vec3 pointerOrigin_object = worldToObject.Mul4(pointerOrigin_world);
			const Vec3 pointerDirection_object = worldToObject.Mul3(pointerDirection_world);
			
			hoverSegmentIndex = -1;
			hoverItemIndex = -1;
			
			// determine intersection point
			
			if (pointerDirection_object[2] > 0.f)
			{
				const float t = - pointerOrigin_object[2] / pointerDirection_object[2];
				
				const Vec3 intersectionPoint = pointerOrigin_object + pointerDirection_object * t;
				
				// determine hover segment
				
				const float angle = atan2f(intersectionPoint[1], intersectionPoint[0]);
				
				debug.angle = angle;
				
				const float distance = hypotf(intersectionPoint[0], intersectionPoint[1]);
				
				if (distance > kInnerRadius)
				{
					const float anglePerSegment = Calc::m2PI / segments.size();
					const float anglePerSegmentToDraw = Calc::m2PI / (segments.size() + 1);
					
					for (size_t i = 0; i < segments.size(); ++i)
					{
						const float segmentAngle = i * anglePerSegment;
						
						float angleDifference = angle - segmentAngle;
						
						while (angleDifference < Calc::mPI)
							angleDifference += Calc::m2PI;
						while (angleDifference > Calc::mPI)
							angleDifference -= Calc::m2PI;
						
						if (angleDifference >= - anglePerSegmentToDraw / 2.f &&
							angleDifference <= + anglePerSegmentToDraw / 2.f)
						{
							hoverSegmentIndex = i;
							
							hoverItemIndex = (int)roundf((distance - kFirstItemDistance) / kItemSpacing);
							
							if (hoverItemIndex < 0 || hoverItemIndex >= segments[i].items.size())
							{
								hoverItemIndex = -1;
							}
						}
					}
				}
			}
		}
	}
	
	if (captureElem.hasCapture && !pointerIsActive)
	{
		// invoke action
		
		if (hoverSegmentIndex != -1 && hoverItemIndex != -1)
		{
			auto & segment = segments[hoverSegmentIndex];
			
			if (segment.items[hoverItemIndex].action != nullptr)
			{
				segment.items[hoverItemIndex].action(sceneEditor);
			}
		}
	}
}

void InteractiveRing::tickAnimation(
	const Mat4x4 & viewMatrix,
	const float dt)
{
	rotation = viewMatrix;
	rotation.SetTranslation(Vec3());
	
	if (captureElem.hasCapture)
	{
		openAnim = fminf(1.f, openAnim + dt / kRingOpenAnimTime);
	}
	else
	{
		openAnim = fmaxf(0.f, openAnim - dt / kRingCloseAnimTime);
	}
	
	for (size_t segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex)
	{
		auto & segment = segments[segmentIndex];
		
		if (captureElem.hasCapture && segmentIndex == hoverSegmentIndex)
		{
			segment.hoverAnim = fminf(1.f, segment.hoverAnim + dt / kSegmentAnimTime);
		}
		else
		{
			segment.hoverAnim = fmaxf(0.f, segment.hoverAnim - dt / kSegmentAnimTime);
		}
		
		for (size_t itemIndex = 0; itemIndex < segment.items.size(); ++itemIndex)
		{
			auto & item = segment.items[itemIndex];
			
			if (captureElem.hasCapture && segmentIndex == hoverSegmentIndex && hoverItemIndex == itemIndex)
			{
				item.hoverAnim = fminf(1.f, item.hoverAnim + dt / kItemAnimTime);
			}
			else
			{
				item.hoverAnim = fmaxf(0.f, item.hoverAnim - dt / kItemAnimTime);
			}
		}
	}
}

void InteractiveRing::drawOpaque(const SceneEditor & sceneEditor) const
{
	if (openAnim == 0.f)
		return;
		
	Mat4x4 objectToWorld;
	calculateTransform(objectToWorld);
	
	pushCullMode(CULL_NONE, CULL_CCW);
	
	gxPushMatrix();
	{
		gxMultMatrixf(objectToWorld.m_v);
		
		// todo : default framework font doesn't work with MSDF library. why? and fix and/or change default font
		setFont("calibri.ttf");
		pushFontMode(FONT_SDF);
		
		// draw background
		
		setColor(100, 100, 100);
		fillCircle(0, 0, kFirstItemDistance / 4.f, 40);
		
		// draw segments
	
		const float anglePerSegment = Calc::m2PI / segments.size();
		const float anglePerSegmentToDraw = Calc::m2PI / (segments.size() + 1);
					
		for (size_t segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex)
		{
			auto & segment = segments[segmentIndex];
			
			const float segmentAngle = segmentIndex * anglePerSegment;
			
			for (int itemIndex = segment.items.size() - 1; itemIndex >= 0; --itemIndex)
			{
				gxPushMatrix();
				{
					auto & item = segment.items[itemIndex];
					
					const bool itemIsEnabled =
						item.isEnabled != nullptr
						? item.isEnabled(sceneEditor)
						: true;
					
					const float itemDistance = kFirstItemDistance + itemIndex * kItemSpacing;
					
					const float scale = lerp<float>(.9f, 1.f, segment.hoverAnim);
					gxScalef(scale, scale, 1);
					
					const float angle1 = segmentAngle - anglePerSegmentToDraw / 2.f / 1.1f / itemDistance;
					const float angle2 = segmentAngle + anglePerSegmentToDraw / 2.f / 1.1f / itemDistance;
			
					Color itemColor;
					if (itemIsEnabled)
					{
						itemColor = kDefaultItemColor;
						itemColor = itemColor.interp(kActiveSegmentItemColor, segment.hoverAnim);
						itemColor = itemColor.interp(kHoverItemColor, item.hoverAnim);
					}
					else
					{
						itemColor = kDefaultItemColor_Disabled;
						itemColor = itemColor.interp(kActiveSegmentItemColor_Disabled, segment.hoverAnim);
						itemColor = itemColor.interp(kHoverItemColor_Disabled, item.hoverAnim);
					}
					
					setColor(itemColor);
					
					gxBegin(GX_TRIANGLE_STRIP);
					{
						const int resolution = 20;
						const float radius1 = itemDistance - kItemSpacing / 2.1f;
						const float radius2 = itemDistance + kItemSpacing / 2.1f;
						
						for (int i = 0; i <= resolution; ++i)
						{
							const float angle = lerp<float>(angle1, angle2, i / float(resolution));
							
							gxVertex2f(cosf(angle) * radius1, sinf(angle) * radius1);
							gxVertex2f(cosf(angle) * radius2, sinf(angle) * radius2);
						}
					}
					gxEnd();
					
					gxPushMatrix();
					{
						const float textDistance = itemDistance;
						
						gxTranslatef(
							cosf(segmentAngle) * textDistance,
							sinf(segmentAngle) * textDistance,
							-.01f);
						
						const float textAngle =
							sinf(segmentAngle) < .01f
							? segmentAngle * Calc::rad2deg + 90
							: segmentAngle * Calc::rad2deg - 90;
							
						gxRotatef(textAngle, 0, 0, 1);
						
						pushBlend(BLEND_ALPHA); // todo : draw text using a batch. move blend scope surrounding batch
						setColor(colorRed);
						drawText(0, 0, .2f, 0, 0, "%s", item.text.c_str());
						popBlend();
					}
					gxPopMatrix();
				}
				gxPopMatrix();
			}
		}
		
	#if defined(DEBUG) && false
		gxPushMatrix();
		{
			gxTranslatef(0, 0, -.02f);
			
			setColor(colorRed);
			drawLine(0, 0, cosf(debug.angle), sinf(debug.angle));
			
			pushBlend(BLEND_ALPHA);
			{
				setColor(colorBlue);
				drawText(0, 1, .1f, 0, 0, "%.2f", debug.angle);
			}
			popBlend();
		}
		gxPopMatrix();
	#endif
		
		popFontMode();
	}
	gxPopMatrix();
	
	popCullMode();
}

void InteractiveRing::calculateTransform(Mat4x4 & out_transform) const
{
	out_transform =
		Mat4x4(true)
		.Mul(transform)
		.Mul(rotation)
		.Scale(openAnim, openAnim, 1)
		.Scale(1, -1, 1);
}
