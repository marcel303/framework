#ifdef IPHONEOS
	#include <QuartzCore/QuartzCore.h>
#endif

#include "Application.h"
#include "Benchmark.h"
#include "BlitTransform.h"
#include "Brush_Pattern.h"
#include "Calc.h"
#include "CommandPacket.h"
#include "DataStream.h"
#include "ExceptionLogger.h"
#include "FileArchive.h"
#include "FileStream.h"
#include "ImageDescription.h"
#include "KlodderSystem.h"
#include "LayerMgr.h"
#include "Log.h"
#include "Path.h"
#include "Settings.h"
#include "StreamProvider.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"
#include "XmlReader.h"
#include "XmlWriter.h"

#if defined(IPAD)
#define MAX_UNDO_BYTES (1024 * 1024 * 32)
#elif defined(IPHONEOS)
#define MAX_UNDO_BYTES (1024 * 1024 * 4 * (scale > 1 ? 6 : 1))
#else
#define MAX_UNDO_BYTES (1024 * 1024 * 32)
#endif
#define MAX_SWATCHES 100

Application::Application()
{
	// setup
	mIsSetup = false;
	mStreamProvider = 0;
	mScale = 1;
	mBackColor1 = Rgba_Make(0.0f, 0.0f, 0.0f);
	mBackColor2 = Rgba_Make(0.2f, 0.0f, 0.0f);

	// change notification
	OnChange = CallBack();
	
	// canvas
	mLayerMgr = new LayerMgr();
	mLayerMgr->Setup(MAX_LAYERS, 0, 0, Rgba_Make(0.0f, 0.0f, 0.0f), Rgba_Make(0.0f, 0.0f, 0.0f));
	
	// command execution
	//mLastColorCommand;
	//mLastToolSelectCommand;
	
	// editing
	mToolType = ToolType_Undefined;
	mToolActive = false;
	mBezierTraveller.Setup(1.0f, HandleBezierTravel, 0, this);
	mTraveller.Setup(1.0f, HandleTravel, this);
	mStrokeSmooth = false;
	mStrokeMirrorX = false;
	mDirtyArea_Total.Reset();
	
	// undo
	mUndoEnabled = false;
	
	// tools
	//mToolSettings_*
	mStrokeInterval = 1.0f;
	
	// brushes
	mBrushLibrarySet.Add(&mBrushLibrary_Standard, false);
	mBrushLibrarySet.Add(&mBrushLibrary_Custom, false);
	mBrushLibrarySet.Add(&mBrushLibrary_Image, false);
	mBrushPattern = 0;
	
	// swatches
	mSwatchMgr.Capacity_set(MAX_SWATCHES);
	
	// IO
	mImageId.Reset();
	mIsActive = false;
	mWriteEnabled = false;
	
	// streams
	mCommandStream = 0;
	mCommandStreamPosition = 0;
	mCommandStreamWriter = 0;
	mDataStream = 0;
	mDataStreamPosition = 0;
	mDataStreamReader = 0;
	mDataStreamWriter = 0;
}

Application::~Application()
{
	Assert(!mIsActive);
	Assert(!mToolActive);
	Assert(mPacketList.size() == 0);
	
	delete mLayerMgr;
	mLayerMgr = 0;
	
	Assert(mCommandStream == 0);
	Assert(mCommandStreamWriter == 0);
	Assert(mDataStream == 0);
	Assert(mDataStreamReader == 0);
	Assert(mDataStreamWriter == 0);
}

void Application::Setup(StreamProvider* streamProvider, const char* brushLibraryStandard, const char* brushLibraryCustom, int scale, Rgba backColor1, Rgba backColor2)
{
	mIsSetup = true;
	mStreamProvider = streamProvider;
	mScale = scale;
	mBackColor1 = backColor1;
	mBackColor2 = backColor2;

	SetupBrushLibraries(brushLibraryStandard, brushLibraryCustom);
	
	mLastColorCommand = CommandPacket::Make_ColorSelect(0.0f, 0.0f, 0.0f, 1.0f);
	mLastToolSelectCommand = CommandPacket::Make_ToolSelect_SoftBrush(31, 0.8f, 0.05f);
	
	mUndoStack.Initialize(10, MAX_UNDO_BYTES);
}

// -------------------
// Change notification
// -------------------

void Application::SignalChange(ChangeType type)
{
	if (OnChange.IsSet())
	{
		OnChange.Invoke(&type);
	}
}

void Application::HandleTouchZoomStateChange(void* obj, void* arg)
{
	TouchZoomEvent* e = (TouchZoomEvent*)arg;
	
	switch (e->oldState)
	{
		case TouchZoomState_Idle:
			break;
		case TouchZoomState_Draw:
			break;
		case TouchZoomState_SwipeAndZoom:
			break;
		default:
			break;
	}
	
	switch (e->newState)
	{
		case TouchZoomState_Idle:
			break;
		case TouchZoomState_Draw:
			break;
		case TouchZoomState_SwipeAndZoom:
			break;
		default:
			break;
	}
}

// ------
// Canvas
// ------

LayerMgr* Application::LayerMgr_get()
{
	return mLayerMgr;
}

// -----------------
// Command execution
// -----------------

void Application::Execute(const CommandPacket& packet)
{
	Assert(mIsSetup);
	Assert(mIsActive);
		
	switch (packet.mCommandType)
	{
		case CommandType_Undefined:
		{
			throw ExceptionVA("command type not set");
			break;
		}
			
		case CommandType_ColorSelect:
		{
			mLastColorCommand = packet;
			ExecuteColorSelect(
				packet.color.rgba[0],
				packet.color.rgba[1],
				packet.color.rgba[2],
				packet.color.rgba[3]);
			break;
		}
		case CommandType_ImageSize:
		{
			ExecuteImageSize(
				packet.image_size.layerCount,
				mScale * packet.image_size.sx,
				mScale * packet.image_size.sy);
			break;
		}
		case CommandType_LayerBlit:
		{
			BlitTransform transform;
			transform.anchorX = packet.layer_blit.transform.anchorX;
			transform.anchorY = packet.layer_blit.transform.anchorY;
			transform.angle = packet.layer_blit.transform.angle;
			transform.scale = packet.layer_blit.transform.scale * (float)mScale;
			transform.x = packet.layer_blit.transform.x * (float)mScale;
			transform.y = packet.layer_blit.transform.y * (float)mScale;
			ExecuteDataLayerBlit(
				packet.layer_blit.index, transform);
			break;
		}
		case CommandType_LayerClear:
		{
			ExecuteDataLayerClear(
				packet.layer_clear.index,
				packet.layer_clear.rgba[0],
				packet.layer_clear.rgba[1],
				packet.layer_clear.rgba[2],
				packet.layer_clear.rgba[3]);
			break;
		}
		case CommandType_LayerMerge:
			ExecuteDataLayerMerge(
				packet.layer_merge.index1,
				packet.layer_merge.index2);
			break;
		case CommandType_LayerSelect:
		{
			ExecuteDataLayerSelect(
				packet.layer_select.index);
			break;
		}
		case CommandType_LayerOpacity:
		{
			ExecuteDataLayerOpacity(
				packet.layer_opacity.index,
				packet.layer_opacity.opacity);
			break;
		}
		case CommandType_LayerOrder:
		{
			std::vector<int> layerOrder;
			for (uint32_t i = 0; i < packet.layer_order.count; ++i)
				layerOrder.push_back(packet.layer_order.order[i]);
			ExecuteLayerOrder(
				layerOrder);
			break;
		}
		case CommandType_LayerVisibility:
		{
			ExecuteDataLayerVisibility(
				packet.layer_visibility.index,
				packet.layer_visibility.visibility != 0);
			break;
		}
		case CommandType_Stroke_Begin:
		{
			ExecuteStrokeBegin(
				packet.stroke_begin.index,
				(packet.stroke_begin.flags & CommandStrokeFlag_Smooth) != 0,
				(packet.stroke_begin.flags & CommandStrokeFlag_MirrorX) != 0,
				(float)mScale * packet.stroke_begin.x,
				(float)mScale * packet.stroke_begin.y);
			break;
		}
		case CommandType_Stroke_End:
		{
			ExecuteStrokeEnd();
			break;
		}
		case CommandType_Stroke_Move:
		{
			ExecuteStrokeMove(
				mScale * packet.stroke_move.x,
				mScale * packet.stroke_move.y);
			break;
		}
		case CommandType_ToolSelect_PatternBrush:
		{
			mLastToolSelectCommand = packet;
			ExecuteToolSelect_PatternBrush(
				mScale * packet.brush_pattern.diameter,
				packet.brush_pattern.pattern_id,
				packet.brush_pattern.spacing);
			break;
		}
		case CommandType_ToolSelect_PatternBrushDirect:
		{
			mLastToolSelectCommand = packet;
			ExecuteToolSelect_PatternBrushDirect(
				mScale * packet.brush_pattern_direct.diameter,
				packet.brush_pattern_direct.pattern_id,
				packet.brush_pattern_direct.spacing);
			break;
		}
		case CommandType_ToolSelect_PatternEraser:
		{
			mLastToolSelectCommand = packet;
			ExecuteToolSelect_PatternEraser(
				mScale * packet.eraser_pattern.diameter,
				packet.eraser_pattern.pattern_id,
				packet.eraser_pattern.spacing);
			break;
		}
		case CommandType_ToolSelect_PatternSmudge:
		{
			mLastToolSelectCommand = packet;
			ExecuteToolSelect_PatternSmudge(
				mScale * packet.smudge_pattern.diameter,
				packet.smudge_pattern.pattern_id,
				packet.smudge_pattern.spacing,
				packet.smudge_pattern.strength);
			break;
		}
		case CommandType_ToolSelect_SoftBrush:
		{
			mLastToolSelectCommand = packet;
			ExecuteToolSelect_SoftBrush(
				mScale * packet.brush_soft.diameter,
				packet.brush_soft.hardness,
				packet.brush_soft.spacing);
			break;
		}
		case CommandType_ToolSelect_SoftBrushDirect:
		{
			mLastToolSelectCommand = packet;
			ExecuteToolSelect_SoftBrushDirect(
				mScale * packet.brush_soft_direct.diameter,
				packet.brush_soft_direct.hardness,
				packet.brush_soft_direct.spacing);
			break;
		}
		case CommandType_ToolSelect_SoftEraser:
		{
			mLastToolSelectCommand = packet;
			ExecuteToolSelect_SoftEraser(
				mScale * packet.eraser_soft.diameter,
				packet.eraser_soft.hardness,
				packet.eraser_soft.spacing);
			break;
		}
		case CommandType_ToolSelect_SoftSmudge:
		{
			mLastToolSelectCommand = packet;
			ExecuteToolSelect_SoftSmudge(
				mScale * packet.smudge_soft.diameter,
				packet.smudge_soft.hardness,
				packet.smudge_soft.spacing,
				packet.smudge_soft.strength);
			break;
		}
		default:
			throw ExceptionVA("unknown packet type: %d", (int)packet.mCommandType);
	}
}

void Application::ExecuteAndSave(const CommandPacket& packet)
{
	if (mWriteEnabled)
	{
		mPacketList.push_back(packet);
	}
	
	try
	{
		Execute(packet);
	}
	catch (std::exception& e)
	{
		mPacketList.pop_back();
		
		throw e;
	}
}

void Application::ExecutionCommit()
{
	LOG_DBG("exec commit", 0);
	
	Assert(mIsActive);
	
	if (mPacketList.empty())
		return;
	
	if (!mWriteEnabled)
	{
		Assert(mPacketList.empty());
		if (!mPacketList.empty())
			mPacketList.clear();
		return;
	}
	
	Assert(mCommandStream->Position_get() == mCommandStreamPosition);
	
	for (size_t i = 0; i < mPacketList.size(); ++i)
		mCommandStreamWriter->Record(mPacketList[i]);
	
	mPacketList.clear();
	
	mCommandStreamPosition = mCommandStream->Position_get();
}

void Application::ExecutionDiscard()
{
	LOG_DBG("exec discard", 0);
	
	if (!mPacketList.empty())
		mPacketList.clear();
}

void Application::ExecuteColorSelect(float r, float g, float b, float a)
{
	LOG_DBG("exec: colorSelect: %f, %f, %f, %f", r, g, b, a);
	
	Assert(mIsActive);
	Assert(r >= 0.0f && r <= 1.0f);
	Assert(g >= 0.0f && g <= 1.0f);
	Assert(b >= 0.0f && b <= 1.0f);
	Assert(a >= 0.0f && a <= 1.0f);
	
	BrushColor_set(Rgba_Make(r, g, b));
	BrushOpacity_set(a);
	
	SignalChange(ChangeType_Color);
}

void Application::ExecuteImageSize(int layerCount, int sx, int sy)
{	
	LOG_DBG("exec: imageSize: %d @ %dx%d", layerCount, sx, sy);
	
//	Assert(mIsActive);
	Assert(layerCount > 0);
	Assert(sx > 0);
	Assert(sy > 0);

	mLayerMgr->Setup(layerCount, sx, sy, mBackColor1, mBackColor2);

	Invalidate();
}

void Application::ExecuteDataLayerBlit(int index, const BlitTransform& transform)
{
	LOG_DBG("exec: layerBlit: %d", index);
	
	Assert(mIsActive);
	
	MacImage image;
	
	// blit
	
	Vec2I size = mLayerMgr->Size_get();
	
	UndoBuffer* undo = new UndoBuffer();
	
	MacImage* dst = mLayerMgr->DataLayer_get(index);
	
	if (mUndoEnabled)
		undo->mPrev.SetDataLayerImage(index, dst, 0, 0, size[0], size[1]);
	if (mWriteEnabled)
		undo->mPrev.SetCommandStreamLocation(mCommandStream->Position_get());
	if (mWriteEnabled)
		undo->mPrev.SetDataStreamLocation(mDataStreamPosition);
	undo->mPrev.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	
	// load image data from data stream
	
	DataStreamOpen();
	DataHeader header = mDataStreamReader->ReadHeader();
	DataSegment* segment = mDataStreamReader->ReadSegment(header);
	image.Load(&segment->mData);
	delete segment;
	DataStreamClose();
	
	mLayerMgr->DataLayerAcquireWithTransform(index, &image, transform);
	
	ExecutionCommit();
	
	if (mUndoEnabled)
		undo->mNext.SetDataLayerImage(index, dst, 0, 0, size[0], size[1]);
	if (mWriteEnabled)
		undo->mNext.SetCommandStreamLocation(mCommandStream->Position_get());
	if (mWriteEnabled)
		undo->mNext.SetDataStreamLocation(mDataStreamPosition);
	undo->mNext.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	
	CommitUndoBuffer(undo);
	
	Invalidate();
}

void Application::ExecuteDataLayerClear(int index, float r, float g, float b, float a)
{
	LOG_DBG("exec: layerClear: %d @ %f, %f, %f, %f", index, r, g, b, a);
	
	Assert(mIsActive);
	Assert(r >= 0.0f && r <= 1.0f);
	Assert(g >= 0.0f && g <= 1.0f);
	Assert(b >= 0.0f && b <= 1.0f);
	Assert(a >= 0.0f && a <= 1.0f);
	
	// convert RGB to premultiplied values

	r *= a;
	g *= a;
	b *= a;

	//
	
	Vec2I size = mLayerMgr->Size_get();

	UndoBuffer* undo = new UndoBuffer();
	
	if (mWriteEnabled)
		undo->mPrev.SetCommandStreamLocation(mCommandStream->Position_get());
	if (mUndoEnabled)
		undo->mPrev.SetDataLayerImage(index, mLayerMgr->DataLayer_get(index), 0, 0, size[0], size[1]);
	undo->mPrev.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	
	//
	
	Rgba color = Rgba_Make(r, g, b, a);
	
	mLayerMgr->DataLayerClear(index, color);
	
	ExecutionCommit();

	//
	
	if (mWriteEnabled)
		undo->mNext.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mNext.SetDataLayerClear(index, color);
	undo->mNext.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	
	CommitUndoBuffer(undo);
	
	//
	
	Invalidate();
}

void Application::ExecuteDataLayerMerge(int index1, int index2)
{
	LOG_DBG("exec: dataLayerMerge: %d + %d", index1, index2);

	Assert(mIsActive);
	Assert(index1 >= 0 && index1 < mLayerMgr->LayerCount_get());
	Assert(index2 >= 0 && index2 < mLayerMgr->LayerCount_get());
	Assert(index1 != index2);
	
	UndoBuffer* undo = new UndoBuffer();

	if (mWriteEnabled)
		undo->mPrev.SetCommandStreamLocation(mCommandStream->Position_get());
	if (mUndoEnabled)
		undo->mPrev.SetDataLayerDualImage(index1, mLayerMgr->DataLayer_get(index1), index2, mLayerMgr->DataLayer_get(index2));
	undo->mPrev.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());

	mLayerMgr->DataLayerMerge(index1, index2);
	
	ExecutionCommit();

	//
	
	if (mWriteEnabled)
		undo->mNext.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mNext.SetDataLayerMerge(index1, index2);
	undo->mNext.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());

	CommitUndoBuffer(undo);

	//
	
	Invalidate();
}

void Application::ExecuteDataLayerSelect(int index)
{
	LOG_DBG("exec: dataLayerSelect: %d", index);
	
	Assert(mIsActive);
	Assert(index >= 0 && index < mLayerMgr->LayerCount_get());

	UndoBuffer* undo = new UndoBuffer();
	
	if (mWriteEnabled)
		undo->mPrev.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mPrev.SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	undo->mPrev.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	
	//
	
	mLayerMgr->ActiveDataLayer_set(index);
	
	ExecutionCommit();

	//
	
	if (mWriteEnabled)
		undo->mNext.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mNext.SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	undo->mNext.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	
	delete undo;
	undo = 0;
	//CommitUndoBuffer(undo);
	
	//
	
	Invalidate();
}

void Application::ExecuteDataLayerOpacity(int index, float opacity)
{
	LOG_DBG("exec: layerOpacity: %d = %f", index, opacity);

	Assert(mIsActive);
	
	UndoBuffer* undo = new UndoBuffer();
	
	if (mWriteEnabled)
		undo->mPrev.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mPrev.SetDataLayerOpacity(index, mLayerMgr->DataLayerOpacity_get(index));
	undo->mPrev.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());

	//
	
	mLayerMgr->DataLayerOpacity_set(index, opacity);

	ExecutionCommit();
	
	if (mWriteEnabled)
		undo->mNext.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mNext.SetDataLayerOpacity(index, mLayerMgr->DataLayerOpacity_get(index));
	undo->mNext.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	
	CommitUndoBuffer(undo);
	
	//
	
	Invalidate();
}

void Application::ExecuteLayerOrder(std::vector<int> layerOrder)
{
	LOG_DBG("exec: layerOrder: %lu", layerOrder.size());
	for (size_t i = 0; i < layerOrder.size(); ++i)
		LOG_DBG("%lu = %d", i, layerOrder[i]);
	
	Assert(mIsActive);
	Assert((int)layerOrder.size() == mLayerMgr->LayerCount_get());

	UndoBuffer* undo = new UndoBuffer();
	
	if (mWriteEnabled)
		undo->mPrev.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mPrev.SetLayerOrder(mLayerMgr->LayerOrder_get());
	undo->mPrev.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());

	//
	
	mLayerMgr->LayerOrder_set(layerOrder);

	ExecutionCommit();
	
	if (mWriteEnabled)
		undo->mNext.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mNext.SetLayerOrder(mLayerMgr->LayerOrder_get());
	undo->mNext.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	
	CommitUndoBuffer(undo);
	
	//
	
	Invalidate();
}

void Application::ExecuteDataLayerVisibility(int index, bool visibility)
{
	LOG_DBG("exec: layerVisibility: %d = %d", index, visibility ? 1 : 0);

	Assert(mIsActive);
	
	UndoBuffer* undo = new UndoBuffer();
	
	if (mWriteEnabled)
		undo->mPrev.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mPrev.SetDataLayerVisibility(index, mLayerMgr->DataLayerVisibility_get(index));
	undo->mPrev.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());

	//
	
	mLayerMgr->DataLayerVisibility_set(index, visibility);

	ExecutionCommit();
	
	if (mWriteEnabled)
		undo->mNext.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mNext.SetDataLayerVisibility(index, mLayerMgr->DataLayerVisibility_get(index));
	undo->mNext.DBG_SetActiveDataLayer(mLayerMgr->ActiveDataLayer_get());
	
	CommitUndoBuffer(undo);
	
	//
	
	Invalidate();
}

void Application::ExecuteStrokeBegin(int index, bool smooth, bool mirrorX, float x, float y)
{
	LOG_DBG("exec: strokeBegin: %d @ %f, %f", index, x, y);
	
	Assert(mIsActive);
	Assert(!mToolActive);
	Assert(mLayerMgr->ActiveDataLayer_get() == index);
	
	if (mLayerMgr->ActiveDataLayer_get() != index)
	{
		Assert(false);
		mLayerMgr->ActiveDataLayer_set(index);
	}
	
	mStrokeSmooth = smooth;
	mStrokeMirrorX = mirrorX;
	
	if (mStrokeSmooth)
		mBezierTraveller.Begin(x, y);
	else
		mTraveller.Begin(x, y);
	
	mToolActive = true;
}

void Application::ExecuteStrokeEnd()
{
	LOG_DBG("exec: strokeEnd", 0);
	
	Assert(mIsActive);
	Assert(mToolActive);
	
	mToolActive = false;
	
	if (mStrokeSmooth)
		mBezierTraveller.End(mBezierTraveller.LastLocation_get()[0], mBezierTraveller.LastLocation_get()[1]);
	else
		mTraveller.End(mTraveller.m_Coord[0], mTraveller.m_Coord[1]);
	
//	const int layer = mLayerMgr->ActiveLayer_get();
	const int index = mLayerMgr->ActiveDataLayer_get();
	
	// create undo buffer
	
	UndoBuffer* undo = new UndoBuffer();
	
	if (mDirtyArea_Total.IsSet_get())
	{
		RectI rect = mDirtyArea_Total.ToRectI();
		
		if (mUndoEnabled)
			undo->mPrev.SetDataLayerImage(index, mLayerMgr->DataLayer_get(index), rect.m_Position[0], rect.m_Position[1], rect.m_Size[0], rect.m_Size[1]);
		
		// update primary bitmap
		
		if (mToolType == ToolType_SoftBrush || mToolType == ToolType_PatternBrush)
		{
			LayerMgr_get()->FlattenBrush(rect);
		}
		if (mToolType == ToolType_SoftEraser || mToolType == ToolType_PatternEraser)
		{
			LayerMgr_get()->FlattenEraser(rect);
		}
		
		LayerMgr_get()->ValidateLayer(rect.m_Position[0], rect.m_Position[1], rect.m_Size[0], rect.m_Size[1]);
		
		if (mUndoEnabled)
			undo->mNext.SetDataLayerImage(index, mLayerMgr->DataLayer_get(index), rect.m_Position[0], rect.m_Position[1], rect.m_Size[0], rect.m_Size[1]);
		
		mDirtyArea_Total.Reset();
	}
	
	if (mWriteEnabled)
		undo->mPrev.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mPrev.DBG_SetActiveDataLayer(index);
	
	ExecutionCommit();
	
	if (mWriteEnabled)
		undo->mNext.SetCommandStreamLocation(mCommandStream->Position_get());
	undo->mNext.DBG_SetActiveDataLayer(index);
	
	CommitUndoBuffer(undo);
}

void Application::ExecuteStrokeMove(float x, float y)
{
	LOG_DBG("exec: strokeMove: %f, %f", x, y);
	
	Assert(mIsActive);
	Assert(mToolActive);
	
	if (mStrokeSmooth)
		mBezierTraveller.Update(x, y);
	else
		mTraveller.Update(x, y);
}

void Application::ExecuteToolSelect_SoftBrush(int diameter, float hardness, float spacing)
{
	LOG_DBG("exec: toolSelect_SoftBrush: %d, %f, %f", diameter, hardness, spacing);
	
	Assert(mIsActive);
	Assert(diameter > 0);
	Assert(hardness >= 0.0f && hardness <= 1.0f);
	Assert(spacing >= 0.0f);
	
	mToolSettings_BrushSoft = ToolSettings_BrushSoft(diameter, hardness, spacing);
	
	ToolType_set(ToolType_SoftBrush);
	mToolBrush.Setup(diameter, hardness, false);
	StrokeInterval_set(diameter * spacing);
	StrokeIsOriented_set(false);
}

void Application::ExecuteToolSelect_PatternBrush(int diameter, uint32_t patternId, float spacing)
{
	LOG_DBG("exec: toolSelect_PatternBrush: %d, %lu, %f", diameter, patternId, spacing);
	
	Assert(mIsActive);
	Assert(diameter > 0);
	Assert(spacing >= 0.0f);
	
	mToolSettings_BrushPattern = ToolSettings_BrushPattern(diameter, patternId, spacing);
	
	ToolType_set(ToolType_PatternBrush);
	BrushPattern_set(patternId);
	mToolBrush.Setup_Pattern(diameter, &mBrushPattern->mFilter, mBrushPattern->mIsOriented);
	StrokeInterval_set(diameter * spacing);
	StrokeIsOriented_set(mBrushPattern->mIsOriented);
}

void Application::ExecuteToolSelect_SoftBrushDirect(int diameter, float hardness, float spacing)
{
	LOG_DBG("exec: toolSelect_SoftBrushDirect: %d, %f, %f", diameter, hardness, spacing);
	
	Assert(mIsActive);
	Assert(diameter > 0);
	Assert(hardness >= 0.0f && hardness <= 1.0f);
	Assert(spacing >= 0.0f);
	
	mToolSettings_BrushSoftDirect = ToolSettings_BrushSoftDirect(diameter, hardness, spacing);
	
	ToolType_set(ToolType_SoftBrushDirect);
	mToolBrushDirect.Setup(diameter, hardness, false);
	StrokeInterval_set(diameter * spacing);
	StrokeIsOriented_set(false);
}

void Application::ExecuteToolSelect_PatternBrushDirect(int diameter, uint32_t patternId, float spacing)
{
	LOG_DBG("exec: toolSelect_PatternBrushDirect: %d, %lu, %f", diameter, patternId, spacing);
	
	Assert(mIsActive);
	Assert(diameter > 0);
	Assert(spacing >= 0.0f);
	
	mToolSettings_BrushPatternDirect = ToolSettings_BrushPatternDirect(diameter, patternId, spacing);
	
	ToolType_set(ToolType_PatternBrushDirect);
	BrushPattern_set(patternId);
	mToolBrushDirect.Setup_Pattern(diameter, &mBrushPattern->mFilter, mBrushPattern->mIsOriented);
	StrokeInterval_set(diameter * spacing);
	StrokeIsOriented_set(mBrushPattern->mIsOriented);
}

void Application::ExecuteToolSelect_SoftSmudge(int diameter, float hardness, float spacing, float strength)
{
	LOG_DBG("exec: toolSelect_SoftSmudge: %d, %f, %f %f", diameter, hardness, spacing, strength);
	
	Assert(mIsActive);
	Assert(diameter > 0);
	Assert(hardness >= 0.0f && hardness <= 1.0f);
	Assert(spacing >= 0.0f);
	Assert(strength >= 0.0f && strength <= 1.0f);
	
	mToolSettings_SmudgeSoft = ToolSettings_SmudgeSoft(diameter, hardness, spacing, strength);
	
	ToolType_set(ToolType_SoftSmudge);
	mToolBrush.Setup(diameter, hardness, false);
	mToolSmudge.Setup(strength);
	StrokeInterval_set(diameter * spacing);
	StrokeIsOriented_set(false);
}

void Application::ExecuteToolSelect_PatternSmudge(int diameter, uint32_t patternId, float spacing, float strength)
{
	LOG_DBG("exec: toolSelect_PatternSmudge: %d, %lu, %f, %f", diameter, patternId, spacing, strength);
	
	Assert(mIsActive);
	Assert(diameter > 0);
	Assert(spacing >= 0.0f);
	Assert(strength >= 0.0f && strength <= 1.0f);
	
	mToolSettings_SmudgePattern = ToolSettings_SmudgePattern(diameter, patternId, spacing, strength);
	
	ToolType_set(ToolType_PatternSmudge);
	BrushPattern_set(patternId);
	mToolBrush.Setup_Pattern(diameter, &mBrushPattern->mFilter, mBrushPattern->mIsOriented);
	mToolSmudge.Setup(strength);
	StrokeInterval_set(diameter * spacing);
	StrokeIsOriented_set(mBrushPattern->mIsOriented);
}

void Application::ExecuteToolSelect_SoftEraser(int diameter, float hardness, float spacing)
{
	LOG_DBG("exec: toolSelect_SoftEraser: %d, %f, %f", diameter, hardness, spacing);
	
	Assert(mIsActive);
	Assert(diameter > 0);
	Assert(hardness >= 0.0f && hardness <= 1.0f);
	Assert(spacing >= 0.0f);
	
	mToolSettings_EraserSoft = ToolSettings_EraserSoft(diameter, hardness, spacing);
	
	ToolType_set(ToolType_SoftEraser);
	mToolBrush.Setup(diameter, hardness, false);
	StrokeInterval_set(diameter * spacing);
	StrokeIsOriented_set(false);
}

void Application::ExecuteToolSelect_PatternEraser(int diameter, uint32_t patternId, float spacing)
{
	LOG_DBG("exec: toolSelect_PatternEraser: %d, %lu, %f", diameter, patternId, spacing);
	
	Assert(mIsActive);
	Assert(diameter > 0);
	Assert(spacing >= 0.0f);
	
	mToolSettings_EraserPattern = ToolSettings_EraserPattern(diameter, patternId, spacing);
	
	ToolType_set(ToolType_PatternEraser);
	BrushPattern_set(patternId);
	mToolBrush.Setup_Pattern(diameter, &mBrushPattern->mFilter, mBrushPattern->mIsOriented);
	StrokeInterval_set(diameter * spacing);
	StrokeIsOriented_set(mBrushPattern->mIsOriented);
}

// -------
// Editing
// -------

ToolType Application::ToolType_get() const
{
	return mToolType;
}

void Application::ToolType_set(ToolType type)
{
	mToolType = type;

	switch (type)
	{
		case ToolType_SoftBrushDirect:
		case ToolType_PatternBrushDirect:
		case ToolType_SoftSmudge:
		case ToolType_PatternSmudge:
			mLayerMgr->SetMode_Direct();
			break;

		case ToolType_SoftBrush:
		case ToolType_PatternBrush:
			mLayerMgr->SetMode_Brush();
			break;
				
		case ToolType_SoftEraser:
		case ToolType_PatternEraser:
			mLayerMgr->SetMode_Eraser();
			break;

		default:
			throw ExceptionVA("unknown tool type: %d", (int)type);
	}
}

void Application::HandleBezierTravel(void* obj, BezierTravellerState state, float x, float y)
{
	Application* self = (Application*)obj;
	
	if (state == BezierTravellerState_Update)
		self->mTraveller.Update(x, y);
	else if (state == BezierTravellerState_Begin)
		self->mTraveller.Begin(x, y);
	else if (state == BezierTravellerState_End)
		self->mTraveller.End(x, y);
}

void Application::HandleTravel(void* obj, const TravelEvent& e)
{
	Application* self = (Application*)obj;

	self->DoPaint(e.x, e.y, e.dx, e.dy);
	
	if (self->mStrokeMirrorX)
	{
		self->DoPaint(self->LayerMgr_get()->Size_get()[0] - e.x, e.y, -e.dx, e.dy);
	}
}

void Application::StrokeBegin(int index, bool smooth, bool mirrorX, float x, float y)
{
#ifdef DEBUG
//	smooth = false;
#endif
	
	Assert(mLastColorCommand.mCommandType != CommandType_Undefined);
	Assert(mLastToolSelectCommand.mCommandType != CommandType_Undefined);
	
	CommandPacket packet = CommandPacket::Make_StrokeBegin(index, smooth, mirrorX, x, y);
	
	ExecuteAndSave(mLastColorCommand);
	ExecuteAndSave(mLastToolSelectCommand);
	ExecuteAndSave(packet);
}

void Application::StrokeEnd()
{
	CommandPacket packet = CommandPacket::Make_StrokeEnd();
	
	ExecuteAndSave(packet);
}

void Application::StrokeMove(float x, float y)
{
	CommandPacket packet = CommandPacket::Make_StrokeMove(x, y);
	
	ExecuteAndSave(packet);
}

void Application::StrokeCancel()
{
	Assert(mToolActive);
	
	mToolActive = false;
	
	if (mStrokeSmooth)
		mBezierTraveller.End(mBezierTraveller.LastLocation_get()[0], mBezierTraveller.LastLocation_get()[1]);
	else
		mTraveller.End(mTraveller.m_Coord[0], mTraveller.m_Coord[1]);
	
	if (mDirtyArea_Total.IsSet_get())
	{
		RectI rect = mDirtyArea_Total.ToRectI();
		
		if (mToolType == ToolType_SoftBrush || mToolType == ToolType_PatternBrush || mToolType == ToolType_SoftEraser || mToolType == ToolType_PatternEraser)
		{
			LayerMgr_get()->ClearBrushOverlay(rect);
		}
		else
		{
			// copy layer back into editing buffer
			
			mLayerMgr->CopyLayerToEditingBuffer(rect.m_Position[0], rect.m_Position[1], rect.m_Size[0], rect.m_Size[1]);
		}
		
		mLayerMgr->Invalidate(
			mDirtyArea_Total.x1,
			mDirtyArea_Total.y1,
			mDirtyArea_Total.x2 - mDirtyArea_Total.x1 + 1,
			mDirtyArea_Total.y2 - mDirtyArea_Total.y1 + 1);

		mDirtyArea_Total.Reset();
	}
	
	ExecutionDiscard();
}

void Application::Invalidate()
{
	Vec2I size = mLayerMgr->Size_get();

	Invalidate(0, 0, size[0], size[1]);
}

void Application::Invalidate(int x, int y, int sx, int sy)
{
	if (sx == 0 || sy == 0)
		return;
	
	AreaI area(Vec2I(x, y), Vec2I(x + sx - 1, y + sy - 1));
	
	area.Clip(mLayerMgr->EditingBuffer_get()->Area_get());
	
	mLayerMgr->Invalidate(x, y, sx, sy);

	mDirtyArea_Total.Merge(area);
}

void Application::DoPaint(float x, float y, float dx, float dy)
{
//	LOG_DBG("doPaint: %f, %f - %f, %f", x, y, dx, dy);
	
	ITool* tool = Tool_get(mToolType);
	
	switch (mToolType)
	{
		case ToolType_PatternSmudge:
		case ToolType_SoftSmudge:
			{
				tool->Apply(mLayerMgr->EditingBuffer_get(), mToolBrush.Filter_get(), x, y, dx, dy, mLayerMgr->EditingDirty_get());
				break;
			}
		
		case ToolType_PatternBrush:
		case ToolType_SoftBrush:
		case ToolType_PatternEraser:
		case ToolType_SoftEraser:
			{
				tool->ApplyFilter(mLayerMgr->EditingBrush_get(), mToolBrush.Filter_get(), x, y, dx, dy, mLayerMgr->EditingDirty_get());
				break;
			}
			
		case ToolType_PatternBrushDirect:
		case ToolType_SoftBrushDirect:
			{
				// fixme: get brush colour quickly
				// fixme: don't require filter to be passed in
				Rgba color = BrushColor_get();
				color.rgb[3] = BrushOpacity_get();
				color.rgb[0] *= color.rgb[3];
				color.rgb[1] *= color.rgb[3];
				color.rgb[2] *= color.rgb[3];
				tool->ApplyFilter(
					mLayerMgr->EditingBuffer_get(), 
					mToolBrushDirect.Filter_get(),
					x, y, dx, dy,
					color,
					mLayerMgr->EditingDirty_get());
				break;
			}

		default:
			throw ExceptionVA("unknown tool type: %d", (int)mToolType);
	}
	
	if (mLayerMgr->EditingDirty_get().IsSet_get())
	{
		mDirtyArea_Total.Merge(mLayerMgr->EditingDirty_get());
	}
}

// ----
// Undo
// ----

void Application::CommitUndoBuffer(UndoBuffer* undo)
{
	Assert(mPacketList.size() == 0);
	
	if (mUndoEnabled)
	{
		mUndoStack.Commit(undo);
	
		SignalChange(ChangeType_Undo);
	}
	else
	{
		delete undo;
		undo = 0;
	}
}

void Application::UndoEnabled_set(bool enabled)
{
	mUndoEnabled = enabled;
}

bool Application::HasUndo_get() const
{
	return mUndoStack.HasUndo_get();
}

bool Application::HasRedo_get() const
{
	return mUndoStack.HasRedo_get();
}

void Application::Undo()
{
	if (!mUndoStack.HasUndo_get())
	{
		LOG_ERR("undo stack has no undo", 0);
		return;
	}
	
	LOG_DBG("undo", 0);
	
	bool isEditing = mLayerMgr->EditingIsEnabled_get();
	
//	if (isEditing)
//		mLayerMgr->EditingEnd();
	
	UndoBuffer* buffer = mUndoStack.GetPrevBuffer();
	
	bool rebuildCaches = false;
	
	if (buffer->mPrev.mHasImage)
	{
		// restore bitmap data
		
		MacImage* prev = &buffer->mPrev.mImage;
		
		LOG_DBG("undo: update bitmap data: %dx%d", prev->Sx_get(), prev->Sy_get());

		// update layer

		// todo: use (optimized) editing begin/end (with only a partial editing buffer update). code beneath is just hackish
		
		buffer->mPrev.mImage.Blit(
			mLayerMgr->DataLayer_get(buffer->mPrev.mImageDataLayer),
			buffer->mPrev.mImageLocation[0],
			buffer->mPrev.mImageLocation[1]);
		
		if (buffer->mPrev.mImageDataLayer != mLayerMgr->ActiveDataLayer_get())
		{
			rebuildCaches = true;
		}
		else
		{
			// copy layer to editing surface
 			
			mLayerMgr->CopyLayerToEditingBuffer(
				buffer->mPrev.mImageLocation[0],
				buffer->mPrev.mImageLocation[1],
				buffer->mPrev.mImage.Sx_get(),
				buffer->mPrev.mImage.Sy_get());
		}
		
		Invalidate(buffer->mPrev.mImageLocation[0], buffer->mPrev.mImageLocation[1], prev->Sx_get(), prev->Sy_get());
	}
	
	if (buffer->mPrev.mHasDualImage)
	{
		// restore bitmap data
		
		MacImage* prev1 = &buffer->mPrev.mDualImage1;
		MacImage* prev2 = &buffer->mPrev.mDualImage2;
		int index1 = buffer->mPrev.mDualImageDataLayer1;
		int index2 = buffer->mPrev.mDualImageDataLayer2;
		
		LOG_DBG("undo: update bitmap data: %dx%d -> %d", prev1->Sx_get(), prev1->Sy_get(), index1);
		LOG_DBG("undo: update bitmap data: %dx%d -> %d", prev2->Sx_get(), prev2->Sy_get(), index2);
		
		// update layers
		
		prev1->Blit(mLayerMgr->DataLayer_get(index1));
		prev2->Blit(mLayerMgr->DataLayer_get(index2));
		
		rebuildCaches = true;
		
		Invalidate();
	}
	
	Assert(buffer->mPrev.mHasLayerClear == false);
	
	Assert(buffer->mPrev.mHasLayerMerge == false);
	
	if (buffer->mPrev.mHasLayerOrder)
	{
		LOG_DBG("undo: layer order", 0);
		for (size_t i = 0; i < buffer->mPrev.mLayerOrder.size(); ++i)
			LOG_DBG("undo: layer order: %d -> %d", (int)i, buffer->mPrev.mLayerOrder[i]);
		
		mLayerMgr->LayerOrder_set(buffer->mPrev.mLayerOrder);
		
		rebuildCaches = true;

		Invalidate();
	}
	
	if (buffer->mPrev.mHasLayerOpacity)
	{
		LOG_DBG("undo: layer opacity: %d = %f", buffer->mPrev.mLayerOpacityIndex, buffer->mPrev.mLayerOpacityValue);
		
		mLayerMgr->DataLayerOpacity_set(buffer->mPrev.mLayerOpacityIndex, buffer->mPrev.mLayerOpacityValue);

		rebuildCaches = true;
		
		Invalidate();
	}
	
	if (buffer->mPrev.mHasLayerVisibility)
	{
		LOG_DBG("undo: layer visibility: %d = %d", buffer->mPrev.mLayerVisibilityIndex, buffer->mPrev.mLayerVisibilityValue ? 1 : 0);
		
		mLayerMgr->DataLayerVisibility_set(buffer->mPrev.mLayerVisibilityIndex, buffer->mPrev.mLayerVisibilityValue);

		rebuildCaches = true;
		
		Invalidate();
	}
	
	if (buffer->mPrev.mHasActiveDataLayer)
	{
		LOG_DBG("undo: layer select: %d", buffer->mPrev.mActiveDataLayer);
		
		mLayerMgr->ActiveDataLayer_set(buffer->mPrev.mActiveDataLayer);
		
		Invalidate();
	}
	
	if (buffer->mPrev.mHasCommandStreamLocation)
	{
		size_t location = buffer->mPrev.mCommandStreamLocation;
		
		LOG_DBG("undo: update stream location: %d", (int)location);
		
		mCommandStream->Seek(location, SeekMode_Begin);
		mCommandStreamPosition = location;
	}

	if (buffer->mPrev.mHasDataStreamLocation)
	{
		size_t location = buffer->mPrev.mDataStreamLocation;

		LOG_DBG("undo: update data stream location: %lu", location);

		Assert(!mDataStream);
		mDataStreamPosition = location;
	}
	
/*	if (buffer->mPrev.mHasDbgActiveDataLayer)
	{
		if (buffer->mPrev.mDbgActiveDataLayer != mLayerMgr->ActiveDataLayer_get())
			throw ExceptionVA("undo: active layer mismatch");
	}*/

	mUndoStack.Seek(-1);
	
	if (isEditing)
	{
		if (rebuildCaches)
		{
			mLayerMgr->EditingEnd();
			mLayerMgr->EditingBegin(true);
		}
	}
	
	SignalChange(ChangeType_Undo);
}

void Application::Redo()
{
	if (!mUndoStack.HasRedo_get())
	{
		LOG_ERR("undo stack has no redo", 0);
		return;
	}
	
	LOG_DBG("redo", 0);
	
	bool isEditing = mLayerMgr->EditingIsEnabled_get();
	
//	if (isEditing)
//		mLayerMgr->EditingEnd();
	
	mUndoStack.Seek(+1);
	
	UndoBuffer* buffer = mUndoStack.GetPrevBuffer();
	
	Assert(buffer != 0);
	
	bool rebuildCaches = false;
	
	if (buffer->mNext.mHasImage)
	{
		// restore bitmap data
		
		MacImage* curr = &buffer->mNext.mImage;
		
		LOG_DBG("redo: update bitmap data: %dx%d", curr->Sx_get(), curr->Sy_get());
		
		// update layer

		buffer->mNext.mImage.Blit(
			mLayerMgr->DataLayer_get(buffer->mNext.mImageDataLayer),
			buffer->mNext.mImageLocation[0],
			buffer->mNext.mImageLocation[1]);
		
		if (buffer->mNext.mImageDataLayer != mLayerMgr->ActiveDataLayer_get())
		{
			rebuildCaches = true;
		}
		else
		{
			// copy layer to editing surface
 			
			mLayerMgr->CopyLayerToEditingBuffer(
				buffer->mNext.mImageLocation[0],
				buffer->mNext.mImageLocation[1],
				buffer->mNext.mImage.Sx_get(),
				buffer->mNext.mImage.Sy_get());
		}
		
		Invalidate(buffer->mNext.mImageLocation[0], buffer->mNext.mImageLocation[1], curr->Sx_get(), curr->Sy_get());
	}
	
	Assert(buffer->mNext.mHasDualImage == false);
	
	if (buffer->mNext.mHasLayerClear)
	{
		mUndoEnabled = false;
		
		Rgba color = buffer->mNext.mLayerClearColor;
		
		ExecuteDataLayerClear(buffer->mNext.mLayerClearDataLayer, color.rgb[0], color.rgb[1], color.rgb[2], color.rgb[3]);
		
		mUndoEnabled = true;
		
		rebuildCaches = true;
	}
	
	if (buffer->mNext.mHasLayerMerge)
	{
		mUndoEnabled = false;
		
		ExecuteDataLayerMerge(buffer->mNext.mLayerMergeDataLayer1, buffer->mNext.mLayerMergeDataLayer2);
		
		mUndoEnabled = true;
		
		rebuildCaches = true;
	}
	
	if (buffer->mNext.mHasLayerOrder)
	{
		LOG_DBG("redo: layer order", 0);
		for (size_t i = 0; i < buffer->mNext.mLayerOrder.size(); ++i)
			LOG_DBG("redo: layer order: %d -> %d", (int)i, buffer->mNext.mLayerOrder[i]);
		
		mLayerMgr->LayerOrder_set(buffer->mNext.mLayerOrder);
		
		rebuildCaches = true;

		Invalidate();
	}

	if (buffer->mNext.mHasLayerOpacity)
	{
		LOG_DBG("redo: layer opacity: %d = %f", buffer->mNext.mLayerOpacityIndex, buffer->mNext.mLayerOpacityValue);
		
		mLayerMgr->DataLayerOpacity_set(buffer->mNext.mLayerOpacityIndex, buffer->mNext.mLayerOpacityValue);
		
		rebuildCaches = true;

		Invalidate();
	}
	
	if (buffer->mNext.mHasLayerVisibility)
	{
		LOG_DBG("redo: layer visibility: %d = %d", buffer->mNext.mLayerVisibilityIndex, buffer->mNext.mLayerVisibilityValue ? 1 : 0);
		
		mLayerMgr->DataLayerVisibility_set(buffer->mNext.mLayerVisibilityIndex, buffer->mNext.mLayerVisibilityValue);
		
		rebuildCaches = true;

		Invalidate();
	}
	
	if (buffer->mNext.mHasActiveDataLayer)
	{
		LOG_DBG("redo: layer select: %d", buffer->mNext.mActiveDataLayer);
		
		mLayerMgr->ActiveDataLayer_set(buffer->mNext.mActiveDataLayer);

		Invalidate();
	}
	
	if (buffer->mNext.mHasCommandStreamLocation)
	{
		size_t location = buffer->mNext.mCommandStreamLocation;
		
		LOG_DBG("redo: update stream location: %d", (int)location);
		
		mCommandStream->Seek(location, SeekMode_Begin);
		mCommandStreamPosition = location;
	}

	if (buffer->mNext.mHasDataStreamLocation)
	{
		size_t location = buffer->mNext.mDataStreamLocation;

		LOG_DBG("undo: update data stream location: %lu", location);

		Assert(!mDataStream);
		mDataStreamPosition = location;
	}
	
/*	if (buffer->mNext.mHasDbgActiveDataLayer)
	{
		if (buffer->mNext.mDbgActiveDataLayer != mLayerMgr->ActiveDataLayer_get())
			throw ExceptionVA("redo: active layer mismatch");
	}*/

	if (isEditing)
	{
		if (rebuildCaches)
		{
			mLayerMgr->EditingEnd();
			mLayerMgr->EditingBegin(true);
		}
	}
	
	SignalChange(ChangeType_Undo);
}

// -----
// Tools
// -----

void Application::ColorSelect(float r, float g, float b, float a)
{
	CommandPacket packet = CommandPacket::Make_ColorSelect(r, g, b, a);
	
	ExecuteAndSave(packet);
}

void Application::SwitchErazorBrush()
{
	switch (mToolType)
	{
		case ToolType_PatternBrush:
		{
			ToolSettings_EraserPattern settings;
			settings.diameter = mToolSettings_BrushPattern.diameter;
			settings.patternId = mToolSettings_BrushPattern.patternId;
			settings.spacing = mToolSettings_BrushPattern.spacing;
			ToolSelect_EraserPattern(settings);
			break;
		}
		case ToolType_SoftBrush:
		{
			ToolSettings_EraserSoft settings;
			settings.diameter = mToolSettings_BrushSoft.diameter;
			settings.hardness = mToolSettings_BrushSoft.hardness;
			settings.spacing = mToolSettings_BrushSoft.spacing;
			ToolSelect_EraserSoft(settings);
			break;
		}
		case ToolType_PatternBrushDirect:
		{
			ToolSettings_EraserPattern settings;
			settings.diameter = mToolSettings_BrushPattern.diameter;
			settings.patternId = mToolSettings_BrushPattern.patternId;
			settings.spacing = mToolSettings_BrushPattern.spacing;
			ToolSelect_EraserPattern(settings);
			break;
		}
		case ToolType_SoftBrushDirect:
		{
			ToolSettings_EraserSoft settings;
			settings.diameter = mToolSettings_BrushSoft.diameter;
			settings.hardness = mToolSettings_BrushSoft.hardness;
			settings.spacing = mToolSettings_BrushSoft.spacing;
			ToolSelect_EraserSoft(settings);
			break;
		}
		case ToolType_PatternEraser:
		{
			ToolSettings_BrushPattern settings;
			settings.diameter = mToolSettings_EraserPattern.diameter;
			settings.patternId = mToolSettings_EraserPattern.patternId;
			settings.spacing = mToolSettings_EraserPattern.spacing;
			ToolSelect_BrushPattern(settings);
			break;
		}
		case ToolType_SoftEraser:
		{
			ToolSettings_BrushSoft settings;
			settings.diameter = mToolSettings_EraserSoft.diameter;
			settings.hardness = mToolSettings_EraserSoft.hardness;
			settings.spacing = mToolSettings_EraserSoft.spacing;
			ToolSelect_BrushSoft(settings);
			break;
		}
		case ToolType_PatternSmudge:
		{
			ToolSettings_EraserPattern settings;
			settings.diameter = mToolSettings_SmudgePattern.diameter;
			settings.patternId = mToolSettings_SmudgePattern.patternId;
			settings.spacing = mToolSettings_SmudgePattern.spacing;
			ToolSelect_EraserPattern(settings);
			break;
		}
		case ToolType_SoftSmudge:
		{
			ToolSettings_EraserSoft settings;
			settings.diameter = mToolSettings_SmudgeSoft.diameter;
			settings.hardness = mToolSettings_SmudgeSoft.hardness;
			settings.spacing = mToolSettings_SmudgeSoft.spacing;
			ToolSelect_EraserSoft(settings);
			break;
		}
			
		case ToolType_Undefined:
			break;
	}
}

void Application::ImageInitialize(int layerCount, int sx, int sy)
{
	Assert(!mLayerMgr->EditingIsEnabled_get());
	
	bool undoEnabled = mUndoEnabled;
	
	UndoEnabled_set(false);

	ImageSize(layerCount, sx, sy);
//	mLayerMgr->EditingBegin(false);
	DataLayerClear(mLayerMgr->ActiveDataLayer_get(), 1.0f, 1.0f, 1.0f, 1.0f);
	ExecutionCommit();
	ColorSelect(0.1f, 0.1f, 0.5f, 1.0f);
	ToolSettings_BrushSoft settings(7, 0.5f, 0.05f);
	ToolSelect_BrushSoft(settings);
//	mLayerMgr->EditingEnd();

	//
	
	ExecutionCommit();

	UndoEnabled_set(undoEnabled);
}

void Application::ImageSize(int layerCount, int sx, int sy)
{
	CommandPacket packet = CommandPacket::Make_ImageSize(layerCount, sx, sy);

	ExecuteAndSave(packet);
}

void Application::DataLayerBlit(int index, MacImage* src, const BlitTransform& transform)
{
	LOG_DBG("dataLayerBlit: %d", index);
	
	// serialize image data to data stream
	
	DataStreamOpen();
	MemoryStream stream;
	src->Save(&stream);
	uint8_t* bytes = 0;
	int byteCount = 0;
	stream.ToArray(&bytes, &byteCount);
	mDataStreamWriter->WriteSegment("image_raw", "", bytes, byteCount, false);
	delete[] bytes;
	DataStreamClose();

	//
	
	CommandPacket packet = CommandPacket::Make_DataLayerBlit(index, transform);
	
	ExecuteAndSave(packet);
}

void Application::DataLayerClear(int index, float r, float g, float b, float a)
{
	CommandPacket packet = CommandPacket::Make_DataLayerClear(index, r, g, b, a);

	ExecuteAndSave(packet);
}

void Application::DataLayerMerge(int index1, int index2)
{
	CommandPacket packet = CommandPacket::Make_DataLayerMerge(index1, index2);

	ExecuteAndSave(packet);
}

void Application::DataLayerOpacity(int index, float opacity)
{
	CommandPacket packet = CommandPacket::Make_DataLayerOpacity(index, opacity);
	
	ExecuteAndSave(packet);
}

void Application::LayerOrder(std::vector<int> layerOrder)
{
	CommandPacket packet = CommandPacket::Make_LayerOrder(&layerOrder[0], layerOrder.size());
	
	ExecuteAndSave(packet);
}

void Application::DataLayerSelect(int index)
{
	CommandPacket packet = CommandPacket::Make_DataLayerSelect(index);

	ExecuteAndSave(packet);
}

void Application::DataLayerVisibility(int index, bool visibility)
{	
	CommandPacket packet = CommandPacket::Make_DataLayerVisibility(index, visibility);
	
	ExecuteAndSave(packet);
}

void Application::ToolSelect_BrushSoft(ToolSettings_BrushSoft settings)
{
	CommandPacket packet = CommandPacket::Make_ToolSelect_SoftBrush(settings.diameter, settings.hardness, settings.spacing);
	
	ExecuteAndSave(packet);
}

void Application::ToolSelect_BrushPattern(ToolSettings_BrushPattern settings)
{
	CommandPacket packet = CommandPacket::Make_ToolSelect_PatternBrush(settings.patternId, settings.diameter, settings.spacing);

	ExecuteAndSave(packet);
}

void Application::ToolSelect_BrushSoftDirect(ToolSettings_BrushSoftDirect settings)
{
	CommandPacket packet = CommandPacket::Make_ToolSelect_SoftBrushDirect(settings.diameter, settings.hardness, settings.spacing);
	
	ExecuteAndSave(packet);
}

void Application::ToolSelect_BrushPatternDirect(ToolSettings_BrushPatternDirect settings)
{
	CommandPacket packet = CommandPacket::Make_ToolSelect_PatternBrushDirect(settings.patternId, settings.diameter, settings.spacing);

	ExecuteAndSave(packet);
}

void Application::ToolSelect_EraserSoft(ToolSettings_EraserSoft settings)
{
	CommandPacket packet = CommandPacket::Make_ToolSelect_SoftEraser(settings.diameter, settings.hardness, settings.spacing);
	
	ExecuteAndSave(packet);
}

void Application::ToolSelect_EraserPattern(ToolSettings_EraserPattern settings)
{
	CommandPacket packet = CommandPacket::Make_ToolSelect_PatternEraser(settings.patternId, settings.diameter, settings.spacing);
	
	ExecuteAndSave(packet);
}

void Application::ToolSelect_SmudgeSoft(ToolSettings_SmudgeSoft settings)
{
	CommandPacket packet = CommandPacket::Make_ToolSelect_SoftSmudge(settings.diameter, settings.hardness, settings.spacing, settings.strength);
	
	ExecuteAndSave(packet);
}

void Application::ToolSelect_SmudgePattern(ToolSettings_SmudgePattern settings)
{
	CommandPacket packet = CommandPacket::Make_ToolSelect_PatternSmudge(settings.patternId, settings.diameter, settings.spacing, settings.strength);
	
	ExecuteAndSave(packet);
}

void Application::BrushPattern_set(uint32_t patternId)
{
	Brush_Pattern* pattern;

	if (!FindPattern(patternId, &pattern))
	{
		// add brush to image library

		Brush_Pattern* temp = pattern->Duplicate();

		mBrushLibrary_Image.Append(temp, true);

		pattern = temp;
	}

	mBrushPattern = pattern;
}

uint32_t Application::BrushPattern_get() const
{
	if (!mBrushPattern)
	{
#ifdef DEBUG
		throw ExceptionNA();
#else
		return 0;
#endif
	}
	
	return mBrushPattern->mPatternId;
}

void Application::StrokeInterval_set(float interval)
{
	if (interval < 0.5f)
		interval = 0.5f;
	
	mTraveller.m_Step = interval;
	
	mStrokeInterval = interval;
}

float Application::StrokeInterval_get() const
{
	return mStrokeInterval;
}

bool Application::FindPattern(uint32_t patternId, Brush_Pattern** out_pattern)
{
	Brush_Pattern* brush = 0;

	*out_pattern = mBrushLibrary_Image.Find(patternId);

	if (*out_pattern)
		return true;

	// find brush

	*out_pattern = mBrushLibrary_Standard.Find(patternId);

	if (*out_pattern)
		return true;

	*out_pattern = mBrushLibrary_Custom.Find(patternId);

	if (*out_pattern)
		return false;

	if (brush == 0)
		throw ExceptionVA("brush not found: %lu", patternId);

	return false;
}

Rgba Application::BrushColor_get() const
{
	return mLayerMgr->BrushColor_get();
}

void Application::BrushColor_set(Rgba color)
{
	mLayerMgr->BrushColor_set(color);
}

float Application::BrushOpacity_get() const
{
	return mLayerMgr->BrushOpacity_get();
}

void Application::BrushOpacity_set(float opacity)
{
	mLayerMgr->BrushOpacity_set(opacity);
}

void Application::StrokeIsOriented_set(bool isOriented)
{
	mToolBrush.IsOriented_set(isOriented);
	mToolBrushDirect.IsOriented_set(isOriented);
	mTraveller.m_Lag = isOriented;
}

ITool* Application::Tool_get(ToolType type)
{
	switch (type)
	{
	case ToolType_PatternBrush:
		return &mToolBrush;
	case ToolType_PatternBrushDirect:
		return &mToolBrushDirect;
	case ToolType_PatternEraser:
		return &mToolBrush;
	case ToolType_PatternSmudge:
		return &mToolSmudge;
	case ToolType_SoftBrush:
		return &mToolBrush;
	case ToolType_SoftBrushDirect:
		return &mToolBrushDirect;
	case ToolType_SoftEraser:
		return &mToolBrush;
	case ToolType_SoftSmudge:
		return &mToolSmudge;
	default:
		throw ExceptionVA("unknown tool type: %d", (int)type);
	}
}

BrushLibrarySet* Application::BrushLibrarySet_get()
{
	return &mBrushLibrarySet;
}

// -------
// Brushes
// -------

void Application::SetupBrushLibraries(const char* standardFileName, const char* customFileName)
{
	mBrushLibrary_Standard_FileName = standardFileName;
	mBrushLibrary_Custom_FileName = customFileName;
	
	if (FileStream::Exists(mBrushLibrary_Standard_FileName.c_str()))
	{
		FileStream stream;
		stream.Open(mBrushLibrary_Standard_FileName.c_str(), OpenMode_Read);
		mBrushLibrary_Standard.Load(&stream, true);
	}
	else
	{
		mBrushLibrary_Standard.Clear();
		
		throw ExceptionVA("missing standard brush library");
	}

	if (FileStream::Exists(mBrushLibrary_Custom_FileName.c_str()))
	{
		FileStream stream;
		stream.Open(mBrushLibrary_Custom_FileName.c_str(), OpenMode_Read);
		mBrushLibrary_Custom.Load(&stream, true);
	}
	else
	{
		mBrushLibrary_Custom.Clear();
		
		// todo: save brush library
	}
}

void Application::AddBrush(Brush_Pattern* brush)
{
	// todo: create scaled down version of brush
	
	// todo: append brush to custom brush library (LQ)
	
	// todo: append brush to custom brush library (HQ mirror)
}

// ----------
// Eyedropper
// ----------

Rgba Application::GetColorAtLocation(Vec2F location) const
{
	Rgba result;
	mLayerMgr->EditingBuffer_get()->SampleAA(location[0], location[1], result);
	return result;
}

// --------
// Swatches
// --------

SwatchMgr* Application::SwatchMgr_get()
{
	return &mSwatchMgr;
}

// --
// IO
// --

ImageId Application::AllocateImageId()
{
#if KLODDER_LITE==0
	while (true)
	{
		int id = gSettings.GetInt32("curr_image_id", 0);

		ImageId imageId(String::Format("%03d", id).c_str());

		if (!ImageExists(imageId))
			return imageId;
		else
			gSettings.SetInt32("curr_image_id", id + 1);
	}
#else
	int id = 0;
	
	return ImageId(String::Format("%03d", id).c_str());
#endif
}

bool Application::ImageExists(ImageId imageId)
{
	std::string path = gSystem.GetDocumentPath(imageId.mName.c_str());

	std::string fileName = path + ".xml";
	
	LOG_DBG("imageExists: %s", fileName.c_str());

	return FileStream::Exists(fileName.c_str());
}

void Application::ImageDelete(ImageId imageId)
{
	std::string path = gSystem.GetDocumentPath(imageId.mName.c_str());
	
	std::string fileName = path + ".xml";
	
	LOG_DBG("imageDelete: %s", fileName.c_str());
	
	FileStream::Delete(fileName.c_str());
}

void Application::ImageReset(ImageId imageId)
{
	Assert(!mImageId.IsSet_get());
	Assert(mCommandStreamPosition == 0);
	Assert(mDataStreamPosition == 0);
	Assert(mSwatchMgr.SwatchCount_get() == 0);
	
	mImageId = imageId;
	
	mUndoStack.Clear();
	
	mCommandStreamPosition = 0;
	mDataStreamPosition = 0;
	
	mLastColorCommand = CommandPacket::Make_ColorSelect(0.5f, 0.5f, 0.5f, 1.0f);
	mLastToolSelectCommand = CommandPacket::Make_ToolSelect_SoftBrush(5, 0.5f, 0.05f);
	
	mSwatchMgr.Clear();
}

void Application::ImageDuplicate(ImageId srcId, ImageId dstId)
{	
	LoadImage(srcId);
	
	mImageId = dstId;
	
	SaveImage();
	
	// manually copy command and data streams
	
	{
		std::string src = GetPath_CommandStream(srcId);
		std::string dst = GetPath_CommandStream(dstId);
		
		FileStream srcStream;
		FileStream dstStream;
		
		srcStream.Open(src.c_str(), OpenMode_Read);
		dstStream.Open(dst.c_str(), OpenMode_Write);
		
		StreamExtensions::StreamTo(&srcStream, &dstStream, 4096);
	}
	
	{
		std::string src = GetPath_DataStream(srcId);
		std::string dst = GetPath_DataStream(dstId);
		
		FileStream srcStream;
		FileStream dstStream;
		
		srcStream.Open(src.c_str(), OpenMode_Read);
		dstStream.Open(dst.c_str(), OpenMode_Write);
		
		StreamExtensions::StreamTo(&srcStream, &dstStream, 4096);
	}
}

void Application::ImageActivate(bool writeEnabled)
{
	if (mIsActive)
		throw ExceptionVA("image already active");
	if (!mImageId.IsSet_get())
		throw ExceptionVA("image ID not set");
	
	mWriteEnabled = writeEnabled;
	
	if (mWriteEnabled)
	{
		// open streams
		
		CommandStreamOpen();
		
		// note: make sure data stream exists- nothing more
		
		DataStreamOpen();
		DataStreamClose();
	}
	
	mIsActive = true;
	
	Execute(mLastColorCommand);
	Execute(mLastToolSelectCommand);
}

void Application::ImageDeactivate()
{
	if (!mIsActive)
		throw ExceptionVA("image not active");
	
	// make sure brush stroke is ended
	
	if (mToolActive)
	{
		if (mWriteEnabled)
			StrokeEnd();
		else
			ExecuteStrokeEnd();
	}
	
	if (mWriteEnabled)
	{
		// close streams
		
		CommandStreamClose();
	}
	
	mIsActive = false;
}

bool Application::ImageIsActive_get() const
{
	return mIsActive;
}

void Application::SaveImage()
{
	if (!mImageId.IsSet_get())
		throw ExceptionVA("image ID not set");
	
	std::string path = gSystem.GetDocumentPath(mImageId.mName.c_str());
	
	MacImage merged;
	mLayerMgr->RenderMergedFinal(merged);
	
	SaveImageXml(path);
	SaveImageData(path);
	SaveImagePreview(path, &merged);
	SaveImageThumbnail(path, &merged);
	SaveImageBrushes(path);
}

void Application::SaveImageXml(const std::string& path)
{
	volatile Benchmark bm("save image xml");
	
	std::string fileName = path + ".xml";
	
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Write);
	
	XmlWriter writer(&stream, false);
	
	writer.BeginNode("picture");
	{
		writer.BeginNode("format");
		{
			writer.WriteAttribute_Int32("sx", mLayerMgr->Size_get()[0]);
			writer.WriteAttribute_Int32("sy", mLayerMgr->Size_get()[1]);
		}
		writer.EndNode();
		
		writer.BeginNode("streams");
		{
			writer.BeginNode("commandStream");
			{
				writer.WriteAttribute_Int32("position", mCommandStreamPosition);
			}
			writer.EndNode();
			writer.BeginNode("dataStream");
			{
				writer.WriteAttribute_Int32("position", mDataStreamPosition);
			}
			writer.EndNode();
		}
		writer.EndNode();
		
		writer.BeginNode("layers");
		{
			writer.WriteAttribute_Int32("count", mLayerMgr->LayerCount_get());
			writer.WriteAttribute_Int32("active", mLayerMgr->ActiveDataLayer_get());
			
			for (int i = 0; i < mLayerMgr->LayerCount_get(); ++i)
			{
				writer.BeginNode("layer");
				{
					const int index = mLayerMgr->LayerOrder_get(i);
					
					writer.WriteAttribute_Int32("index", index);
					writer.WriteAttribute_Int32("order", i);
					writer.WriteAttribute_Int32("visibility", mLayerMgr->DataLayerVisibility_get(index) ? 1 : 0);
					writer.WriteAttribute_Int32("opacity", int(mLayerMgr->DataLayerOpacity_get(index) * 100.0f));
					std::string dataLayerFileName = Path::GetFileName(path + String::Format("_layer%d.bin", index));
					writer.WriteAttribute("file", dataLayerFileName.c_str());
				}
				writer.EndNode();
			}
		}
		writer.EndNode();
		
		writer.BeginNode("restore");
		{
			writer.BeginNode("color");
			{
				MemoryStream packetStream;
				StreamWriter streamWriter(&packetStream, false);
				mLastColorCommand.Write(streamWriter);
				uint8_t* bytes;
				int byteCount;
				packetStream.ToArray(&bytes, &byteCount);
				writer.WriteAttribute_Bytes("data", bytes, byteCount);
				delete[] bytes;
			}
			writer.EndNode();

			writer.BeginNode("tool");
			{
				MemoryStream packetStream;
				StreamWriter streamWriter(&packetStream, false);
				mLastToolSelectCommand.Write(streamWriter);
				uint8_t* bytes;
				int byteCount;
				packetStream.ToArray(&bytes, &byteCount);
				writer.WriteAttribute_Bytes("data", bytes, byteCount);
				delete[] bytes;
			}
			writer.EndNode();
			
			writer.BeginNode("swatches");
			{
				MemoryStream swatchesStream;
				mSwatchMgr.Write(&swatchesStream);
				uint8_t* bytes;
				int byteCount;
				swatchesStream.ToArray(&bytes, &byteCount);
				writer.WriteAttribute_Bytes("data", bytes, byteCount);
				delete[] bytes;
			}
			writer.EndNode();
		}
		writer.EndNode();
	}
	writer.EndNode();
	
	writer.Close();
	
	stream.Close();
}

void Application::SaveImageData(const std::string& path)
{
	volatile Benchmark bm("save image data");
	
	for (int i = 0; i < mLayerMgr->LayerCount_get(); ++i)
	{
		std::string fileName = path + String::Format("_layer%d.bin", i);
		
		FileStream stream;
		
		stream.Open(fileName.c_str(), OpenMode_Write);
		
		MacImage* image = mLayerMgr->DataLayer_get(i);
		
		image->Save(&stream);
		
		stream.Close();
	}
}

void Application::SaveImagePreview(const std::string& path, MacImage* merged)
{
	volatile Benchmark bm("save image preview");
	
	std::string fileName = path + "_preview.bin";
	
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Write);
	
	merged->Save(&stream);
	
	stream.Close();
}

void Application::SaveImageThumbnail(const std::string& path, MacImage* merged)
{
	volatile Benchmark bm("save image thumbnail");
	
	MacImage thumbnail;
	thumbnail.Size_set(90, 135, false);
	merged->Blit_Resampled(&thumbnail, false);
	
#if 1
	{
		std::string fileName = path + "_thumbnail.png";

		gSystem.SaveAsPng(thumbnail, fileName.c_str());
	}
#endif
	
#if 1
	{
		std::string fileName = path + "_thumbnail.bin";
		
		FileStream stream;
		stream.Open(fileName.c_str(), OpenMode_Write);
		thumbnail.Save(&stream);
		stream.Close();
	}
#endif
}

void Application::SaveImageBrushes(const std::string& path)
{
	volatile Benchmark bm("save image brushes");
	
	if (!mImageId.IsSet_get())
		throw ExceptionVA("image ID not set");
	
	std::string fileName = path + ".brushes";

	FileStream stream;

	stream.Open(fileName.c_str(), OpenMode_Write);

	mBrushLibrary_Custom.Save(&stream);
}

static std::string GetPath()
{
	return gSystem.GetDocumentPath("") + "/";
}

static std::string GetPath(ImageId id)
{
	return gSystem.GetDocumentPath(id.mName.c_str());
}

void Application::LoadImage(ImageId id)
{
	if (!id.IsSet_get())
		throw ExceptionVA("image ID not set");
	
	ImageReset(id);
	
	std::string path = GetPath(id);
	
	LoadImageXml(path);
	LoadImageData(path);
}

void Application::LoadImageXml(const std::string& path)
{
	volatile Benchmark bm("load image xml");
	
	// read
	
	kdImageDescription description = LoadImageDescription(mImageId);
	
	// validate

	description.Validate();

	// make effective

	mCommandStreamPosition = description.commandStream.position;
	mDataStreamPosition = description.dataStream.position;
	
	ExecuteImageSize(description.layerCount, description.sx, description.sy);
	
	std::vector<int> layerOrder;
	
	for (int i = 0; i < description.layerCount; ++i)
	{
		mLayerMgr->DataLayerVisibility_set(i, description.dataLayerVisibility[i]);
		mLayerMgr->DataLayerOpacity_set(i, description.dataLayerOpacity[i]);
	}
	
	mLayerMgr->LayerOrder_set(description.layerOrder);
	mLayerMgr->ActiveDataLayer_set(description.activeLayer);
	
	if (description.hasLastColorCommand)
		mLastColorCommand = description.lastColorCommand;
	
	if (description.hasLastToolSelectCommand)
		mLastToolSelectCommand = description.lastToolSelectCommand;
	
	for (int i = 0; i < description.swatches.SwatchCount_get(); ++i)
		mSwatchMgr.Add(description.swatches.Swatch_get(i));
}

void Application::LoadImageData(const std::string& path)
{
	volatile Benchmark bm("load image data");
	
	for (int i = 0; i < mLayerMgr->LayerCount_get(); ++i)
	{
		std::string fileName = Path::GetFileName(GetPath(mImageId) + String::Format("_layer%d.bin", i));
		
		MacImage* image = mLayerMgr->DataLayer_get(i);
		
		LoadImageDataLayer(fileName, image);
	}
	
	Invalidate();
}

kdImageDescription Application::LoadImageDescription(ImageId id)
{
	volatile Benchmark bm("load image description");
	
	std::string path = GetPath(id);
	
	std::string fileName = path + ".xml";
	
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Read);
	
	kdImageDescription description;
	
	description.Read(&stream);

	return description;
}

void Application::LoadImageDataLayer(const std::string& _fileName, MacImage* dst)
{
	std::string path = GetPath();
	
	std::string fileName = path + _fileName;
	
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Read);
	
	dst->Load(&stream);
	
	stream.Close();
}

void Application::LoadImagePreview(ImageId imageId, MacImage& image)
{
	volatile Benchmark bm("load image preview");
	
	const std::string path = GetPath(imageId);
	
	std::string fileName = path + "_preview.bin";
	
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Read);
	
	image.Load(&stream);
	
	stream.Close();
}

void Application::LoadImageThumbnail(ImageId id, MacImage& image, bool flipY)
{
	volatile Benchmark bm("load image thumbnail");
	
	const std::string path = GetPath(id);

	std::string fileName = path + "_thumbnail.bin";
	
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Read);
	
	image.Load(&stream);
	
	stream.Close();
}

void Application::SaveImageArchive(ImageId imageId, Stream* stream)
{
	// pack multiple resources into one archive and return result
	
	kdImageDescription description = LoadImageDescription(imageId);
	
	std::string baseName = GetPath(imageId);
	
	std::string fileNameCommandStream = baseName + ".rec";
	std::string fileNameBrushes = baseName + ".brushes";
	std::string fileNamePreview = baseName + "_preview.bin";
	std::string fileNameDataStream = baseName + ".dat";
	
	// open streams
	
	FileStream streamCommands;
	FileStream streamBrushes;
	FileStream streamPreview;
	FileStream streamData;
	
	streamCommands.Open(fileNameCommandStream.c_str(), OpenMode_Read);
	streamBrushes.Open(fileNameBrushes.c_str(), OpenMode_Read);
	streamPreview.Open(fileNamePreview.c_str(), OpenMode_Read);
	streamData.Open(fileNameDataStream.c_str(), OpenMode_Read);
	
	// stream data to archive
	
	FileArchive archive;
	
	archive.SaveBegin(stream);
	archive.SaveAdd(stream, "recording", &streamCommands, description.commandStream.position);
	archive.SaveAdd(stream, "brushes", &streamBrushes);
	archive.SaveAdd(stream, "preview", &streamPreview);
	archive.SaveAdd(stream, "data", &streamData, description.dataStream.position);
	archive.SaveEnd(stream);
}

std::string Application::GetPath_CommandStream(ImageId imageId)
{
	return GetPath(imageId) + ".rec";
}

std::string Application::GetPath_DataStream(ImageId imageId)
{
	return GetPath(imageId) + ".dat";
}

ImageId Application::ImageId_get() const
{
	return mImageId;
}

// -------
// Streams
// -------

void Application::CommandStreamOpen()
{
	std::string fileName = GetPath_CommandStream(mImageId);
	
	mCommandStream = new FileStream(fileName.c_str(), (OpenMode)(OpenMode_Write | OpenMode_Append));
	mCommandStreamWriter = new CommandStreamWriter(mCommandStream, false);
	
	LOG_DBG("command stream open: position: %d", mCommandStreamPosition);
	
	mCommandStream->Seek(mCommandStreamPosition, SeekMode_Begin);
}

void Application::CommandStreamClose()
{
	mCommandStreamPosition = mCommandStream->Position_get();
	
	LOG_DBG("command stream close: position: %d", mCommandStreamPosition);
	
	delete mCommandStreamWriter;
	mCommandStreamWriter = 0;
	
	mCommandStream->Close();
	delete mCommandStream;
	mCommandStream = 0;
}

void Application::DataStreamOpen()
{
	Assert(mDataStream == 0);

	std::string path = gSystem.GetDocumentPath(mImageId.mName.c_str());
	std::string fileName = path + ".dat";
	
	if (mStreamProvider)
		mDataStream = mStreamProvider->OpenStream(StreamType_Data);
	else
		mDataStream = new FileStream(fileName.c_str(), (OpenMode)(OpenMode_Write | OpenMode_Append));
	mDataStreamReader = new DataStreamReader(mDataStream, false);
	mDataStreamWriter = new DataStreamWriter(mDataStream, false);
	
	mDataStream->Seek(mDataStreamPosition, SeekMode_Begin);
}

void Application::DataStreamClose()
{
	Assert(mDataStream != 0);

	mDataStreamPosition = mDataStream->Position_get();
	
	delete mDataStreamReader;
	mDataStreamReader = 0;
	
	delete mDataStreamWriter;
	mDataStreamWriter = 0;
	
	if (mStreamProvider)
	{
		mStreamProvider->CloseStream(StreamType_Data, mDataStream);
		mDataStream = 0;
	}
	else
	{
		mDataStream->Close();
		delete mDataStream;
		mDataStream = 0;
	}
}

// ---------
// Debugging
// ---------

void Application::DBG_PaintAt(float x, float y, float dx, float dy)
{
	DoPaint(x, y, dx, dy);
}

void Application::DBG_ValidateCommandStream(ImageId imageId)
{
	std::string path = gSystem.GetDocumentPath(imageId.mName.c_str());
	std::string fileName = path + ".rec";
	
	FileStream stream(fileName.c_str(), OpenMode_Read);
	
	CommandStreamWriter::DBG_TestDeserialization(&stream);
	
	// fixme
	CommandStreamWriter::DBG_TestSerialization();
}
