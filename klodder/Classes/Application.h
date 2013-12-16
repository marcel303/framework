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
	
	void Setup(StreamProvider* streamProvider, const char* brushLibraryStandard, const char* brushLibraryCustom, int scale, Rgba backColor1, Rgba backColor2);
	
private:
	// setup
	bool mIsSetup;
	StreamProvider* mStreamProvider;
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
	static void HandleTouchZoomStateChange(void* obj, void* arg);
	
	// ------
	// Canvas
	// ------
public:
	LayerMgr* LayerMgr_get();
	
private:
	LayerMgr* mLayerMgr;
	
	// -----------------
	// Command execution
	// -----------------
public:
	void Execute(const CommandPacket& packet);
	void ExecuteAndSave(const CommandPacket& packet);
	void ExecutionCommit();
	void ExecutionDiscard();
	
	void ExecuteColorSelect(float r, float g, float b, float a);
	void ExecuteImageSize(int layerCount, int sx, int sy);
	void ExecuteDataLayerBlit(int index, const BlitTransform& transform);
	void ExecuteDataLayerClear(int index, float r, float g, float b, float a);
	void ExecuteDataLayerMerge(int index1, int index2);
	void ExecuteDataLayerSelect(int index);
//	void ExecuteLayerSwap(int layer1, int layer2);
	void ExecuteDataLayerOpacity(int index, float opacity);
	void ExecuteLayerOrder(std::vector<int> layerOrder);
	void ExecuteDataLayerVisibility(int index, bool visibility);
	void ExecuteStrokeBegin(int index, bool smooth, bool mirrorX, float x, float y);
	void ExecuteStrokeEnd();
	void ExecuteStrokeMove(float x, float y);
	void ExecuteToolSelect_SoftBrush(int diameter, float hardness, float spacing);
	void ExecuteToolSelect_PatternBrush(int diameter, uint32_t patternId, float spacing);
	void ExecuteToolSelect_SoftBrushDirect(int diameter, float hardness, float spacing);
	void ExecuteToolSelect_PatternBrushDirect(int diameter, uint32_t patternId, float spacing);
	void ExecuteToolSelect_SoftSmudge(int diameter, float hardness, float spacing, float strength);
	void ExecuteToolSelect_PatternSmudge(int diameter, uint32_t patternId, float spacing, float strength);
	void ExecuteToolSelect_SoftEraser(int diameter, float hardness, float spacing);
	void ExecuteToolSelect_PatternEraser(int diameter, uint32_t patternId, float spacing);
	
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
	void ToolType_set(ToolType type);
public:
	
	void StrokeBegin(int index, bool smooth, bool mirrorX, float x, float y);
	void StrokeEnd();
	void StrokeMove(float x, float y);
	void StrokeCancel();
	
	void Invalidate();
	void Invalidate(int x, int y, int sx, int sy);
	
private:
	static void HandleBezierTravel(void* obj, BezierTravellerState state, float x, float y);
	static void HandleTravel(void* obj, const TravelEvent& e);
	void DoPaint(float x, float y, float dx, float dy);

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
	void CommitUndoBuffer(UndoBuffer* undo);
	
	UndoStack mUndoStack;
	bool mUndoEnabled;
	
public:
	void UndoEnabled_set(bool enabled);
	bool HasUndo_get() const;
	bool HasRedo_get() const;
	void Undo();
	void Redo();
	
	// -----
	// Tools
	// -----
public:
	void ColorSelect(float r, float g, float b, float a);
	void ImageInitialize(int layerCount, int sx, int sy); // execute some commands to get image into an initialized state
	void ImageSize(int layerCount, int sx, int sy);
	void DataLayerBlit(int index, MacImage* src, const BlitTransform& transform);
	void DataLayerClear(int index, float r, float g, float b, float a);
	void DataLayerMerge(int index1, int index2);
	void DataLayerOpacity(int index, float opacity);
	void LayerOrder(std::vector<int> order);
	void DataLayerSelect(int index);
	void DataLayerVisibility(int index, bool visibility);
	void ToolSelect_BrushSoft(ToolSettings_BrushSoft settings);
	void ToolSelect_BrushPattern(ToolSettings_BrushPattern settings);
	void ToolSelect_BrushSoftDirect(ToolSettings_BrushSoftDirect settings);
	void ToolSelect_BrushPatternDirect(ToolSettings_BrushPatternDirect settings);
	void ToolSelect_SmudgeSoft(ToolSettings_SmudgeSoft settings);
	void ToolSelect_SmudgePattern(ToolSettings_SmudgePattern settings);
	void ToolSelect_EraserSoft(ToolSettings_EraserSoft settings);
	void ToolSelect_EraserPattern(ToolSettings_EraserPattern settings);
	void SwitchErazorBrush();
	
private:
	void BrushPattern_set(uint32_t patternId);
public:
	uint32_t BrushPattern_get() const;
private:
	void StrokeInterval_set(float interval);
public:
	float StrokeInterval_get() const;
private:
	bool FindPattern(uint32_t patternId, Brush_Pattern** out_pattern);
public:
	Rgba BrushColor_get() const;
private:
	void BrushColor_set(Rgba color);
public:
	float BrushOpacity_get() const;
private:
	void BrushOpacity_set(float opacity);
	void StrokeIsOriented_set(bool isOriented);
	
private:
	ITool* Tool_get(ToolType type);
public:
	BrushLibrarySet* BrushLibrarySet_get();
	
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
	void SetupBrushLibraries(const char* standardFileName, const char* customFileName);
	void AddBrush(Brush_Pattern* brush);
	
	BrushLibrary mBrushLibrary_Standard;
	std::string mBrushLibrary_Standard_FileName;
	BrushLibrary mBrushLibrary_Custom;
	std::string mBrushLibrary_Custom_FileName;
	BrushLibrary mBrushLibrary_Image;
	BrushLibrarySet mBrushLibrarySet;

	Brush_Pattern* mBrushPattern;

	// ----------
	// Eyedropper
	// ----------
public:
	Rgba GetColorAtLocation(Vec2F location) const;
	
	// --------
	// Swatches
	// --------
public:
	SwatchMgr* SwatchMgr_get();
	
private:
	SwatchMgr mSwatchMgr;
	
	// --
	// IO
	// --
public:
	ImageId AllocateImageId();
	bool ImageExists(ImageId imageId); // return true if the image exists
	void ImageDelete(ImageId imageId); // removes an image
	void ImageReset(ImageId imageId); // reset some state to prepare for a new image
	void ImageDuplicate(ImageId srcId, ImageId dstId); // duplicate an image
	
	void ImageActivate(bool writeEnabled);
	void ImageDeactivate();
	bool ImageIsActive_get() const;
	
	void SaveImage();
	void SaveImageXml(const std::string& path);
	void SaveImageData(const std::string& path);
	void SaveImagePreview(const std::string& path, MacImage* merged);
	void SaveImageThumbnail(const std::string& path, MacImage* merged);
	void SaveImageBrushes(const std::string& path);
	void LoadImage(ImageId id);
	void LoadImageXml(const std::string& path);
	void LoadImageData(const std::string& path);
	
	static kdImageDescription LoadImageDescription(ImageId id);
	static void LoadImageDataLayer(const std::string& fileName, MacImage* dst);
	static void LoadImagePreview(ImageId id, MacImage& image);
	static void LoadImageThumbnail(ImageId id, MacImage& image, bool flipY);
	
	static void SaveImageArchive(ImageId id, Stream* stream);

	static std::string GetPath_CommandStream(ImageId imageId);
	static std::string GetPath_DataStream(ImageId imageId);
	
	ImageId ImageId_get() const;
	
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
	
	Stream* mCommandStream;
	int mCommandStreamPosition;
	CommandStreamWriter* mCommandStreamWriter;
	
	Stream* mDataStream;
	int mDataStreamPosition;
	DataStreamWriter* mDataStreamWriter;
	DataStreamReader* mDataStreamReader;
	
	// ---------
	// Debugging
	// ---------
public:
	void DBG_PaintAt(float x, float y, float dx, float dy);
	static void DBG_ValidateCommandStream(ImageId imageId);
};
