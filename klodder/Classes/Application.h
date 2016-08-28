#pragma once

#include "BezierTraveller.h"
#include "BrushLibrary.h"
#include "BrushLibrarySet.h"
#include "ChangeType.h"
#include "CommandStream.h"
#include "ImageDescription.h"
#include "ImageId.h"
#include "klodder_forward.h"
#include "Tool_Blur.h"
#include "Tool_Brush.h"
#include "Tool_Colorize.h"
#include "Tool_Sharpen.h"
#include "Tool_Smudge.h"
#include "TouchDLG.h"
#include "TouchZoom.h"
#include "Traveller.h"
#include "UndoStack.h"

class Application
{
public:
	Application();
	~Application();
	
	void Setup(
		StreamProvider * streamProvider,
		const char * brushLibraryStandard,
		const char * brushLibraryCustom,
		const int scale,
		const Rgba & backColor1,
		const Rgba & backColor2);
	
private:
	// setup
	bool mIsSetup;
	StreamProvider * mStreamProvider;
	int mScale;
	Rgba mBackColor1;
	Rgba mBackColor2;
	
	// -------------------
	// Change notification
	// -------------------
public:
	CallBack OnChange;
	
private:
	void SignalChange(ChangeType type);
	static void HandleTouchZoomStateChange(void * obj, void * arg);
	
	// ------
	// Canvas
	// ------
public:
	LayerMgr * LayerMgr_get();
	
private:
	LayerMgr * mLayerMgr;
	
	// -----------------
	// Command execution
	// -----------------
public:
	void Execute(const CommandPacket & packet);
	void ExecuteAndSave(const CommandPacket & packet);
	void ExecutionCommit();
	void ExecutionDiscard();
	
	void ExecuteColorSelect(const float r, const float g, const float b, const float a);
	void ExecuteImageSize(const int layerCount, const int sx, const int sy);
	void ExecuteDataLayerBlit(const int index, const BlitTransform & transform);
	void ExecuteDataLayerClear(const int index, const float r, const float g, const float b, const float a);
	void ExecuteDataLayerMerge(const int index1, const int index2);
	void ExecuteDataLayerSelect(const int index);
//	void ExecuteLayerSwap(const int layer1, const int layer2);
	void ExecuteDataLayerOpacity(const int index, const float opacity);
	void ExecuteLayerOrder(const std::vector<int> & layerOrder);
	void ExecuteDataLayerVisibility(const int index, const bool visibility);
	void ExecuteStrokeBegin(const int index, const bool smooth, const bool mirrorX, const float x, const float y);
	void ExecuteStrokeEnd();
	void ExecuteStrokeMove(const float x, const float y);
	void ExecuteToolSelect_SoftBrush(const int diameter, const float hardness, const float spacing);
	void ExecuteToolSelect_PatternBrush(const int diameter, const uint32_t patternId, const float spacing);
	void ExecuteToolSelect_SoftBrushDirect(const int diameter, const float hardness, const float spacing);
	void ExecuteToolSelect_PatternBrushDirect(const int diameter, const uint32_t patternId, const float spacing);
	void ExecuteToolSelect_SoftSmudge(const int diameter, const float hardness, const float spacing, const float strength);
	void ExecuteToolSelect_PatternSmudge(const int diameter, const uint32_t patternId, const float spacing, const float strength);
	void ExecuteToolSelect_SoftEraser(const int diameter, const float hardness, const float spacing);
	void ExecuteToolSelect_PatternEraser(const int diameter, const uint32_t patternId, const float spacing);
	
private:
	CommandPacket mLastColorCommand;
	CommandPacket mLastToolSelectCommand;
	std::vector<CommandPacket> mPacketList;
	
	// -------
	// Editing
	// -------
public:
	ToolType ToolType_get() const;
private:
	void ToolType_set(const ToolType type);
public:
	
	void StrokeBegin(const int index, const bool smooth, const bool mirrorX, const float x, const float y);
	void StrokeEnd();
	void StrokeMove(const float x, const float y);
	void StrokeCancel();
	
	void Invalidate();
	void Invalidate(const int x, const int y, const int sx, const int sy);
	
private:
	static void HandleBezierTravel(void * obj, const BezierTravellerState state, const float x, const float y);
	static void HandleTravel(void * obj, const TravelEvent & e);
	void DoPaint(const float x, const float y, const float dx, const float dy);

	ToolType mToolType;
	bool mToolActive;
	BezierTraveller mBezierTraveller;
	Traveller mTraveller;
	bool mStrokeSmooth;
	bool mStrokeMirrorX;
	AreaI mDirtyArea_Total;
	
	// ----
	// Undo
	// ----
private:
	void CommitUndoBuffer(UndoBuffer * undo);
	
	UndoStack mUndoStack;
	bool mUndoEnabled;
	
public:
	void UndoEnabled_set(const bool enabled);
	bool HasUndo_get() const;
	bool HasRedo_get() const;
	void Undo();
	void Redo();
	
	// -----
	// Tools
	// -----
public:
	void ColorSelect(const float r, const float g, const float b, const float a);
	void ImageInitialize(const int layerCount, const int sx, const int sy); // execute some commands to get image into an initialized state
	void ImageSize(const int layerCount, const int sx, const int sy);
	void DataLayerBlit(const int index, const MacImage * src, const BlitTransform & transform);
	void DataLayerClear(const int index, const float r, const float g, const float b, const float a);
	void DataLayerMerge(const int index1, const int index2);
	void DataLayerOpacity(const int index, const float opacity);
	void LayerOrder(const std::vector<int> & order);
	void DataLayerSelect(const int index);
	void DataLayerVisibility(const int index, const bool visibility);
	void ToolSelect_BrushSoft(const ToolSettings_BrushSoft & settings);
	void ToolSelect_BrushPattern(const ToolSettings_BrushPattern & settings);
	void ToolSelect_BrushSoftDirect(const ToolSettings_BrushSoftDirect & settings);
	void ToolSelect_BrushPatternDirect(const ToolSettings_BrushPatternDirect & settings);
	void ToolSelect_SmudgeSoft(const ToolSettings_SmudgeSoft & settings);
	void ToolSelect_SmudgePattern(const ToolSettings_SmudgePattern & settings);
	void ToolSelect_EraserSoft(const ToolSettings_EraserSoft & settings);
	void ToolSelect_EraserPattern(const ToolSettings_EraserPattern & settings);
	void SwitchErazorBrush();
	
private:
	void BrushPattern_set(const uint32_t patternId);
public:
	uint32_t BrushPattern_get() const;
private:
	void StrokeInterval_set(const float interval);
public:
	float StrokeInterval_get() const;
private:
	bool FindPattern(const uint32_t patternId, Brush_Pattern ** out_pattern);
public:
	const Rgba & BrushColor_get() const;
private:
	void BrushColor_set(const Rgba & color);
public:
	float BrushOpacity_get() const;
private:
	void BrushOpacity_set(const float opacity);
	void StrokeIsOriented_set(const bool isOriented);
	
private:
	Tool * Tool_get(const ToolType type);
public:
	BrushLibrarySet * BrushLibrarySet_get();
	
	ToolSettings_BrushSoft mToolSettings_BrushSoft;
	ToolSettings_BrushPattern mToolSettings_BrushPattern;
	ToolSettings_BrushSoftDirect mToolSettings_BrushSoftDirect;
	ToolSettings_BrushPatternDirect mToolSettings_BrushPatternDirect;
	ToolSettings_SmudgeSoft mToolSettings_SmudgeSoft;
	ToolSettings_SmudgePattern mToolSettings_SmudgePattern;
	ToolSettings_EraserSoft mToolSettings_EraserSoft;
	ToolSettings_EraserPattern mToolSettings_EraserPattern;
	
private:	
	Tool_Blur mToolBlur;
	Tool_Brush mToolBrush;
	Tool_BrushDirect mToolBrushDirect;
	Tool_Colorize mToolColorize;
	Tool_Sharpen mToolSharpen;
	Tool_Smudge mToolSmudge;
	float mStrokeInterval;

	// -------
	// Brushes
	// -------	
private:
	void SetupBrushLibraries(const char * standardFileName, const char * customFileName);
	void AddBrush(Brush_Pattern * brush);
	
	BrushLibrary mBrushLibrary_Standard;
	std::string mBrushLibrary_Standard_FileName;
	BrushLibrary mBrushLibrary_Custom;
	std::string mBrushLibrary_Custom_FileName;
	BrushLibrary mBrushLibrary_Image;
	BrushLibrarySet mBrushLibrarySet;

	Brush_Pattern * mBrushPattern;

	// ----------
	// Eyedropper
	// ----------
public:
	Rgba GetColorAtLocation(const Vec2F & location) const;
	
	// --------
	// Swatches
	// --------
public:
	SwatchMgr * SwatchMgr_get();
	
private:
	SwatchMgr mSwatchMgr;
	
	// --
	// IO
	// --
public:
	ImageId AllocateImageId();
	bool ImageExists(const ImageId & imageId); // return true if the image exists
	void ImageDelete(const ImageId & imageId); // removes an image
	void ImageReset(const ImageId & imageId); // reset some state to prepare for a new image
	void ImageDuplicate(const ImageId & srcId, const ImageId & dstId); // duplicate an image
	
	void ImageActivate(const bool writeEnabled);
	void ImageDeactivate();
	bool ImageIsActive_get() const;
	
	void SaveImage();
	void SaveImageXml(const std::string & path);
	void SaveImageData(const std::string & path);
	void SaveImagePreview(const std::string & path, const MacImage * merged);
	void SaveImageThumbnail(const std::string & path, const MacImage * merged);
	void SaveImageBrushes(const std::string & path);
	void LoadImage(const ImageId & id);
	void LoadImageXml(const std::string & path);
	void LoadImageData(const std::string & path);
	
	static kdImageDescription LoadImageDescription(const ImageId & id);
	static void LoadImageDataLayer(const std::string & fileName, MacImage * dst);
	static void LoadImagePreview(const ImageId & id, MacImage & image);
	static void LoadImageThumbnail(const ImageId & id, MacImage & image, const bool flipY);
	
	static void SaveImageArchive(const ImageId & id, Stream * stream);

	static std::string GetPath_CommandStream(const ImageId & imageId);
	static std::string GetPath_DataStream(const ImageId & imageId);
	
	const ImageId & ImageId_get() const;
	
private:
	ImageId mImageId;
	bool mIsActive;
	bool mWriteEnabled;
	
	// -------
	// Streams
	// -------
private:
	void CommandStreamOpen();
	void CommandStreamClose();
	void DataStreamOpen();
	void DataStreamClose();
	
	Stream * mCommandStream;
	int mCommandStreamPosition;
	CommandStreamWriter * mCommandStreamWriter;
	
	Stream * mDataStream;
	int mDataStreamPosition;
	DataStreamWriter * mDataStreamWriter;
	DataStreamReader * mDataStreamReader;
	
	// ---------
	// Debugging
	// ---------
public:
	void DBG_PaintAt(const float x, const float y, const float dx, const float dy);
	static void DBG_ValidateCommandStream(const ImageId & imageId);
};
