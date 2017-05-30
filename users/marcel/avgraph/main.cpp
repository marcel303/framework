#include "Calc.h"
#include "framework.h"
#include "graph.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"

#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxTypes.h"

#include "vfxNodes/vfxNodeBase.h"
#include "vfxNodes/vfxNodeBinaryOutput.h"
#include "vfxNodes/vfxNodeComposite.h"
#include "vfxNodes/vfxNodeDelayLine.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "vfxNodes/vfxNodeDotDetector.h"
#include "vfxNodes/vfxNodeFsfx.h"
#include "vfxNodes/vfxNodeImageCpuDownsample.h"
#include "vfxNodes/vfxNodeImageCpuEqualize.h"
#include "vfxNodes/vfxNodeImageCpuToGpu.h"
#include "vfxNodes/vfxNodeImageDownsample.h"
#include "vfxNodes/vfxNodeImpulseResponse.h"
#include "vfxNodes/vfxNodeKinect1.h"
#include "vfxNodes/vfxNodeKinect2.h"
#include "vfxNodes/vfxNodeLeapMotion.h"
#include "vfxNodes/vfxNodeLiteral.h"
#include "vfxNodes/vfxNodeLogicSwitch.h"
#include "vfxNodes/vfxNodeMapEase.h"
#include "vfxNodes/vfxNodeMapRange.h"
#include "vfxNodes/vfxNodeMath.h"
#include "vfxNodes/vfxNodeMouse.h"
#include "vfxNodes/vfxNodeNoiseSimplex2D.h"
#include "vfxNodes/vfxNodeOsc.h"
#include "vfxNodes/vfxNodeOscPrimitives.h"
#include "vfxNodes/vfxNodeOscSend.h"
#include "vfxNodes/vfxNodePhysicalSpring.h"
#include "vfxNodes/vfxNodePicture.h"
#include "vfxNodes/vfxNodePictureCpu.h"
#include "vfxNodes/vfxNodeSampleAndHold.h"
#include "vfxNodes/vfxNodeSound.h"
#include "vfxNodes/vfxNodeSpectrum1D.h"
#include "vfxNodes/vfxNodeSpectrum2D.h"
#include "vfxNodes/vfxNodeTime.h"
#include "vfxNodes/vfxNodeTransform2D.h"
#include "vfxNodes/vfxNodeTriggerOnchange.h"
#include "vfxNodes/vfxNodeTriggerTimer.h"
#include "vfxNodes/vfxNodeTriggerTreshold.h"
#include "vfxNodes/vfxNodeTouches.h"
#include "vfxNodes/vfxNodeVideo.h"
#include "vfxNodes/vfxNodeXinput.h"
#include "vfxNodes/vfxNodeYuvToRgb.h"

#include "mediaplayer_new/MPUtil.h"
#include "../libparticle/ui.h"

using namespace tinyxml2;

/*

todo :
+ replace surface type inputs and outputs to image type
+ add VfxImageBase type. let VfxPlug use this type for image type inputs and outputs. has virtual getTexture method
+ add VfxImage_Surface type. let VfxNodeFsfx use this type
+ add VfxPicture type. type name = 'picture'
+ add VfxImage_Texture type. let VfxPicture use this type
+ add VfxVideo type. type name = 'video'
+ add default value to socket definitions
	+ add to XML
	- add ability to reset values to their default in UI
+ add editorValue to node inputs and outputs. let get*** methods use this value when plug is not connected
+ let graph editor set editorValue for nodes. only when editor is set on type definition
+ add socket connection selection. remove connection on BACKSPACE
+ add multiple node selection
# on typing 0..9 let node value editor erase editorValue and begin typing. requires state transition? end editing on ENTER or when selecting another entity
# add ability to increment and decrement editorValue. use mouse Y movement or scroll wheel (?)
+ remember number of digits entered after '.' when editing editorValue. use this information when incrementing/decrementing values
- add zoom in/out
	+ add basic implementation
	- improve zoom in and out behavior
		- clamp max zoom level
		- improve font rendering so it's both resolution independent and supports sub-pixel translation
	+ save/load zoom and focus position to/from XML
	+ add option to quickly reset drag and zoom values
	+ use arrow keys to navigate workspace (when no nodes are selected)
+ add sine, saw, triangle and square oscillators
+ save/load link ids
+ save/load next alloc ids for nodes and links
+ free literal values on graph free
+ recreate DatGui when loading graph / current node gets freed
# prioritize input between DatGui and graph editor. do hit test on DatGui
+ add 'color' type name
+ implement OSC node
+ implement Leap Motion node
- add undo/redo support. just serialize/deserialize graph for every action?
	- note : serialize/deserialize entire graph doesn't work nicely with real-time connection
			 we will need to serialize node on remove and re-add/restore it during undo (also invoking real-time connection)
			 same for links and all other actions. we need to perform the opposite action on undo
+ UI element focus: graph editor vs property editor
+ add ability to collapse nodes, so they take up less space
	+ SPACE to toggle
	+ fix hit test
	+ fix link end point locations
+ passthrough toggle on selection: check if all passthrough. yes? disable passthrough, else enable
+ add socket output value editing, for node types that define it on their outputs. required for literals
+ add enum value types. use combo box to select values
	+ define enums for ease node type
- add ability to randomize input values
# fix white screen issue on Windows when GUI is visible
+ add trigger support
+ add real-time connection
	+ editing values updates values in live version
	+ marking nodes passthrough gets reflected in live
	# disabling nodes and links gets reflected in live -> deferred until later
	+ adding nodes should add nodes in live
	+ removing nodes should remove nodes in live
	+ adding links should add nodes in live
	+ removing links should remove nodes in live
+ add reverse real-time connection
	+ let graph edit sample socket input and output values
		+ let graph edit show a graph of the values when hovering over a socket
+ make it possible to disable nodes
+ make it possible to disable links
- add drag and drop support string literals
+ integrate with UI from libparticle. it supports enums, better color picking, incrementing values up and down in checkboxes
+ add mouse up/down movement support to increment/decrement values of int/float text boxes
+ add option to specify (in UiState) how far text boxes indent their text fields
+ add history of last nodes added
+ insert node on pressing enter in the node type name box
	+ or when pressing one of the suggestion buttons
- add suggestion based purely on matching first part of string (no fuzzy string comparison)
	- order of listing should be : pure matches, fuzzy matches, history. show history once type name text box is made active
	- clear type name text box when adding node
- automatically hide UI when mouse/keyboard is inactive for a while
+ remove 'editor' code
+ allocate literal values for unconnected plugs when live-editing change comes in for input
+ show which nodes and links are actively traversed. add live-connection callback to query activity
	+ add support for one-shot activity
	+ add support for continuous activity
+ show min/max on valueplotter
+ add editor options menu
- improve OSC node
	# purchase and evaluate TouchOSC
	- purchase and evaluate Lemur (by Liine)
	- figure out how to best interop with this software
	- adapt OSC node to fit these products
	- have a learning function, to setup mappings from inputs to outputs
+ save/load editor options to editor XML
+ add editorName to nodes and add a text box to edit it
+ add 2 texture inputs to fsfx node
+ integrate CCL bugfixes and changes
+ add Kinect and Kinect2 nodes
+ add specialized visualizer node, that's present in the editor only. visualize values, but with lots of options for how to. also, make the node resizable
	+ extract visualization code and make it reusable
	+ add support for resizing (special) node types
	# add links to visualizer nodes too to visually 'document' what's the input (?)
		+ add node names to visualizer caption
		+ don't add links as it looks messy and not adding links adds less burden to organizing the visualizers the way you want to
	+ figure out a way for the user to make a visualizer. maybe when dragging a link into empty space?
		+ add visualizer when right clicking on an in- or output socket
- visualize active links and show direction of data flow
- add buttons to manually trigger nodes
	- like the BANG node in max
	- add ability to trigger any input/output trigger (?)
- investigate VVVV's ability to turn everything into vectors of values and to combine lists
- double click node to perform node-specific action
	- add real-time editing callback for double click event
	- open text editor for ps/vs when double clicking fsfx node
	- open container when double clicking container node
- rename _TypeDefinition to _NodeTypeDefinition
+ make time node use local vfx graph instance time, not process time
- add sub-graph container node. to help organize complex graphs
- add mouse cursor to user interface
+ allow trigger inputs to have multiple incoming connections
- make nodes use rounded rectangles
- make links use bezier curves
- add buffer type and add buffer input(s) to fsfx node ?
+ add OpenGL texture routines/helper object. seems we are doing a lot of duplicate/messy/easy to fuck up texture management in various nodes and other places
- add editor option to disable real-time preview
	- add time dilation effect on no input before stopping responding ?
	- add way for UI/editor to tell update loop it's animating something (camera..)
- hide node text until mouse moves close to node ? makes the screen more serene and helps optimize UI drawing
- look at Bitwig 2 for inspiration of node types
- add per-node profiling data
	- measure time tick and draw take
	- report node details. perhaps when hovering above it?
	- add special colouring mode of the node background ? black (zero cpu) -> red -> orange -> yellow (high cpu)
		- add editor option to show cpu/gpu cost
		- add GPU performance markers
- add real-time callback to get node description. report tick/draw time and some other stats/info
	- report texture format, memory usage
	- report cpu image channel count, memory usage, alignment
	- report video playback time
	- report number of dots
	- report analog values for xinput
	- report list of N latest events OSC send node
	- report list of N latest events OSC receive node
- automatically un-fold nodes (temporarily) when the mouse hovers over them ?
	- (temporarily) un-fold node when it is the only selected node. allows connecting sockets
	- (temporarily) un-fold hovered over node when connecting sockets
+ always to dragAndZoom tick ? regard of state
+ fix node dragging as dragAndZoom updates the viewport
	+ remember where (in node space) the mouse went down
	+ calculate new position based on current mouse position in graph space and initial position in node space
- add visualizer for multi-channel data

todo : nodes :
+ add ease node
	+ value
	+ ease type
	+ ease param1
	+ ease param2
	+ mirror?
	+ result
+ add time node
+ add timer node
- add sample.float node
- add sample.image node. outputs r/g/b/a. specify normalized vs screen coords?
+ add impulse response node. measure input impulse response with oscilator at given frequency
+ add sample and hold node. has trigger for input
- add doValuePlotter to ui framework
+ add simplex noise node
+ add binary counter node, outputting 4-8 bit values (1.f or 0.f)
+ add delay node. 4 inputs for delay. take max for delay buffer. delay buffer filled at say fixed 120 hz. 4 outputs delayed values
- add quantize node
- investigate how to render 2D and 3D shapes
- investigate ways of composing/decomposing image data and masking
	- is it possible to create a texture sharing data with a base texture and to just change the rgba swizzling?
- add timeline node (?). trigger events based on markers on a timeline
	- add (re)start input trigger
	- can be very very useful to trigger effects
	- add time! input trigger. performs seek operation
+ add gamepad node
- add MIDI node
- kinect node:
	- don't calculate images when output sockets are not connected (?) or when real-time connection asks for the output ..
	- add player index output ?
	- add point cloud xyz output image. or make a node which can calculate this for us, giving (optional) rgb, depth data, and an enum which controls the projection params (should be set to Kinect1 or Kinect2)
- add pitch control to oscillators ?
+ add restart signal to oscillators ? if input > 0, reset phase
- add 'window' size to square oscillator
+ add spring node ? does physical simulation of a spring
+ add node which sends a trigger when a value changes. send new value as trigger data
+ add node which sends a trigger when a value crosses a treshold
+ add pitch and semitone nodes
- add audio playback node
	+ add play! trigger
	+ add pause! trigger
	+ add resume! trigger
	- add play! output trigger
	- add pause! output trigger
	+ add time output
	- add loop! output
	+ add restart on filename change
	+ add restart on loop change
	+ add BPM and beat output trigger
	- fix issue with output time not stable when paused
	- fix issue with output time not reset on filename change or looping. remember start time? -> capture time on next provide
+ video: add loop input
+ video: add playback speed input
+ add FFT analyser node. output image with amplitude per band
	+ output is image. input = ?
- add note (like C1) to MIDI note
+ add base event ID to OSC send node ?
- add adsr node
+ add random noise node with update frequency. updates random value N times per second
+ add touch pad node which reads data from the MacBook's touch pad (up to ten fingers..)
- add node which can analyze images, detect the dots in them, and send the dots as output
	+ add dot detection node
	- will need a vector socket value type ?
- add CPU image downsample node.
	+ downscale 2x2 or 4x4. would make dot detector operate faster on large video files
	- maybe should work with maximum size constraints and keep downscaling until met ? makes it possible to have varying sized image data incoming and have some kind of size gaurantee on the output
+ add spectrum2d node
+ add dot detector node
- let nodes that allocate a surface push their surface as the current surface, so rendering in dep nodes happens in these surfaces ?
- add channels.toGpu node
	- convert a single channel into a buffer
	- convert all channels for GPU access
	- specify which channels. perhaps using swizzle control ?
		- perhaps add index 1, 2, 3, 4 and set them to -1 by default, except for index 1, which should be 0
		- perhaps add a string input which specifies channels ..

todo : fsfx :
- let FSFX use fsfx.vs vertex shader. don't require effects to have their own vertex shader
- expose uniforms/inputs from FSFX pixel shader
- iterate FSFX pixel shaders and generate type definitions based on FSFX name and exposed uniforms
	- OR: allow nodes to specify dynamic input sockets. use real-time callback to get the list of inputs
	- store inputs by name in nodes (like regular inputs)
	- let FSFX node  resize its inputs dynamically (?)
	- match the dynamic sockets by name ? add to VfxNodeBase to try to get socket based on name if index lookup fails ?
- add standard include file (shaderSource(..)) for FSFX nodes. include params, time, texture1 and 2 and maybe some common functions too

todo : framework :
+ optimize text rendering. use a dynamic texture atlas instead of one separate texture for each glyph. drawText should only emit a single draw call
- add MSDF font rendering support

todo : media player
+ for image analysis we often only need luminance. make it an option to output YUV Y-channel only?
	+ outputting Y+UV is just as cheap as Y only. added planar YUV support.
- add option to disable texture generation
+ add image_cpu value type ?
	+ extend video node so it can output Y/UV image_cpu data
	+ extend video node so it can output RGB image_cpu data
		# requires rewriting media player a little, so consume (acquire) and release of frame data is possible
		+ add Y and UV pointers to MP::VideoFrame
- add image to image_cpu node. default behaviour is to delay by a few frames
+ add image_cpu to image (gpu) node. default behaviour is to upload immediately
- add openAsync call which accepts OpenParams
- add yuvToRgb node
	+ add node and shader
	+ let user select colour space
	- verify color spaces. check what avcodec does, QuickTime player, etc .. there's many ways to go from yuv -> rgb !
+ add image_y, image_u, image_v to video node
+ double check image.toGpu node uses optimized code path for converting single channel source to texture 

todo : UI
+ add drop down list for (large) enums
+ add load/save notifications to UI., maybe a UI message that briefly appears on the bottom. white text on dark background ?
- make nodes into lilly shapes ?
	- the lilly is round so doesn't prefer a certain direction. democracy for the nodes !
	- let the petals become outputs
	- so a lilly is a circle with petals. clicking around the circle's radius will let you reorient the lilly -> which will set the output socket orientations. more freedom, love and happiness for all !
	- let lillies grow when planted
	- 'Victoria Regina', the largest water lilly known to man. found in the rain forest. and on a lamp post somewhere in Rotterdam, with the story of two brothers and their greeen house
	- allocate link colours based on hue, where the hue is 360 / numTypes * indexOfTypeInTypeDefinitionLibrary
		- it will be such a happy sight to behold :-)
- add ability to set node to horizontal or vertical mode. vertical mode hides socket names/is more condensed
	- maybe also a sphere mode ?
	*** I think I like the lilly idea better

reference :
+ http://www.dsperados.com (company based in Utrecht ? send to Stijn)

*/

//#define FILENAME "kinect.xml"
#define FILENAME "yuvtest.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

extern void testDatGui();
extern void testNanovg();
extern void testFourier1d();
extern void testFourier2d();
extern void testChaosGame();
extern void testImpulseResponseMeasurement();
extern void testTextureAtlas();
extern void testDynamicTextureAtlas();
extern void testDotDetector();

struct VfxNodeTriggerAsFloat : VfxNodeBase
{
	enum Input
	{
		kInput_Trigger,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeTriggerAsFloat()
		: VfxNodeBase()
		, outputValue(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Trigger, kVfxPlugType_Trigger);
		addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
	}
	
	virtual void tick(const float dt) override
	{
		const VfxTriggerData * triggerData = getInputTriggerData(kInput_Trigger);
		
		if (triggerData != nullptr)
			outputValue = triggerData->asFloat();
		else
			outputValue = 0.f;
	}
};

VfxNodeBase * createVfxNode(const GraphNodeId nodeId, const std::string & typeName, VfxGraph * vfxGraph)
{
	VfxNodeBase * vfxNode = nullptr;
	
#define DefineNodeImpl(_typeName, _type) \
	else if (typeName == _typeName) \
		vfxNode = new _type();
	
	if (false)
	{
	}
	DefineNodeImpl("intBool", VfxNodeBoolLiteral)
	DefineNodeImpl("intLiteral", VfxNodeIntLiteral)
	DefineNodeImpl("floatLiteral", VfxNodeFloatLiteral)
	DefineNodeImpl("transformLiteral", VfxNodeTransformLiteral)
	DefineNodeImpl("stringLiteral", VfxNodeStringLiteral)
	DefineNodeImpl("colorLiteral", VfxNodeColorLiteral)
	DefineNodeImpl("trigger.asFloat", VfxNodeTriggerAsFloat)
	DefineNodeImpl("time", VfxNodeTime)
	DefineNodeImpl("sampleAndHold", VfxNodeSampleAndHold)
	DefineNodeImpl("binary.output", VfxNodeBinaryOutput)
	DefineNodeImpl("transform.2d", VfxNodeTransform2D)
	DefineNodeImpl("trigger.onchange", VfxNodeTriggerOnchange)
	DefineNodeImpl("trigger.timer", VfxNodeTriggerTimer)
	DefineNodeImpl("trigger.treshold", VfxNodeTriggerTreshold)
	DefineNodeImpl("logic.switch", VfxNodeLogicSwitch)
	DefineNodeImpl("noise.simplex2d", VfxNodeNoiseSimplex2D)
	DefineNodeImpl("sample.delay", VfxNodeDelayLine)
	DefineNodeImpl("impulse.response", VfxNodeImpulseResponse)
	DefineNodeImpl("physical.spring", VfxNodePhysicalSpring)
	DefineNodeImpl("map.range", VfxNodeMapRange)
	DefineNodeImpl("ease", VfxNodeMapEase)
	DefineNodeImpl("math.add", VfxNodeMathAdd)
	DefineNodeImpl("math.sub", VfxNodeMathSub)
	DefineNodeImpl("math.mul", VfxNodeMathMul)
	DefineNodeImpl("math.sin", VfxNodeMathSin)
	DefineNodeImpl("math.cos", VfxNodeMathCos)
	DefineNodeImpl("math.abs", VfxNodeMathAbs)
	DefineNodeImpl("math.min", VfxNodeMathMin)
	DefineNodeImpl("math.max", VfxNodeMathMax)
	DefineNodeImpl("math.sat", VfxNodeMathSat)
	DefineNodeImpl("math.neg", VfxNodeMathNeg)
	DefineNodeImpl("math.sqrt", VfxNodeMathSqrt)
	DefineNodeImpl("math.pow", VfxNodeMathPow)
	DefineNodeImpl("math.exp", VfxNodeMathExp)
	DefineNodeImpl("math.mod", VfxNodeMathMod)
	DefineNodeImpl("math.fract", VfxNodeMathFract)
	DefineNodeImpl("math.floor", VfxNodeMathFloor)
	DefineNodeImpl("math.ceil", VfxNodeMathCeil)
	DefineNodeImpl("math.round", VfxNodeMathRound)
	DefineNodeImpl("math.sign", VfxNodeMathSign)
	DefineNodeImpl("math.hypot", VfxNodeMathHypot)
	DefineNodeImpl("math.pitch", VfxNodeMathPitch)
	DefineNodeImpl("math.semitone", VfxNodeMathSemitone)
	DefineNodeImpl("osc.sine", VfxNodeOscSine)
	DefineNodeImpl("osc.saw", VfxNodeOscSaw)
	DefineNodeImpl("osc.triangle", VfxNodeOscTriangle)
	DefineNodeImpl("osc.square", VfxNodeOscSquare)
	DefineNodeImpl("osc.random", VfxNodeOscRandom)
	else if (typeName == "display")
	{
		vfxNode = new VfxNodeDisplay();
		
		// fixme : move display node id handling out of here. remove nodeId and vfxGraph passed in to this function
		Assert(vfxGraph->displayNodeId == kGraphNodeIdInvalid);
		vfxGraph->displayNodeId = nodeId;
	}
	DefineNodeImpl("mouse", VfxNodeMouse)
	DefineNodeImpl("xinput", VfxNodeXinput)
	DefineNodeImpl("touches", VfxNodeTouches)
	DefineNodeImpl("leap", VfxNodeLeapMotion)
	DefineNodeImpl("kinect1", VfxNodeKinect1)
	DefineNodeImpl("kinect2", VfxNodeKinect2)
	DefineNodeImpl("osc", VfxNodeOsc)
	DefineNodeImpl("osc.send", VfxNodeOscSend)
	DefineNodeImpl("composite", VfxNodeComposite)
	DefineNodeImpl("picture", VfxNodePicture)
	DefineNodeImpl("picture.cpu", VfxNodePictureCpu)
	DefineNodeImpl("video", VfxNodeVideo)
	DefineNodeImpl("sound", VfxNodeSound)
	DefineNodeImpl("spectrum.1d", VfxNodeSpectrum1D)
	DefineNodeImpl("spectrum.2d", VfxNodeSpectrum2D)
	DefineNodeImpl("fsfx", VfxNodeFsfx)
	DefineNodeImpl("image.dots", VfxNodeDotDetector)
	DefineNodeImpl("image.toGpu", VfxNodeImageCpuToGpu)
	DefineNodeImpl("image_cpu.equalize", VfxNodeImageCpuEqualize)
	DefineNodeImpl("image_cpu.downsample", VfxNodeImageCpuDownsample)
	DefineNodeImpl("image.downsample", VfxNodeImageDownsample)
	DefineNodeImpl("yuvToRgb", VfxNodeYuvToRgb)
	else
	{
		logError("unknown node type: %s", typeName.c_str());
	}
	
#undef DefineNodeImpl

	return vfxNode;
}

VfxGraph * constructVfxGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	VfxGraph * vfxGraph = new VfxGraph();
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		if (node.nodeType != kGraphNodeType_Regular)
		{
			continue;
		}
		
		if (node.isEnabled == false)
		{
			continue;
		}
		
		VfxNodeBase * vfxNode = createVfxNode(node.id, node.typeName, vfxGraph);
		
		Assert(vfxNode != nullptr);
		if (vfxNode == nullptr)
		{
			logError("unable to create node");
		}
		else
		{
			vfxNode->isPassthrough = node.isPassthrough;
			
			vfxNode->initSelf(node);
			
			vfxGraph->nodes[node.id] = vfxNode;
		}
	}
	
	for (auto & linkItr : graph.links)
	{
		auto & link = linkItr.second;
		
		if (link.isEnabled == false)
		{
			continue;
		}
		
		auto srcNodeItr = vfxGraph->nodes.find(link.srcNodeId);
		auto dstNodeItr = vfxGraph->nodes.find(link.dstNodeId);
		
		Assert(srcNodeItr != vfxGraph->nodes.end() && dstNodeItr != vfxGraph->nodes.end());
		if (srcNodeItr == vfxGraph->nodes.end() || dstNodeItr == vfxGraph->nodes.end())
		{
			if (srcNodeItr == vfxGraph->nodes.end())
				logError("source node doesn't exist");
			if (dstNodeItr == vfxGraph->nodes.end())
				logError("destination node doesn't exist");
		}
		else
		{
			auto srcNode = srcNodeItr->second;
			auto dstNode = dstNodeItr->second;
			
			auto input = srcNode->tryGetInput(link.srcNodeSocketIndex);
			auto output = dstNode->tryGetOutput(link.dstNodeSocketIndex);
			
			Assert(input != nullptr && output != nullptr);
			if (input == nullptr || output == nullptr)
			{
				if (input == nullptr)
					logError("input node socket doesn't exist");
				if (output == nullptr)
					logError("output node socket doesn't exist");
			}
			else
			{
				input->connectTo(*output);
				
				// note : this may add the same node multiple times to the list of predeps. note that this
				//        is ok as nodes will be traversed once through the travel id + it works nicely
				//        with the live connection as we can just remove the predep and still have one or
				//        references to the predep if the predep was referenced more than once
				srcNode->predeps.push_back(dstNode);
				
				// if this is a trigger, add a trigger target to dstNode
				if (output->type == kVfxPlugType_Trigger)
				{
					VfxNodeBase::TriggerTarget triggerTarget;
					triggerTarget.srcNode = srcNode;
					triggerTarget.srcSocketIndex = link.srcNodeSocketIndex;
					triggerTarget.dstSocketIndex = link.dstNodeSocketIndex;
					
					dstNode->triggerTargets.push_back(triggerTarget);
				}
			}
		}
	}
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		auto typeDefintion = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
		
		if (typeDefintion == nullptr)
			continue;
		
		auto vfxNodeItr = vfxGraph->nodes.find(node.id);
		
		if (vfxNodeItr == vfxGraph->nodes.end())
			continue;
		
		VfxNodeBase * vfxNode = vfxNodeItr->second;
		
		auto & vfxNodeInputs = vfxNode->inputs;
		
		for (auto inputValueItr : node.editorInputValues)
		{
			const std::string & inputName = inputValueItr.first;
			const std::string & inputValue = inputValueItr.second;
			
			for (size_t i = 0; i < typeDefintion->inputSockets.size(); ++i)
			{
				if (typeDefintion->inputSockets[i].name == inputName)
				{
					if (i < vfxNodeInputs.size())
					{
						if (vfxNodeInputs[i].isConnected() == false)
						{
							vfxGraph->connectToInputLiteral(vfxNodeInputs[i], inputValue);
						}
					}
				}
			}
		}
	}
	
	for (auto vfxNodeItr : vfxGraph->nodes)
	{
		auto nodeId = vfxNodeItr.first;
		auto nodeItr = graph.nodes.find(nodeId);
		auto & node = nodeItr->second;
		auto vfxNode = vfxNodeItr.second;
		
		vfxNode->init(node);
	}
	
	return vfxGraph;
}

//

int main(int argc, char * argv[])
{
	//framework.waitForEvents = true;
	
	framework.enableRealTimeEditing = true;
	
	//framework.minification = 2;
	
	framework.enableDepthBuffer = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		initUi();
		
		MP::Util::InitializeLibAvcodec();
		
		//testDatGui();
		
		//testNanovg();
		
		//testChaosGame();
		
		//testFourier1d();
		
		//testFourier2d();
		
		//testImpulseResponseMeasurement();
		
		//testTextureAtlas();
		
		//testDynamicTextureAtlas();
		
		//testDotDetector();
		
		//
		
		GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		{
			XMLDocument * document = new XMLDocument();
			
			if (document->LoadFile("types.xml") == XML_SUCCESS)
			{
				const XMLElement * xmlLibrary = document->FirstChildElement("library");
				
				if (xmlLibrary != nullptr)
				{
					typeDefinitionLibrary->loadXml(xmlLibrary);
				}
			}
			
			delete document;
			document = nullptr;
		}
		
		//
		
		RealTimeConnection * realTimeConnection = new RealTimeConnection();
		
		//
		
		GraphEdit * graphEdit = new GraphEdit(typeDefinitionLibrary);
		
		graphEdit->realTimeConnection = realTimeConnection;

		VfxGraph * vfxGraph = new VfxGraph();
		
		realTimeConnection->vfxGraph = vfxGraph;
		realTimeConnection->vfxGraphPtr = &vfxGraph;
		
		//
		
		graphEdit->load(FILENAME);
		
		//
		
		while (!framework.quitRequested)
		{
			if (graphEdit->editorOptions.realTimePreview)
				framework.waitForEvents = false;
			else
			{
				framework.waitForEvents = true;
			}
			
			framework.process();
			
			if (!graphEdit->editorOptions.realTimePreview)
			{
				framework.timeStep = std::min(framework.timeStep, 1.f / 15.f);
			}
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			const float dt = framework.timeStep;
			
			g_currentVfxGraph = realTimeConnection->vfxGraph;
			
			if (graphEdit->tick(dt))
			{
			}
			else if (keyboard.wentDown(SDLK_s))
			{
				graphEdit->save(FILENAME);
			}
			else if (keyboard.wentDown(SDLK_l))
			{
				graphEdit->load(FILENAME);
			}
			
			g_currentVfxGraph = nullptr;
			
			// fixme : this should be handled by graph edit
			
			if (!graphEdit->selectedNodes.empty())
			{
				const GraphNodeId nodeId = *graphEdit->selectedNodes.begin();
				
				graphEdit->propertyEditor->setNode(nodeId);
			}
			
			framework.beginDraw(31, 31, 31, 255);
			{
				if (vfxGraph != nullptr)
				{
					vfxGraph->tick(framework.timeStep);
					vfxGraph->draw();
				}
				
				graphEdit->draw();
			}
			framework.endDraw();
		}
		
		delete graphEdit;
		graphEdit = nullptr;
		
		delete realTimeConnection;
		realTimeConnection = nullptr;
		
		delete typeDefinitionLibrary;
		typeDefinitionLibrary = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
