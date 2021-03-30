#pragma once

// ecs-sceneEditor
#include "editor/transformGizmos.h"

// ecs-scene
#include "scene.h"

// imgui-framework
#include "imgui-framework.h"

// framework
#include "framework-camera.h"

// std
#include <list>
#include <set>
#include <vector>

//

#if defined(DEBUG)
	#define ENABLE_LOAD_AFTER_SAVE_TEST 0
	#define ENABLE_SAVE_LOAD_TIMING     0
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
	static const int kMainWindowWidth = 370;
	static const int kMaxNodeDisplayNameFilter = 100;
	static const int kMaxComponentTypeNameFilter = 100;
	static const int kMaxParameterFilter = 100;
	
	TypeDB * typeDB = nullptr;
	
	Scene scene; // todo : this should live outside the editor, but referenced
	
	Camera camera;
	bool cameraIsActive = false;
	
#if ENABLE_TRANSFORM_GIZMOS
	TransformGizmo transformGizmo;
#endif

	FrameworkImGuiContext guiContext;
	
	bool showUi = true;
	
	struct Selection
	{
		std::set<int> selectedNodes;
	} selection;
	
	int hoverNodeId = -1;
	
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
		
		int viewportX = 0;
		int viewportY = 0;
		int viewportSx = -1;
		int viewportSy = -1;
	} preview;
	
	struct DeferredState
	{
		std::set<int> nodesToRemove;
		std::vector<SceneNode*> nodesToAdd; // note : this needs to be a vector, to ensure nodes are added in the same order to the scene as they were added here
		std::set<int> nodesToSelect;
		
		bool isFlushed() const
		{
			return
				nodesToRemove.empty() &&
				nodesToAdd.empty() &&
				nodesToSelect.empty();
		}
	};
	
	struct Deferred : DeferredState
	{
		bool isProcessing = false;
		int numActivations = 0;
		
		std::list<DeferredState> stack;
	} deferred;
	
	enum NodeStructureEditingAction
	{
		kNodeStructureEditingAction_None,
		kNodeStructureEditingAction_NodeCopy,
		kNodeStructureEditingAction_NodeCopyTree,
		kNodeStructureEditingAction_NodePasteChild,
		kNodeStructureEditingAction_NodePasteSibling,
		kNodeStructureEditingAction_NodeRemove,
		kNodeStructureEditingAction_NodeAddChild,
		kNodeStructureEditingAction_NodeSceneAttach,
		kNodeStructureEditingAction_NodeSceneAttachUpdate,
		kNodeStructureEditingAction_NodeSceneImport
	};
	
	struct NodeUi
	{
		char nodeDisplayNameFilter[kMaxNodeDisplayNameFilter] = { };
		char componentTypeNameFilter[kMaxComponentTypeNameFilter] = { };
		std::set<int> visibleNodes; // set of visible nodes, when the node structure is filtered, by for instance node name or component type
		std::set<int> nodesToOpen_deferred; // nodes inside this set will be forcibly opened inside the node structure editor and scrolled into view
		std::set<int> nodesToOpen_active; // deferredNodesToOpen will be moved into here when the scene is traversed. we need a copy, so we can clear the deferred set and add new nodes to it during scene traversal itself
		int nodeToGiveFocus = -1;
	} nodeUi;
	
	struct ParameterUi
	{
		char component_filter[kMaxParameterFilter] = { };
		char parameter_filter[kMaxParameterFilter] = { };
		
		bool showAnonymousComponents = false;
	} parameterUi;
	
	struct Undo
	{
		std::vector<std::string> versions;
		int currentVersionIndex = -1;
		
		std::string currentVersion;
	} undo;
	
	struct ClipboardInfo
	{
		std::vector<std::string> lines;
		bool hasNode = false;
		bool hasNodeTree = false;
		bool hasComponent = false;
		std::string componentTypeName;
		int component_lineIndex = -1;
		int component_lineIndent = -1;
	} clipboardInfo;
	
	struct DocumentInfo
	{
		std::string path;
	} documentInfo;
	
	//
	
	void init(TypeDB * in_typeDB);
	void shut();

	void getEditorViewport(int & x, int & y, int & sx, int & sy) const;
	
	SceneNode * raycast(Vec3Arg rayOrigin, Vec3Arg rayDirection, const std::set<int> & nodesToIgnore) const;
	
	/**
	 * Begin a deferred update to the scene structure. Note we need to defer updates, due to
	 * the fact that the scene editor traverses the scene structure at the same time as making
	 * updates to it. Making immediate updates would invalidate and/or complicate scene traversal.
	 */
	void deferredBegin();
	/**
	 * End a deferred update, either committing the changes, or canceling them.
	 */
	void deferredEnd(const bool commit);
	/**
	 * End a deferred update, committing the changes made.
	 * Note that when a deferred update is begun in the scope of another deferred update,
	 * the changes will be committed to the parent scope. When this is the top-level update,
	 * the changes will be committed to the actual scene.
	 */
	void deferredEndCommit();
	/**
	 * Ends a deferred update, canceling all of the made changes. The scene will be left in
	 * the same state it initially was in.
	 */
	void deferredEndCancel();
	
	void removeNodeSubTree(const int nodeId);
	
	void addNodesToAdd();
	void removeNodesToRemove();
	void selectNodesToSelect(const bool append);
	
	/**
	 * The undo system works by initially capturing the new or loaded scene into the undo history. After each (significant)
	 * editing operation, a new undo history item is added. Note that before an editing operation begins, a history item
	 * to return to the previous state already exists. undoCaptureBegin() only exists to assist in catching bugs, due to
	 * missing undoCaptureEnd calls when making edits. undoCaptureEnd() actually captures the scene into the undo
	 * history to ensure the *next* editing operation can be rolled back.
	 */
	bool undoCapture(std::string & text) const; // capture the entire scene into text format
	void undoCaptureBegin(const bool isPristine = true); // begin a scene capture
	void undoCaptureEnd();   // end a scene capture and add it to the undo history
	void undoReset(); // reset the undo history
	
	void editNode(const int nodeId);
	
	void updateClipboardInfo();
	
	bool pasteNodeFromLines(const int parentId, LineReader & line_reader);
	bool pasteNodeTreeFromLines(const int parentId, LineReader & line_reader, const bool keepRootNode);
	
	NodeStructureEditingAction doNodeStructureContextMenu(SceneNode & node);
	
	enum NodeContextMenuResult
	{
		kNodeContextMenuResult_None,
		kNodeContextMenuResult_ComponentShouldBeRemoved,
		kNodeContextMenuResult_ComponentAdded
	};
	
	NodeContextMenuResult doNodeContextMenu(SceneNode & node, ComponentBase * selectedComponent);
	
	NodeStructureEditingAction editNodeStructure_traverse(const int nodeId);
	
	void updateNodeVisibility();
	
	void markNodeOpenUntilRoot(const int in_nodeId);
	
// todo : this is a test method. remove from scene editor and move elsewhere
	int addNodeFromTemplate_v2(Vec3Arg position, const AngleAxis & angleAxis, const int parentId);
	
	int addNodesFromScene(const char * path, const int parentId);
	
	int attachScene(const char * path, const int parentId);
	int updateAttachedScene(const int rootNodeId);
	
	void tickGui(const float dt, bool & inputIsCaptured);
	void tickView(const float dt, bool & inputIsCaptured);
	void tickKeyCommands(bool & inputIsCaptured);
	
	void validateNodeReferences() const;
	void validateNodeStructure() const;
	
	void performAction_save();
	void performAction_save(const char * path, const bool updateDocumentPath);
	void performAction_load(const char * path);
	
	void performAction_undo();
	void performAction_redo();
	
	void performAction_copy(const bool deep) const;
	void performAction_copySceneNodes() const;
	void performAction_copySceneNodeTrees() const;
	bool performAction_pasteChild();
	bool performAction_pasteSibling();
	bool performAction_paste(const int parentNodeId);
	bool performAction_addChild();
	bool performAction_addChild(const int parentNodeId);
	bool performAction_sceneAttach();
	bool performAction_sceneAttachUpdate();
	bool performAction_sceneImport();
	bool performAction_duplicate();
	
	void drawNodeBoundingBox(const SceneNode & node) const;
	void drawNodes() const;
	
	void drawSceneOpaque() const;
	void drawSceneTranslucent() const;
	
	void drawEditorOpaque() const;
	void drawEditorTranslucent() const;
	void drawEditorGizmosOpaque(const bool depthObscuredPass) const;
	void drawEditorGizmosTranslucent() const;
	void drawEditorSelectedNodeLabels() const;
	
	void drawGui() const;
	void drawView() const;
	
	bool loadSceneFromLines_nonDestructive(std::vector<std::string> & lines, const char * basePath);
	
	void resetDocument();
	
	static void updateFilePaths(Scene & scene, TypeDB * typeDB, const char * oldBasePath, const char * newBasePath);
};
