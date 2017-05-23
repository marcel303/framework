#include "Calc.h"
#include "framework.h"
#include "graph.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "../avpaint/video.h"

#include "vfxGraph.h"
#include "vfxTypes.h"

#include "vfxNodes/vfxNodeBase.h"
#include "vfxNodes/vfxNodeComposite.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "vfxNodes/vfxNodeFsfx.h"
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
#include "vfxNodes/vfxNodeTriggerOnchange.h"
#include "vfxNodes/vfxNodeTriggerTimer.h"
#include "vfxNodes/vfxNodeTriggerTreshold.h"
#include "vfxNodes/vfxNodeVideo.h"
#include "vfxNodes/vfxNodeXinput.h"

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
- insert node on pressing enter in the node type name box
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
- add ability to set node to horizontal or vertical mode. vertical mode hides socket names/is more condensed
	- maybe also a sphere mode ?
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
- rename _TypeDefinition to _NodeTypeDefinition
+ make time node use local vfx graph instance time, not process time
- add sub-graph container node. to help organize complex graphs
- add mouse cursor to user interface
+ allow trigger inputs to have multiple incoming connections
- make nodes use rounded rectangles
- make links use bezier curves

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
	- is it possible to create a texture sharing data with a base texture and to just change to rgba swizzling?
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
- add spring node ? does physical simulation of a spring
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
- add FFT analyser node. output image with amplitude per band
	- output is image. input = ?
- add note (like C1) to MIDI note
+ add base event ID to OSC send node ?
- add editor option to disable real-time preview
	- add time dilation effect on no input before stopping responding ?
	- add way for UI/editor to tell update loop it's animating something (camera..)
- hide node text until mouse moves close to node ? makes the screen more serene and helps optimize UI drawing
- add adsr node
- add random noise node with frequency
- look at Bitwig 2 for inspiration of node types
- add touch pad node which reads data from the MacBook's touch pad (up to ten fingers..)
- add node which can analyze images, detect the dots in them, and send the dots as output
	- will need a vector socket value type ?

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
- for image analysis we often only need luminance. make it an option to output YUV Y-channel only?
- add image_cpu value type ?
	- extend video node so it can output Y/UV/RGB image_cpu data
		- requires rewriting media player a little, so consume (acquire) and release of frame data is possible
	- add spectrum2d node
	- add dot detector node
- add image to image_cpu node. default behaviour is to delay by a few frames
- add image_cpu to image node. default behaviour is to upload immediately

todo : UI
+ add drop down list for (large) enums
+ add load/save notifications to UI., maybe a UI message that briefly appears on the bottom. white text on dark background ?

reference :
+ http://www.dsperados.com (company based in Utrecht ? send to Stijn)

*/

#define FILENAME "kinect.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

extern void testDatGui();
extern void testNanovg();
extern void testFourier2d();
extern void testChaosGame();
extern void testImpulseResponseMeasurement();

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

struct VfxNodeBinaryOutput : VfxNodeBase
{
	enum Input
	{
		kInput_Value,
		kInput_Modulo,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value1,
		kOutput_Value2,
		kOutput_Value3,
		kOutput_Value4,
		kOutput_Value5,
		kOutput_Value6,
		kOutput_COUNT
	};
	
	float outputValue[6];
	
	VfxNodeBinaryOutput()
		: VfxNodeBase()
		, outputValue()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kVfxPlugType_Float);
		addInput(kInput_Modulo, kVfxPlugType_Int);
		addOutput(kOutput_Value1, kVfxPlugType_Float, &outputValue[0]);
		addOutput(kOutput_Value2, kVfxPlugType_Float, &outputValue[1]);
		addOutput(kOutput_Value3, kVfxPlugType_Float, &outputValue[2]);
		addOutput(kOutput_Value4, kVfxPlugType_Float, &outputValue[3]);
		addOutput(kOutput_Value5, kVfxPlugType_Float, &outputValue[4]);
		addOutput(kOutput_Value6, kVfxPlugType_Float, &outputValue[5]);
	}
	
	virtual void tick(const float dt) override
	{
		double valueAsDouble = getInputFloat(kInput_Value, 0.f);
		
		valueAsDouble = std::round(valueAsDouble);
		
		int valueAsInt = int(valueAsDouble);
		
		const int modulo = getInputInt(kInput_Modulo, 0);
		
		if (modulo != 0)
			valueAsInt %= modulo;
		
		outputValue[0] = (valueAsInt & (1 << 0)) >> 0;
		outputValue[1] = (valueAsInt & (1 << 1)) >> 1;
		outputValue[2] = (valueAsInt & (1 << 2)) >> 2;
		outputValue[3] = (valueAsInt & (1 << 3)) >> 3;
		outputValue[4] = (valueAsInt & (1 << 4)) >> 4;
		outputValue[5] = (valueAsInt & (1 << 5)) >> 5;
	}
};

struct VfxNodeTransform2d : VfxNodeBase
{
	enum Input
	{
		kInput_X,
		kInput_Y,
		kInput_Scale,
		kInput_ScaleX,
		kInput_ScaleY,
		kInput_Angle,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Transform,
		kOutput_COUNT
	};
	
	VfxTransform transform;
	
	VfxNodeTransform2d()
		: VfxNodeBase()
		, transform()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_X, kVfxPlugType_Float);
		addInput(kInput_Y, kVfxPlugType_Float);
		addInput(kInput_Scale, kVfxPlugType_Float);
		addInput(kInput_ScaleX, kVfxPlugType_Float);
		addInput(kInput_ScaleY, kVfxPlugType_Float);
		addInput(kInput_Angle, kVfxPlugType_Float);
		addOutput(kOutput_Transform, kVfxPlugType_Transform, &transform);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		// todo : parse node.editorValue;
	}
	
	virtual void tick(const float dt) override
	{
		const float x = getInputFloat(kInput_X, 0.f);
		const float y = getInputFloat(kInput_Y, 0.f);
		const float scale = getInputFloat(kInput_Scale, 1.f);
		const float scaleX = getInputFloat(kInput_ScaleX, 1.f);
		const float scaleY = getInputFloat(kInput_ScaleY, 1.f);
		const float angle = getInputFloat(kInput_Angle, 0.f);
		
		Mat4x4 t;
		Mat4x4 s;
		Mat4x4 r;
		
		t.MakeTranslation(x, y, 0.f);
		s.MakeScaling(scale * scaleX, scale * scaleY, 1.f);
		r.MakeRotationZ(Calc::DegToRad(angle));
		
		transform.matrix = t * r * s;
	}
};

struct VfxNodeDelayLine : VfxNodeBase
{
	const static int kSampleRate = 200;
	
	enum Input
	{
		kInput_Value,
		kInput_Delay1,
		kInput_Delay2,
		kInput_Delay3,
		kInput_Delay4,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value1,
		kOutput_Value2,
		kOutput_Value3,
		kOutput_Value4,
		kOutput_COUNT
	};
	
	float outputValue[4];
	
	float dtRemaining;
	
	DelayLine delayLine;
	
	VfxNodeDelayLine()
		: VfxNodeBase()
		, outputValue()
		, dtRemaining(0.f)
		, delayLine()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kVfxPlugType_Float);
		addInput(kInput_Delay1, kVfxPlugType_Float);
		addInput(kInput_Delay2, kVfxPlugType_Float);
		addInput(kInput_Delay3, kVfxPlugType_Float);
		addInput(kInput_Delay4, kVfxPlugType_Float);
		addOutput(kOutput_Value1, kVfxPlugType_Float, &outputValue[0]);
		addOutput(kOutput_Value2, kVfxPlugType_Float, &outputValue[1]);
		addOutput(kOutput_Value3, kVfxPlugType_Float, &outputValue[2]);
		addOutput(kOutput_Value4, kVfxPlugType_Float, &outputValue[3]);
	}
	
	virtual void tick(const float dt) override
	{
		const float delay1 = getInputFloat(kInput_Delay1, 0.f);
		const float delay2 = getInputFloat(kInput_Delay2, 0.f);
		const float delay3 = getInputFloat(kInput_Delay3, 0.f);
		const float delay4 = getInputFloat(kInput_Delay4, 0.f);
	
		const float maxDelay = std::max(std::max(delay1, delay2), std::max(delay3, delay4));
		
		{
			// set delay line length
			
			const int numSamples = maxDelay * kSampleRate;
			
			if (numSamples != delayLine.getLength())
			{
				delayLine.setLength(numSamples);
			}
		}
		
		if (delayLine.getLength() > 0)
		{
			const float value = getInputFloat(kInput_Value, 0.f);
			
			const float dtTotal = dtRemaining + dt;
			
			const int numSamples = std::floor(dtTotal * kSampleRate);
			
			dtRemaining = dtTotal - numSamples / float(kSampleRate);
			
			for (int i = 0; i < numSamples; ++i)
				delayLine.push(value);
			
			const int offset1 = std::min(delayLine.getLength() - 1, int((maxDelay - delay1) * kSampleRate));
			const int offset2 = std::min(delayLine.getLength() - 1, int((maxDelay - delay2) * kSampleRate));
			const int offset3 = std::min(delayLine.getLength() - 1, int((maxDelay - delay3) * kSampleRate));
			const int offset4 = std::min(delayLine.getLength() - 1, int((maxDelay - delay4) * kSampleRate));
			
			outputValue[0] = delayLine.read(offset1);
			outputValue[1] = delayLine.read(offset2);
			outputValue[2] = delayLine.read(offset3);
			outputValue[3] = delayLine.read(offset4);
		}
		else
		{
			outputValue[0] = 0.f;
			outputValue[1] = 0.f;
			outputValue[2] = 0.f;
			outputValue[3] = 0.f;
		}
	}
};

struct VfxNodeImpulseResponse : VfxNodeBase
{
	const static int kSampleRate = 200;
	
	enum Input
	{
		kInput_Value,
		kInput_Frequency,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_ImpulseResponse,
		kOutput_COUNT
	};
	
	float impulseResponse;
	
	float dtRemaining;
	
	DelayLine delayLine;
	
	VfxNodeImpulseResponse()
		: VfxNodeBase()
		, impulseResponse(0.f)
		, dtRemaining(0.f)
		, delayLine()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kVfxPlugType_Float);
		addInput(kInput_Frequency, kVfxPlugType_Float);
		addOutput(kOutput_ImpulseResponse, kVfxPlugType_Float, &impulseResponse);
	}
	
	virtual void tick(const float dt) override
	{
		const float value = getInputFloat(kInput_Value, 0.f);
		const float frequency = getInputFloat(kInput_Frequency, 0.f);
		
		if (frequency <= 0.f)
		{
			delayLine.setLength(0);
		}
		else
		{
			// set delay line length
			
			//const float delay = 1.f / frequency * 4.f;
			const float delay = 1.f / frequency * 2.f;
			
			const int numSamples = delay * kSampleRate;
			
			if (numSamples != delayLine.getLength())
			{
				delayLine.setLength(numSamples);
			}
		}
		
		if (delayLine.getLength() > 0)
		{
			const float dtTotal = dtRemaining + dt;
			
			const int numSamples = std::floor(dtTotal * kSampleRate);
			
			dtRemaining = dtTotal - numSamples / float(kSampleRate);
			
			for (int i = 0; i < numSamples; ++i)
				delayLine.push(value);
		}
		
		//
		
		if (delayLine.getLength() > 0)
		{
			const double twoPi = 2.0 * M_PI;
			
			double measurementPhase = 0.0;
			double measurementStep = twoPi * frequency / kSampleRate;
			
			double sumX = 0.0;
			double sumY = 0.0;
			
			for (int i = 0; i < delayLine.getLength(); ++i)
			{
				const double value = delayLine.read(i);
				
				const double x = std::cos(measurementPhase);
				const double y = std::sin(measurementPhase);
				
				sumX += value * x;
				sumY += value * y;
				
				measurementPhase = std::fmod(measurementPhase + measurementStep, twoPi);
			}
			
			const double avgX = sumX / delayLine.getLength();
			const double avgY = sumY / delayLine.getLength();
			
			impulseResponse = float(std::hypot(avgX, avgY) * 2.0);
		}
	}
};

static VfxNodeBase * createVfxNode(const GraphNodeId nodeId, const std::string & typeName, VfxGraph * vfxGraph)
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
	DefineNodeImpl("transform.2d", VfxNodeTransform2d)
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
	else if (typeName == "display")
	{
		vfxNode = new VfxNodeDisplay();
		
		// fixme : move display node id handling out of here. remove nodeId and vfxGraph passed in to this function
		Assert(vfxGraph->displayNodeId == kGraphNodeIdInvalid);
		vfxGraph->displayNodeId = nodeId;
	}
	DefineNodeImpl("mouse", VfxNodeMouse)
	DefineNodeImpl("xinput", VfxNodeXinput)
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
	else
	{
		logError("unknown node type: %s", typeName.c_str());
	}
	
#undef DefineNodeImpl

	return vfxNode;
}

static VfxGraph * constructVfxGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
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

struct RealTimeConnection : GraphEdit_RealTimeConnection
{
	VfxGraph * vfxGraph;
	VfxGraph ** vfxGraphPtr;
	
	bool isLoading;
	
	RealTimeConnection()
		: GraphEdit_RealTimeConnection()
		, vfxGraph(nullptr)
		, vfxGraphPtr(nullptr)
		, isLoading(false)
	{
	}
	
	virtual void loadBegin() override
	{
		isLoading = true;
		
		delete vfxGraph;
		vfxGraph = nullptr;
		*vfxGraphPtr = nullptr;
	}
	
	virtual void loadEnd(GraphEdit & graphEdit) override
	{
		vfxGraph = constructVfxGraph(*graphEdit.graph, graphEdit.typeDefinitionLibrary);
		*vfxGraphPtr = vfxGraph;
		
		isLoading = false;
	}
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName) override
	{
		if (isLoading)
			return;
		
		logDebug("nodeAdd");
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr == vfxGraph->nodes.end());
		if (nodeItr != vfxGraph->nodes.end())
			return;
		
		//
		
		GraphNode node;
		node.id = nodeId;
		node.typeName = typeName;
		
		//
		
		VfxNodeBase * vfxNode = createVfxNode(nodeId, typeName, vfxGraph);
		
		if (vfxNode == nullptr)
			return;
		
		vfxNode->initSelf(node);
		
		vfxGraph->nodes[node.id] = vfxNode;
		
		//
		
		vfxNode->init(node);
	}
	
	virtual void nodeRemove(const GraphNodeId nodeId) override
	{
		if (isLoading)
			return;
		
		logDebug("nodeRemove");
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return;
		
		auto node = nodeItr->second;
		
		// all links should be removed at this point. there should be no remaining predeps or connected nodes
		Assert(node->predeps.empty());
		//for (auto & input : node->inputs)
		//	Assert(!input.isConnected()); // may be a literal value node with a non-accounted for (in the graph) connection when created directly from socket value
		// todo : iterate all other nodes, to ensure there are no nodes with references back to this node?
		
		delete node;
		node = nullptr;
		
		vfxGraph->nodes.erase(nodeItr);
	}
	
	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		if (isLoading)
			return;
		
		logDebug("linkAdd");
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto srcNodeItr = vfxGraph->nodes.find(srcNodeId);
		auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
		
		Assert(srcNodeItr != vfxGraph->nodes.end() && dstNodeItr != vfxGraph->nodes.end());
		if (srcNodeItr == vfxGraph->nodes.end() || dstNodeItr == vfxGraph->nodes.end())
		{
			if (srcNodeItr == vfxGraph->nodes.end())
				logError("source node doesn't exist");
			if (dstNodeItr == vfxGraph->nodes.end())
				logError("destination node doesn't exist");
			
			return;
		}

		auto srcNode = srcNodeItr->second;
		auto dstNode = dstNodeItr->second;
		
		auto input = srcNode->tryGetInput(srcSocketIndex);
		auto output = dstNode->tryGetOutput(dstSocketIndex);
		
		Assert(input != nullptr && output != nullptr);
		if (input == nullptr || output == nullptr)
		{
			if (input == nullptr)
				logError("input node socket doesn't exist");
			if (output == nullptr)
				logError("output node socket doesn't exist");
			
			return;
		}
		
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
			triggerTarget.srcSocketIndex = srcSocketIndex;
			triggerTarget.dstSocketIndex = dstSocketIndex;
			
			dstNode->triggerTargets.push_back(triggerTarget);
		}
	}
	
	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		if (isLoading)
			return;
		
		logDebug("linkRemove");
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto srcNodeItr = vfxGraph->nodes.find(srcNodeId);
		
		Assert(srcNodeItr != vfxGraph->nodes.end());
		if (srcNodeItr == vfxGraph->nodes.end())
			return;
		
		auto srcNode = srcNodeItr->second;
		
		auto input = srcNode->tryGetInput(srcSocketIndex);
		
		Assert(input != nullptr);
		if (input == nullptr)
			return;
		
		Assert(input->isConnected());
		input->disconnect();
		
		{
			// attempt to remove dst node from predeps
			
			auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
			
			Assert(dstNodeItr != vfxGraph->nodes.end());
			if (dstNodeItr != vfxGraph->nodes.end())
			{
				auto dstNode = dstNodeItr->second;
				
				bool foundPredep = false;
				
				for (auto i = srcNode->predeps.begin(); i != srcNode->predeps.end(); ++i)
				{
					VfxNodeBase * vfxNode = *i;
					
					if (vfxNode == dstNode)
					{
						srcNode->predeps.erase(i);
						foundPredep = true;
						break;
					}
				}
				
				Assert(foundPredep);
			}
		}
		
		// if this is a link hooked up to a trigger, remove the TriggerTarget from dstNode
		
		if (input->type == kVfxPlugType_Trigger)
		{
			auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
			
			Assert(dstNodeItr != vfxGraph->nodes.end());
			if (dstNodeItr != vfxGraph->nodes.end())
			{
				auto dstNode = dstNodeItr->second;
				
				bool foundTriggerTarget = false;
				
				for (auto triggerTargetItr = dstNode->triggerTargets.begin(); triggerTargetItr != dstNode->triggerTargets.end(); ++triggerTargetItr)
				{
					auto & triggerTarget = *triggerTargetItr;
					
					if (triggerTarget.srcNode == srcNode && triggerTarget.srcSocketIndex == srcSocketIndex)
					{
						dstNode->triggerTargets.erase(triggerTargetItr);
						foundTriggerTarget = true;
						break;
					}
				}
				
				Assert(foundTriggerTarget);
			}
		}
	}
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override
	{
		if (isLoading)
			return;
		
		//logDebug("setNodeIsPassthrough called for nodeId=%d, isPassthrough=%d", int(nodeId), int(isPassthrough));
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return;
		
		auto node = nodeItr->second;
		
		node->isPassthrough = isPassthrough;
	}
	
	static bool setPlugValue(VfxPlug * plug, const std::string & value)
	{
		switch (plug->type)
		{
		case kVfxPlugType_None:
			return false;
		case kVfxPlugType_Bool:
			plug->getRwBool() = Parse::Bool(value);
			return true;
		case kVfxPlugType_Int:
			plug->getRwInt() = Parse::Int32(value);
			return true;
		case kVfxPlugType_Float:
			plug->getRwFloat() = Parse::Float(value);
			return true;
			
		case kVfxPlugType_Transform:
			return false;
			
		case kVfxPlugType_String:
			plug->getRwString() = value;
			return true;
		case kVfxPlugType_Color:
			plug->getRwColor() = Color::fromHex(value.c_str());
			return true;
			
		case kVfxPlugType_Image:
			return false;
		case kVfxPlugType_ImageCpu:
			return false;
		case kVfxPlugType_Trigger:
			return false;
		}
		
		Assert(false); // all cases should be handled explicitly
		return false;
	}
	
	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value) override
	{
		if (isLoading)
			return;
		
		//logDebug("setSrcSocketValue called for nodeId=%d, srcSocket=%s", int(nodeId), srcSocketName.c_str());
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return;
		
		auto node = nodeItr->second;
		
		auto input = node->tryGetInput(srcSocketIndex);
		
		Assert(input != nullptr);
		if (input == nullptr)
			return;
		
		if (input->isConnected())
		{
			setPlugValue(input, value);
		}
		else
		{
			vfxGraph->connectToInputLiteral(*input, value);
		}
	}
	
	static bool getPlugValue(VfxPlug * plug, std::string & value)
	{
		switch (plug->type)
		{
		case kVfxPlugType_None:
			return false;
		case kVfxPlugType_Bool:
			value = String::ToString(plug->getBool());
			return true;
		case kVfxPlugType_Int:
			value = String::ToString(plug->getInt());
			return true;
		case kVfxPlugType_Float:
			value = String::FormatC("%f", plug->getFloat());
			return true;
		case kVfxPlugType_Transform:
			return false;
		case kVfxPlugType_String:
			value = plug->getString();
			return true;
		case kVfxPlugType_Color:
			{
				const Color & color = plug->getColor();
				value = color.toHexString(true);
				return true;
			}
		case kVfxPlugType_Image:
			{
				auto image = plug->getImage();
				value = String::FormatC("%d [%d x %d]", image->getTexture(), image->getSx(), image->getSy());
				return true;
			}
		case kVfxPlugType_ImageCpu:
			{
				auto image = plug->getImageCpu();
				if (image == nullptr)
					return false;
				else
				{
					value = String::FormatC("[%d x %d]", image->sx, image->sy);
					return true;
				}
			}
		case kVfxPlugType_Trigger:
			{
				const VfxTriggerData & triggerData = plug->getTriggerData();
				
				if (triggerData.type == kVfxTriggerDataType_Bool ||
					triggerData.type == kVfxTriggerDataType_Int)
				{
					value = String::FormatC("%d", triggerData.asInt());
					return true;
				}
				else if (triggerData.type == kVfxTriggerDataType_Float)
				{
					value = String::FormatC("%f", triggerData.asFloat());
					return true;
				}
				else if (triggerData.type == kVfxTriggerDataType_None)
				{
					return false;
				}
				else
				{
					Assert(false);
					return false;
				}
			}
			return false;
		}
		
		Assert(false); // all cases should be handled explicitly
		return false;
	}
	
	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value) override
	{
		if (isLoading)
			return false;
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return false;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return false;
		
		auto node = nodeItr->second;
		
		auto input = node->tryGetInput(srcSocketIndex);
		
		Assert(input != nullptr);
		if (input == nullptr)
			return false;
		
		if (input->isConnected() == false)
			return false;
		
		return getPlugValue(input, value);
	}
	
	virtual void setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value) override
	{
		if (isLoading)
			return;
		
		//logDebug("setDstSocketValue called for nodeId=%d, dstSocket=%s", int(nodeId), dstSocketName.c_str());
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return;
		
		auto node = nodeItr->second;
		
		auto output = node->tryGetOutput(dstSocketIndex);
		
		Assert(output != nullptr);
		if (output == nullptr)
			return;
		
		setPlugValue(output, value);
	}
	
	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override
	{
		if (isLoading)
			return false;
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return false;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return false;
		
		auto node = nodeItr->second;
		
		auto output = node->tryGetOutput(dstSocketIndex);
		
		Assert(output != nullptr);
		if (output == nullptr)
			return false;
		
		return getPlugValue(output, value);
	}
	
	virtual int nodeIsActive(const GraphNodeId nodeId) override
	{
		if (isLoading)
			return false;
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return false;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return false;
		
		auto node = nodeItr->second;
		
		int result = 0;
		
		if (node->lastDrawTraversalId + 1 == vfxGraph->nextDrawTraversalId)
			result |= kActivity_Continuous;
		
		if (node->editorIsTriggered)
		{
			result |= kActivity_OneShot;
			
			node->editorIsTriggered = false;
		}
		
		return result;
	}
	
	virtual int linkIsActive(const GraphLinkId linkId) override
	{
		if (isLoading)
			return false;
		
		return false;
	}
};

//

#include "BoxAtlas.h"
#include "textureatlas.h"
#include "Timer.h"

#if 0

struct TextureAtlas
{
	BoxAtlas a;
	
	GLuint texture;
	
	TextureAtlas()
		: a()
		, texture(0)
	{
	}
	
	~TextureAtlas()
	{
		init(0, 0);
	}
	
	void init(const int sx, const int sy)
	{
		a.init(sx, sy);
		
		if (texture != 0)
		{
			glDeleteTextures(1, &texture);
			texture = 0;
			checkErrorGL();
		}
		
		if (sx > 0 || sy > 0)
		{
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, sx, sy);
			checkErrorGL();
		}
	}
	
	BoxAtlasElem * tryAlloc(const uint8_t * values, const int sx, const int sy)
	{
		auto e = a.tryAlloc(sx, sy);
		
		if (e != nullptr)
		{
			if (values != nullptr)
			{
				glBindTexture(GL_TEXTURE_2D, texture);
				
				GLint restoreUnpack;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
				checkErrorGL();
				
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexSubImage2D(GL_TEXTURE_2D, 0, e->x, e->y, sx, sy, GL_RED, GL_UNSIGNED_BYTE, values);
				checkErrorGL();
				
				glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
				checkErrorGL();
			}
			
			return e;
		}
		else
		{
			return nullptr;
		}
	}
	
	void free(BoxAtlasElem *& e)
	{
		if (e != nullptr)
		{
			Assert(e->isAllocated);
			
			uint8_t * zeroes = (uint8_t*)alloca(e->sx * e->sy);
			memset(zeroes, 0, e->sx * e->sy);
			
			glBindTexture(GL_TEXTURE_2D, texture);
			
			GLint restoreUnpack;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
			checkErrorGL();
			
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexSubImage2D(GL_TEXTURE_2D, 0, e->x, e->y, e->sx, e->sy, GL_RED, GL_UNSIGNED_BYTE, zeroes);
			checkErrorGL();
			
			glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
			checkErrorGL();
			
			a.free(e);
		}
	}
	
	void clearTexture(GLuint texture, float r, float g, float b, float a)
	{
		GLuint oldBuffer = 0;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
		checkErrorGL();
		{
			GLuint frameBuffer = 0;
			
			glGenFramebuffers(1, &frameBuffer);
			checkErrorGL();
			
			glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
			checkErrorGL();
			{
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			glDeleteFramebuffers(1, &frameBuffer);
			frameBuffer = 0;
			checkErrorGL();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
		checkErrorGL();
	}
	
	bool makeBigger(const int sx, const int sy)
	{
		if (sx < a.sx || sy < a.sy)
			return false;
		
		// update texture
		
		GLuint oldBuffer = 0;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
		checkErrorGL();
		
		//
		
		GLuint newTexture = 0;
		
		glGenTextures(1, &newTexture);
		glBindTexture(GL_TEXTURE_2D, newTexture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, sx, sy);
		checkErrorGL();
		
		clearTexture(newTexture, 0, 0, 0, 0);
		
		GLuint frameBuffer = 0;
		
		glGenFramebuffers(1, &frameBuffer);
		checkErrorGL();
		
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		checkErrorGL();
		
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, a.sx, a.sy);
		checkErrorGL();
		
		glDeleteFramebuffers(1, &frameBuffer);
		frameBuffer = 0;
		
		glDeleteTextures(1, &texture);
		texture = 0;
		
		//
		
		glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
		checkErrorGL();
		
		//
		
		texture = newTexture;
		
		a.makeBigger(sx, sy);
		
		return true;
	}
	
	bool optimize()
	{
		BoxAtlasElem elems[a.kMaxElems];
		memcpy(elems, a.elems, sizeof(elems));
		
		if (a.optimize() == false)
		{
			return false;
		}
		
		// update texture
		
		GLuint oldBuffer = 0;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
		checkErrorGL();
		
		//
		
		GLuint newTexture = 0;
		
		glGenTextures(1, &newTexture);
		glBindTexture(GL_TEXTURE_2D, newTexture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, a.sx, a.sy);
		checkErrorGL();
		
		clearTexture(newTexture, 0, 0, 0, 0);
		
		GLuint frameBuffer = 0;
		
		glGenFramebuffers(1, &frameBuffer);
		checkErrorGL();
		
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		checkErrorGL();
		
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		checkErrorGL();
		
		for (int i = 0; i < a.kMaxElems; ++i)
		{
			auto & eSrc = elems[i];
			auto & eDst = a.elems[i];
			
			Assert(eSrc.isAllocated == eDst.isAllocated);
			
			if (eSrc.isAllocated)
			{
				glCopyTexSubImage2D(GL_TEXTURE_2D, 0, eDst.x, eDst.y, eSrc.x, eSrc.y, eSrc.sx, eSrc.sy);
				checkErrorGL();
			}
		}
		
		glDeleteFramebuffers(1, &frameBuffer);
		frameBuffer = 0;
		checkErrorGL();
		
		glDeleteTextures(1, &texture);
		texture = 0;
		checkErrorGL();
		
		//
		
		glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
		checkErrorGL();
		
		//
		
		texture = newTexture;
		
		return true;
	}
};

#endif

static void testTextureAtlas()
{
	{
		TextureAtlas ta;
		
		ta.init(128, 128, GL_R8, false, false, nullptr);
		
		bool success = true;
		
		uint8_t * values = (uint8_t*)malloc(1024 * 1024);
		memset(values, 0xff, 1024 * 1024);
		
		const uint64_t tr1 = g_TimerRT.TimeUS_get();
		
		success &= nullptr != ta.tryAlloc(values, 128, 128, GL_RED, GL_UNSIGNED_BYTE);
		
		success &= false != ta.makeBigger(256, 256);
		
		success &= nullptr != ta.tryAlloc(values, 128, 128, GL_RED, GL_UNSIGNED_BYTE);
		success &= nullptr != ta.tryAlloc(values, 128, 128, GL_RED, GL_UNSIGNED_BYTE);
		success &= nullptr != ta.tryAlloc(values, 128, 128, GL_RED, GL_UNSIGNED_BYTE);
		
		success &= false != ta.makeBigger(512, 256);
		
		for (int i = 0; i < 500; ++i)
		{
			success &= nullptr != ta.tryAlloc(values, random(4, 12), random(4, 12), GL_RED, GL_UNSIGNED_BYTE);
		}
		
		const uint64_t tr2 = g_TimerRT.TimeUS_get();
		
		printf("resize test: time=%.2fms, success=%d\n", (tr2 - tr1) / 1000.f, success ? 1 : 0);
		
		delete[] values;
		values = nullptr;
		
		do
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_o))
			{
				const uint64_t to1 = g_TimerRT.TimeUS_get();
				
				const bool success = ta.optimize();
				
				const uint64_t to2 = g_TimerRT.TimeUS_get();
				
				printf("optimize: time=%.2fms, success=%d\n", (to2 - to1) / 1000.f, success ? 1 : 0);
			}
			
			framework.beginDraw(0, 0, 0, 0);
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(ta.texture);
				{
					setColor(colorWhite);
					drawRect(0, 0, ta.a.sx, ta.a.sy);
				}
				gxSetTexture(0);
				popBlend();
				
			#if 1
				setColor(127, 0, 0);
				for (auto & e : ta.a.elems)
					if (e.isAllocated)
						drawRectLine(e.x, e.y, e.x + e.sx, e.y + e.sy);
			#endif
			}
			framework.endDraw();
		} while (!keyboard.wentDown(SDLK_SPACE));
	}
}

static void testDynamicTextureAtlas()
{
	{
		BoxAtlas a;
		
		a.init(128, 128);
		
		bool success = true;
		
		const uint64_t tr1 = g_TimerRT.TimeUS_get();
		
		success &= nullptr != a.tryAlloc(128, 128);
		
		success &= false != a.makeBigger(256, 256);
		
		success &= nullptr != a.tryAlloc(128, 128);
		success &= nullptr != a.tryAlloc(128, 128);
		success &= nullptr != a.tryAlloc(128, 128);
		
		success &= false != a.makeBigger(512, 256);
		
		for (int i = 0; i < 500; ++i)
		{
			success &= nullptr != a.tryAlloc(random(4, 8), random(4, 8));
		}
		
		const uint64_t tr2 = g_TimerRT.TimeUS_get();
		
		printf("resize test: time=%.2fms, success=%d\n", (tr2 - tr1) / 1000.f, success ? 1 : 0);
	}
	
	BoxAtlas a;
	
	a.init(1024, 1024);
	
	const int kMaxElems = 1000;
	//const int kMaxElems = 100;
	
	BoxAtlasElem * elems[kMaxElems];
	int numElems = 0;
	
	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
	for (int i = 0; i < kMaxElems; ++i)
	{
		const int sx = random(8, 32);
		const int sy = random(8, 32);
		
		BoxAtlasElem * e = a.tryAlloc(sx, sy);
		
		if (e != nullptr)
		{
			//logDebug("alloc %d, %d @ %d, %d", sx, sy, e->x, e->y);
			
			elems[numElems++] = e;
		}
		else
		{
			logDebug("failed to alloc %d, %d", sx, sy);
		}
	}
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
	
	printf("insert took %.2fms for %d elems (%d/%d)\n", (t2 - t1) / 1000.f, kMaxElems, numElems, kMaxElems);
	
	const uint64_t to1 = g_TimerRT.TimeUS_get();
	
	a.optimize();
	
	const uint64_t to2 = g_TimerRT.TimeUS_get();
	
	printf("optimize took %.2fms for %d elems (%d/%d)\n", (to2 - to1) / 1000.f, kMaxElems, numElems, kMaxElems);
	
	for (int i = 0; i < numElems; ++i)
	{
		a.free(elems[i]);
	}
	
	numElems = 0;
	
	//
	
	for (int i = 0; i < 6; ++i)
	{
		const int sx = random(64, 128);
		const int sy = random(64, 128);
		
		BoxAtlasElem * e = a.tryAlloc(sx, sy);
		
		if (e != nullptr)
		{
			logDebug("alloc %d, %d @ %d, %d", sx, sy, e->x, e->y);
			
			elems[numElems++] = e;
		}
		else
		{
			logDebug("failed to alloc %d, %d", sx, sy);
		}
	}
	
	for (int i = 0; i < numElems; ++i)
	{
		a.free(elems[i]);
	}
	
	numElems = 0;
	
	//
	
	TextureAtlas ta;
	
	ta.init(GFX_SX/2, GFX_SY/2, GL_R8, false, false, nullptr);
	
	const uint64_t tt1 = g_TimerRT.TimeUS_get();

	for (int i = 0; i < kMaxElems; ++i)
	{
		const int sx = random(8, 32);
		const int sy = random(8, 32);
		
		uint8_t values[sx * sy];
		
		for (int i = 0; i < sx * sy; ++i)
			values[i] = i;
		
		BoxAtlasElem * e = ta.tryAlloc(values, sx, sy, GL_RED, GL_UNSIGNED_BYTE);
		
		if (e != nullptr)
		{
			logDebug("alloc %d, %d @ %d, %d", sx, sy, e->x, e->y);
			
			elems[numElems++] = e;
		}
		else
		{
			logDebug("failed to alloc %d, %d", sx, sy);
		}
	}

	const uint64_t tt2 = g_TimerRT.TimeUS_get();
	
	printf("OpenGL insert took %.2fms for %d elems (%d/%d)\n", (tt2 - tt1) / 1000.f, kMaxElems, numElems, kMaxElems);
	
#if 1
	for (int i = 0; i < numElems; ++i)
	{
		ta.free(elems[i]);
	}
	
	numElems = 0;
#endif
	
	const int kAnimSize = 1000;
	int animItr = 0;
	
	do
	{
		framework.process();
		
		//
		
		if (mouse.isDown(BUTTON_LEFT))
			SDL_Delay(500);
		
		//
		
	#if 1
		if (animItr == kAnimSize)
		{
			for (int i = 0; i < numElems; ++i)
			{
				ta.free(elems[i]);
			}
			
			numElems = 0;
			animItr = 0;
		}
		
		const int numToAdd = mouse.x * 50 / GFX_SX;
		
	//for (int i = 0; i < kAnimSize; ++i)
	for (int i = 0; i < numToAdd; ++i)
		if (numElems < kMaxElems && animItr < kAnimSize)
		{
			const int sx = random(4, 100);
			const int sy = random(4, 100);
			
			uint8_t values[sx * sy];
			
			for (int i = 0; i < sx * sy; ++i)
				values[i] = i;
			
			BoxAtlasElem * e = ta.tryAlloc(values, sx, sy, GL_RED, GL_UNSIGNED_BYTE);
			
			if (e != nullptr)
			{
				logDebug("alloc %d, %d @ %d, %d", sx, sy, e->x, e->y);
				
				elems[numElems++] = e;
			}
			else
			{
				logDebug("failed to alloc %d, %d", sx, sy);
			}
			
			animItr++;
		}
	#endif
	
		if (keyboard.isDown(SDLK_o))
		{
			const uint64_t to1 = g_TimerRT.TimeUS_get();
			
			const bool success = ta.optimize();
			
			const uint64_t to2 = g_TimerRT.TimeUS_get();
			
			printf("optimize: time=%.2fms, success=%d\n", (to2 - to1) / 1000.f, success ? 1 : 0);
		}
		
		if (keyboard.isDown(SDLK_b))
		{
			const uint64_t tb1 = g_TimerRT.TimeUS_get();
			
			const bool success = ta.makeBigger(ta.a.sx + 4, ta.a.sy + 4);
			
			const uint64_t tb2 = g_TimerRT.TimeUS_get();
			
			printf("makeBigger: time=%.2fms, success=%d\n", (tb2 - tb1) / 1000.f, success ? 1 : 0);
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushBlend(BLEND_OPAQUE);
			gxSetTexture(ta.texture);
			{
				setColor(colorWhite);
				drawRect(0, 0, ta.a.sx, ta.a.sy);
			}
			gxSetTexture(0);
			popBlend();
			
		#if 1
			setColor(colorRed);
			for (auto & e : ta.a.elems)
				if (e.isAllocated)
					drawRectLine(e.x, e.y, e.x + e.sx, e.y + e.sy);
			
			BoxAtlas ao;
			ao.copyFrom(ta.a);
			
			ao.optimize();
			
			setColor(colorGreen);
			for (auto & e : ao.elems)
				if (e.isAllocated)
					drawRectLine(e.x, e.y, e.x + e.sx, e.y + e.sy);
		#endif
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	logDebug("done!");
}

#include "image.h"
#include "Timer.h"

#define USE_GRID 1 // improves matching result as we find the closes island when enabled. also greatly increases detection speed when there's many dots being detected
#define USE_SSE2 1// improves tresholding and detection speed
#define USE_READPIXELS_OPTIMIZE 1
#define USE_READPIXELS_FENCES 0

struct DotIsland
{
	int totalX;
	int totalY;
	int numPixels;
	
	int x;
	int y;
	
	int minX;
	int minY;
	int maxX;
	int maxY;
	
#if USE_GRID
	uint16_t next;
#endif
};

template <typename T>
static bool isValidIndex(const T value)
{
	return value != T(-1);
}

static int detectDots(const uint8_t * data, const int sx, const int sy, const int maxRadius, DotIsland * __restrict islands, int maxIslands, const bool useGrid)
{
	int numIslands = 0;
	
	const int maxRadiusSq = maxRadius * maxRadius;
	
#if USE_GRID
	const int gridSx = sx / maxRadius + 1;
	const int gridSy = sy / maxRadius + 1;
	
	uint16_t grid[gridSx][gridSy];
	memset(grid, 0xff, sizeof(grid));
#endif
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict dataLine = data + y * sx;
		
	#if USE_GRID
		const int gridY = y / maxRadius;
	#endif
	
		for (int x = 0; x < sx; )
		{
		#if USE_SSE2
			// see if ant of the next 16 pixels passes the treshold test. if not, skip the next 16 pixels
			// we only do this test once every 16 pixels, to avoid redundant calculations
			
			if ((x & 15) == 0)
			{
				const __m128i data16 = _mm_loadu_si128((const __m128i*)(dataLine + x));
				
				if (_mm_movemask_epi8(data16) == 0)
				{
					x += 16;
					continue;
				}
			}
		#endif
			
			if (dataLine[x])
			{
			#if USE_GRID
				const int gridX = x / maxRadius;
				
				if (useGrid)
				{
					for (int gx = gridX - 2; gx <= gridX + 1; ++gx)
					{
						for (int gy = gridY - 2; gy <= gridY + 1; ++gy)
						{
							if (gx >= 0 && gx < gridSx && gy >= 0 && gy < gridSy)
							{
								for (uint16_t islandIndex = grid[gx][gy]; isValidIndex(islandIndex); islandIndex = islands[islandIndex].next)
								{
									auto & island = islands[islandIndex];
									
									const int dx = x - island.x;
									const int dy = y - island.y;
									const int dsSq = dx * dx + dy * dy;
									
									if (dsSq < maxRadiusSq)
									{
										island.totalX += x;
										island.totalY += y;
										island.numPixels++;
										
										island.x = island.totalX / island.numPixels;
										island.y = island.totalY / island.numPixels;
										
										island.minX = std::min(island.minX, x);
										island.minY = std::min(island.minY, y);
										island.maxX = std::max(island.maxX, x);
										island.maxY = std::max(island.maxY, y);
										
										goto foundIsland;
									}
								}
							}
						}
					}
				}
				else
			#endif
				{
					for (int i = 0; i < numIslands; ++i)
					{
						auto & island = islands[i];
						
						const int dx = x - island.x;
						const int dy = y - island.y;
						const int dsSq = dx * dx + dy * dy;
						
						if (dsSq < maxRadiusSq)
						{
							island.totalX += x;
							island.totalY += y;
							island.numPixels++;
							
							island.x = island.totalX / island.numPixels;
							island.y = island.totalY / island.numPixels;
							
							island.minX = std::min(island.minX, x);
							island.minY = std::min(island.minY, y);
							island.maxX = std::max(island.maxX, x);
							island.maxY = std::max(island.maxY, y);
							
							goto foundIsland;
						}
					}
				}
				
				// we didn't find an island close to this pixel. add a new one
				
				if (numIslands < maxIslands)
				{
					auto & island = islands[numIslands];
					
					island.totalX = x;
					island.totalY = y;
					island.numPixels = 1;
					
					island.x = x;
					island.y = y;
					
					island.minX = x;
					island.minY = y;
					island.maxX = x;
					island.maxY = y;
					
				#if USE_GRID
					if (useGrid)
					{
						island.next = grid[gridX][gridY];
						grid[gridX][gridY] = numIslands;
					}
				#endif
				
					numIslands++;
				}
				
			foundIsland:
				do { } while (false); // clang compiler errors if there is no expression of a label
			}
			
			++x;
		}
	}
	
	return numIslands;
}

#if USE_SSE2

static __m128i _mm_cmple_epu8(__m128i x, __m128i y)
{
	return _mm_cmpeq_epi8(_mm_min_epu8(x, y), x);
}

static __m128i _mm_cmpge_epu8(__m128i x, __m128i y)
{
	return _mm_cmple_epu8(y, x);
}

#endif

static void testDotDetector()
{
	const int sx = 768;
	const int sy = 512;
	Surface surface(sx, sy, false, false, SURFACE_R8);
	
	// make sure the surface turns up black and white instead of shades of red when we draw it, by applying a swizzle mask
	glBindTexture(GL_TEXTURE_2D, surface.getTexture());
	GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();
	
	uint8_t * surfaceData = new uint8_t[sx * sy];
	memset(surfaceData, 0xff, sizeof(uint8_t) * sx * sy);
	
	uint8_t * maskedData = new uint8_t[sx * sy];
	
	struct Circle
	{
		float x;
		float y;
		float angle;
		float speed;
		float timer;
		float timerRcp;
		
		void randomize()
		{
			x = random(0, sx);
			y = random(0, sy);
			angle = random(0.f, float(M_PI * 2.f));
			speed = random(50.f, 200.f);
			timer = random(1.f, 3.f);
			timerRcp = 1.f / timer;
		}
		
		void tick(const float dt)
		{
			const float dx = std::cosf(angle);
			const float dy = std::sinf(angle);
			
			x += dx * speed * dt;
			y += dy * speed * dt;
			
			timer -= framework.timeStep;
			
			if (timer <= 0.f)
			{
				randomize();
			}
		}
	};
	
	const int kNumCircles = 100;
	Circle circles[kNumCircles];
	
	for (auto & c : circles)
	{
		c.randomize();
	}
	
	bool useVideo = true;
	
	MediaPlayer mp;
	
	if (useVideo)
	{
		mp.openAsync("mocap5.mp4", false);
	}
	
	//
	
	uint64_t averageTime = 0;
	uint64_t averageTimeR = 0;
	uint64_t averageTimeM = 0;
	
	bool useGrid = true;
	int tresholdFunction = 0;
	int tresholdValue = 32;
	int maxRadius = 10;
	
#if USE_READPIXELS_OPTIMIZE
	const int kNumPixelBuffers = 2;
	bool hasPixels = false;
	int pixelBufferIndex = 0;
	GLuint pixelBuffers[kNumPixelBuffers] = { };
#if USE_READPIXELS_FENCES
	GLsync pixelSyncs[kNumPixelBuffers] = { };
#endif
	
	for (int i = 0; i < kNumPixelBuffers; ++i)
	{
		glGenBuffers(1, &pixelBuffers[i]);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffers[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, sx * sy, 0, GL_DYNAMIC_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		checkErrorGL();
	}
#endif

	do
	{
		framework.process();
		
		//
		
		if (keyboard.wentDown(SDLK_g))
			useGrid = !useGrid;
		if (keyboard.wentDown(SDLK_t))
			tresholdFunction = (tresholdFunction + 1) % 2;
		if (keyboard.wentDown(SDLK_UP, true))
			tresholdValue += 1;
		if (keyboard.wentDown(SDLK_DOWN, true))
			tresholdValue -= 1;
		if (keyboard.wentDown(SDLK_a, true))
			maxRadius += 1;
		if (keyboard.wentDown(SDLK_z, true) && maxRadius > 1)
			maxRadius -= 1;
		
		//
		
		const float dt = framework.timeStep * (mouse.isDown(BUTTON_LEFT) ? mouse.y / float(GFX_SY) * 4.f : 1.f);
		
		//
		
		if (mp.isActive(mp.context))
		{
			mp.presentTime += dt;
			
			mp.tick(mp.context);
			
			if (mp.context->hasPresentedLastFrame)
			{
				const std::string filename = mp.context->openParams.filename;
				
				mp.close(false);
				
				mp.presentTime = 0.0;
				
				mp.openAsync(filename.c_str(), false);
			}
		}
		
		// generate a new pattern of moving dots
		
		uint64_t tr1;
		uint64_t tr2;
		
		pushSurface(&surface);
		{
			surface.clear(255, 255, 255, 255);
			
			if (useVideo)
			{
				if (mp.getTexture())
				{
					pushBlend(BLEND_OPAQUE);
					gxSetTexture(mp.getTexture());
					setColor(colorWhite);
					drawRect(0, 0, sx, sy);
					gxSetTexture(0);
					popBlend();
				}
			}
			else
			{
				pushBlend(BLEND_ALPHA);
				hqBegin(HQ_FILLED_CIRCLES);
				{
					for (auto & c : circles)
					{
						const float radius = c.timer * c.timerRcp * 12.f + 3.f;
						
						setColor(colorBlack);
						hqFillCircle(c.x, c.y, radius);
						
						c.tick(dt);
					}
				}
				hqEnd();
				popBlend();
			}
			
			// capture the dots image into cpu accessible memory
			// note : this is slow unless the operation is delayed by 1 or 2 frames,
			//        but for testing purposes we don't care so much about performance
			
			tr1 = g_TimerRT.TimeUS_get();
			
		#if USE_READPIXELS_OPTIMIZE
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffers[pixelBufferIndex]);
			//logDebug("readPixels %d", pixelBufferIndex);
			glReadPixels(0, 0, sx, sy, GL_RED, GL_UNSIGNED_BYTE, 0);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			checkErrorGL();
			
		#if USE_READPIXELS_FENCES
			Assert(pixelSyncs[pixelBufferIndex] == 0);
			pixelSyncs[pixelBufferIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			checkErrorGL();
		#endif
			
			pixelBufferIndex = (pixelBufferIndex + 1) % kNumPixelBuffers;
		#else
			glReadPixels(0, 0, sx, sy, GL_RED, GL_UNSIGNED_BYTE, surfaceData);
		#endif
			
			tr2 = g_TimerRT.TimeUS_get();
		}
		popSurface();
		
		averageTimeR = ((tr2 - tr1) * 1 + averageTimeR * 49) / 50;
		
	#if USE_READPIXELS_OPTIMIZE
		if (pixelBufferIndex == 0)
			hasPixels = true;
		
		if (hasPixels)
	#endif
		{
			const uint64_t tm1 = g_TimerRT.TimeUS_get();
			
		#if USE_READPIXELS_OPTIMIZE
		#if USE_READPIXELS_FENCES
			Assert(pixelSyncs[pixelBufferIndex] != 0);
			const GLenum syncState = glClientWaitSync(pixelSyncs[pixelBufferIndex], GL_SYNC_FLUSH_COMMANDS_BIT, 0);
			checkErrorGL();
			
			Assert(syncState != GL_TIMEOUT_EXPIRED);
			if (syncState != GL_TIMEOUT_EXPIRED)
			{
				Assert(syncState == GL_ALREADY_SIGNALED || syncState == GL_CONDITION_SATISFIED);
				pixelSyncs[pixelBufferIndex] = 0;
			}
			else
			{
				const GLenum syncState = glClientWaitSync(pixelSyncs[pixelBufferIndex], GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
				checkErrorGL();
				
				Assert(syncState == GL_ALREADY_SIGNALED || syncState == GL_CONDITION_SATISFIED);
				pixelSyncs[pixelBufferIndex] = 0;
			}
		#endif
		
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffers[pixelBufferIndex]);
			//logDebug("mapBuffer %d", pixelBufferIndex);
			const uint8_t * __restrict surfaceData = (uint8_t*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, sx * sy, GL_MAP_READ_BIT);
			checkErrorGL();
		#endif
			
			// creste treshold mask
			
			const uint8_t * __restrict surfaceDataItr = surfaceData;
			      uint8_t * __restrict maskedDataItr = maskedData;
			
			if (tresholdFunction == 0)
			{
				const uint8_t treshold = 255 - tresholdValue;
				
				const int numPixels = sx * sy;
				
			#if USE_SSE2
				const __m128i tresholdVec = _mm_set1_epi8(treshold);
				
				const int numPixels16 = numPixels >> 4;
				
				for (int i = 0; i < numPixels16; ++i)
				{
					const __m128i surfaceVec = _mm_loadu_si128((__m128i*)surfaceDataItr);
					const __m128i maskVec = _mm_cmpge_epu8(surfaceVec, tresholdVec);
					
					_mm_storeu_si128((__m128i*)maskedDataItr, maskVec);
					
					surfaceDataItr += 16;
					maskedDataItr += 16;
				}
				
				for (int i = numPixels16 << 4; i < numPixels; ++i)
				{
					const uint8_t value = *surfaceDataItr++;
					
					*maskedDataItr++ = value >= treshold ? 1 : 0;
				}
			#else
				for (int i = 0; i < numPixels; ++i)
				{
					const uint8_t value = *surfaceDataItr++;
					
					*maskedDataItr++ = value >= treshold ? 1 : 0;
				}
			#endif
			}
			else
			{
				const uint8_t treshold = tresholdValue;
				
				const int numPixels = sx * sy;
				
			#if USE_SSE2
				const __m128i tresholdVec = _mm_set1_epi8(treshold);
				
				const int numPixels16 = numPixels >> 4;
				
				for (int i = 0; i < numPixels16; ++i)
				{
					const __m128i surfaceVec = _mm_loadu_si128((__m128i*)surfaceDataItr);
					const __m128i maskVec = _mm_cmple_epu8(surfaceVec, tresholdVec);
					
					_mm_storeu_si128((__m128i*)maskedDataItr, maskVec);
					
					surfaceDataItr += 16;
					maskedDataItr += 16;
				}
				
				for (int i = numPixels16 << 4; i < numPixels; ++i)
				{
					const uint8_t value = *surfaceDataItr++;
					
					*maskedDataItr++ = value <= treshold ? 1 : 0;
				}
			#else
				for (int i = 0; i < numPixels; ++i)
				{
					const uint8_t value = *surfaceDataItr++;
					
					*maskedDataItr++ = value <= treshold ? 1 : 0;
				}
			#endif
			}
			
		#if USE_READPIXELS_OPTIMIZE
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			checkErrorGL();
		#endif
			
			const uint64_t tm2 = g_TimerRT.TimeUS_get();
		
			averageTimeM = ((tm2 - tm1) * 1 + averageTimeM * 49) / 50;
		}
		
		// detect dots
		
		const int kMaxIslands = 1024;
		DotIsland islands[kMaxIslands];
		
		const int radius = mouse.isDown(BUTTON_LEFT) ? (1 + mouse.x / 30) : maxRadius;
		
		const uint64_t td1 = g_TimerRT.TimeUS_get();
		
		const int numIslands = detectDots(maskedData, sx, sy, radius, islands, kMaxIslands, useGrid);
		
		const uint64_t td2 = g_TimerRT.TimeUS_get();
		
		// update the smoothed out over time running average
		
		averageTime = ((td2 - td1) * 1 + averageTime * 49) / 50;
		
		// visualize the dot detection results!
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// draw the original pattern of moving dots
			
			pushBlend(BLEND_OPAQUE);
			setColor(colorWhite);
			gxSetTexture(surface.getTexture());
			drawRect(0, 0, sx, sy);
			gxSetTexture(0);
			popBlend();
			
			// draw detected dots
			
			hqBegin(HQ_STROKED_CIRCLES);
			{
				for (int i = 0; i < numIslands; ++i)
				{
					auto & island = islands[i];
					
					const int dx = island.maxX - island.minX;
					const int dy = island.maxY - island.minY;
					const float ds = std::sqrtf(dx * dx + dy * dy) / 2.f + 1.f;
					
					setColor(colorRed);
					hqStrokeCircle(island.x, island.y, radius, 2.f);
					
					setColor(colorGreen);
					hqStrokeCircle(island.x, island.y, ds, 2.f);
				}
			}
			hqEnd();
			
 			// draw stats and instructional text
			
			setFont("calibri.ttf");
			setColor(colorGreen);
			
			int y = 5 + sy;
			int fontSize = 12;
			int spacing = fontSize + 4;
			
			drawText(5, y, fontSize, +1, +1, "detected %d dots. process took %.02fms, average %.02fms", numIslands, (td2 - td1) / 1000.f, averageTime / 1000.f);
			y += spacing;
			drawText(5, y, fontSize, +1, +1, "opengl-read took %.02fms, treshold took %.02fms", averageTimeR / 1000.f, averageTimeM / 1000.f);
			y += spacing;
			drawText(5, y, fontSize, +1, +1, "useGrid: %d, tresholdFunction: %d, tresholdValue: %d", useGrid ? 1 : 0, tresholdFunction, tresholdValue);
			y += spacing;
			drawText(5, y, fontSize, +1, +1, "G = toggle grid. T = next treshold function. A/Z = change treshold");
			y += spacing;
			drawText(5, y, fontSize, +1, +1, "SPACE = quit test. MOUSE_LBUTTON = enable speed/radius test");
			y += spacing;
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
#if USE_READPIXELS_OPTIMIZE
	for (int i = 0; i < kNumPixelBuffers; ++i)
	{
		glDeleteBuffers(1, &pixelBuffers[i]);
		pixelBuffers[i] = 0;
		checkErrorGL();
		
	#if USE_READPIXELS_FENCES
		if (pixelSyncs[i] != 0)
		{
			const GLenum syncState = glClientWaitSync(pixelSyncs[pixelBufferIndex], GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
			Assert(syncState != GL_TIMEOUT_EXPIRED);
			checkErrorGL();
		}
	#endif
	}
#endif
	
	mp.close(true);
	
	delete[] maskedData;
	maskedData = nullptr;

	delete[] surfaceData;
	surfaceData = nullptr;
}

int main(int argc, char * argv[])
{
	//framework.waitForEvents = true;
	
	framework.enableRealTimeEditing = true;
	
	//framework.minification = 2;
	
	framework.enableDepthBuffer = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		initUi();
		
		//testDatGui();
		
		//testNanovg();
		
		//testChaosGame();
		
		//testFourier2d();
		
		//testImpulseResponseMeasurement();
		
		//testTextureAtlas();
		
		//for (int i = 0; i < 10; ++i)
		//	testDynamicTextureAtlas();
		
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
