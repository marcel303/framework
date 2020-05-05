#pragma once

// ecs-sceneEditor
#include "editor/transformGizmos.h"

// ecs-scene
//#include "helpers.h" // findComponentType
#include "scene.h"

// imgui-framework
#include "imgui-framework.h"

// framework
#include "framework-camera.h"

// std
#include <set>

//

#if defined(DEBUG)
	#define ENABLE_LOAD_AFTER_SAVE_TEST 1
	#define ENABLE_SAVE_LOAD_TIMING     1
#else
	#define ENABLE_LOAD_AFTER_SAVE_TEST 0 // do not alter
	#define ENABLE_SAVE_LOAD_TIMING     0 // do not alter
#endif

#define ENABLE_TRANSFORM_GIZMOS 1

#define ENABLE_QUAT_FIXUP 1

//

struct AngleAxis;

//

struct SceneEditor
{
	static const int kMaxNodeDisplayNameFilter = 100;
	static const int kMaxComponentTypeNameFilter = 100;
	static const int kMaxParameterFilter = 100;
	
	TypeDB * typeDB = nullptr;
	
	Scene scene; // todo : this should live outside the editor, but referenced
	
	Camera camera; // todo : this should live outside the editor, but referenced
	bool cameraIsActive = false;
	
	std::set<int> selectedNodes;
	int hoverNodeId = -1;
	
	int nodeToGiveFocus = -1;
	
#if ENABLE_TRANSFORM_GIZMOS
	TransformGizmo transformGizmo;
#endif

	FrameworkImGuiContext guiContext;
	
	struct
	{
		bool drawGrid = true;
		bool drawGroundPlane = false;
		bool drawNodes = true;
		bool drawNodeBoundingBoxes = true;
	} visibility;
	
	struct
	{
		bool tickScene = false;
		float tickMultiplier = 1.f;
		
		bool drawScene = true;
	} preview;
	
	struct
	{
		std::set<int> nodesToRemove;
		std::vector<SceneNode*> nodesToAdd;
		
		bool isFlushed() const
		{
			return
				nodesToRemove.empty() &&
				nodesToAdd.empty();
		}
	} deferred;
	
	struct
	{
		char nodeDisplayNameFilter[kMaxNodeDisplayNameFilter] = { };
		char componentTypeNameFilter[kMaxComponentTypeNameFilter] = { };
		std::set<int> visibleNodes; // set of visible nodes, when the node structure is filtered, by for instance node name or component type
		std::set<int> nodesToOpen; // nodes inside this set will be forcibly opened inside the node structure editor and scrolled into view
	} nodeUi;
	
	struct
	{
		char component_filter[kMaxParameterFilter] = { };
		char parameter_filter[kMaxParameterFilter] = { };
		
		bool showAnonymousComponents = false;
	} parameterUi;
	
	struct
	{
		std::vector<std::string> versions;
		int currentVersionIndex = -1;
		
		std::string currentVersion;
	} undo;
	
	//
	
	void init(TypeDB * in_typeDB);
	void shut();

	SceneNode * raycast(Vec3Arg rayOrigin, Vec3Arg rayDirection, const std::set<int> & nodesToIgnore) const;
	
	/**
	 * Begin a deferred update to the scene structure. Note we need to defer updates, due to
	 * the fact that the scene editor traverses the scene structure at the same time as making
	 * updates to it. Making immediate updates would invalidate and/or complicate scene traversal.
	 */
	void deferredBegin();
	void deferredEnd(const bool selectAddedNodes);
	
	void removeNodeSubTree(const int nodeId);
	
	void selectNodes(const std::set<int> & nodeIds, const bool append);
	void selectNode(const int nodeId, const bool append);
	
	void addNodesToAdd();
	void removeNodesToRemove();
	
	// undo functions
	bool undoCapture(std::string & text) const; // capture the entire scene into text format
	void undoCaptureBegin(); // begin a scene capture
	void undoCaptureEnd();   // end a scene capture and add it to the undo history
	void undoReset(); // reset the undo history
	
	void editNode(const int nodeId);
	
	void pasteNodeFromText(const int parentId, const char * text);
	void pasteNodeFromClipboard(const int parentId);

	enum NodeStructureContextMenuResult
	{
		kNodeStructureContextMenuResult_None,
		kNodeStructureContextMenuResult_NodeCopy,
		kNodeStructureContextMenuResult_NodePaste,
		kNodeStructureContextMenuResult_NodeQueuedForRemove,
		kNodeStructureContextMenuResult_NodeAdded
	};
	
	NodeStructureContextMenuResult doNodeStructureContextMenu(SceneNode & node);
	
	enum NodeContextMenuResult
	{
		kNodeContextMenuResult_None,
		kNodeContextMenuResult_ComponentShouldBeRemoved,
		kNodeContextMenuResult_ComponentAdded
	};
	
	NodeContextMenuResult doNodeContextMenu(SceneNode & node, ComponentBase * selectedComponent);
	
	void editNodeStructure_traverse(const int nodeId);
	
	void updateNodeVisibility();
	
	void markNodeOpenUntilRoot(const int in_nodeId);
	
// todo : this is a test method. remove from scene editor and move elsewhere
	void addNodeFromTemplate_v1(Vec3Arg position, const AngleAxis & angleAxis, const int parentId);
// todo : this is a test method. remove from scene editor and move elsewhere
	int addNodeFromTemplate_v2(Vec3Arg position, const AngleAxis & angleAxis, const int parentId);
// todo : this is a test method. remove from scene editor and move elsewhere
	int addNodesFromScene_v1(const int parentId);
	
	void tickEditor(const float dt, bool & inputIsCaptured);
	
	void validateNodeReferences() const;
	void validateNodeStructure() const;
	
	void performAction_save();
	void performAction_load();
	
	void performAction_undo();
	void performAction_redo();
	
	void performAction_copy();
	void performAction_paste();
	void performAction_duplicate();
	
	void drawNode(const SceneNode & node) const;
	void drawNodes() const;
	
	void drawSceneOpaque() const;
	void drawSceneTranslucent() const;
	
	void drawEditorOpaque() const;
	void drawEditorTranslucent() const;
	void drawEditorGizmos() const;
	
	void drawEditor() const;
	
	bool loadSceneFromLines_nonDestructive(std::vector<std::string> & lines, const char * basePath);
};
