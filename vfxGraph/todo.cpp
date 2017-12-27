/*

top priority items:
# add buttons to manually trigger nodes
	# like the BANG node in max
	# add ability to trigger any input/output trigger (?)
	# decided not to do this, as UI is better left uncoupled from graph processing (for now). perhaps a separate UI layer on top of vfx- and audio graph makes more sense
- add GPU performance markers
	+ add GPU timer object
	+ add GPU time to vfx node base
	- fix issue with recursive GPU timers
- add text field to node type select
- add a list of xml files to click on for more rapid testing
- add a dedicated shader node. no pre-defined inputs at all. output an image
- rename fsfx v1 to imfx ? redefine behavior to run shader over an image. expressly output as image
# how to convert multiple channels to gpu image with channel(s).toGpu ?
	# removed support for multiple channels for one output socket. always using a single channel resolves this issue
- add ability for nodes to report warnings and errors
	# renamed to gen.primitive
- add gen.random node with different modes. white, pink, brown
- add gen.brownian
+ remove visualizers for nodes which are removed?
+ add polyphonic audio graph node
+ add channel.curve node. give it a min, max, power factor and channel size params. perhaps even 2d
- add multiple window support to framework. would help with managing lists of audio and vfx graphs. just open a window and show the lists over there
- fix issue with immediate values not being restored properly for most types
+ make sure inputs are update to date (last tick ID) when resizing dynamic inputs ?
- raise a menu when socket connect is released on a node itself. ask for which socket to connect to
- make it possible to have voice nodes in audio graph which do not generate audible sound ?
- add an option to audio graph and poly audio graph nodes to output audio or not
- add options to poly and regular audio graph nodes to output mono, stereo, or multi-channel
	- requires an extra mixing level ? perhaps add a voice manager interface. or store voices in audio graph ? let voice manager allocate channel indices, but do mixing itself differently ? or perhaps add voice groups or something ..
+ add channel.fft node


melody = individual, harmony = community, rithm = time, tonal color = mood
ingradients
line input
midi notes
video feed
visual circles.. animated grooop logo
visual score
ideom
metronome.. dissolves into visuals leading
videos
- datum: 21 december. laken, beamer, verloopstukje

todo :
- add undo/redo support. just serialize/deserialize graph for every action?
	- note : serialize/deserialize entire graph doesn't work nicely with real-time connection
			 we will need to serialize node on remove and re-add/restore it during undo (also invoking real-time connection)
			 same for links and all other actions. we need to perform the opposite action on undo
- add ability to randomize input values
- add suggestion based purely on matching first part of string (no fuzzy string comparison)
	- order of listing should be : pure matches, fuzzy matches, history. show history once type name text box is made active
	- clear type name text box when adding node
- visualize direction of link data flow
- investigate VVVV's ability to turn everything into vectors of values and to combine lists
	# add channels combine method (for now?)
		+ combine idea is pretty much covered by merge node
	- add node where channel values can be added to a list -> allow to experiment with combine node
 	- add automatic combine behavior when connecting multiple channels to the same input ?
 		- default to cycle. add link map to override behavior ? (clamp, pad zero)
- double click node to perform node-specific action
	# add real-time editing callback for double click event
		+ add an editor callback to the node type definition instead. the graph editor should be fully functional without real-time editing interface and an implementation running in the backgroup, meaning resource editing should work regardless of real-time interface
	- open text editor for ps/vs when double clicking fsfx node
	- open sub graph for container nodes
- add sub-graph container node. to help organize complex graphs
	- open container when double clicking container node
	- determine how to save nodes hierarchically contained
	- the inputs and outputs of the container node and how it all works should all be implementation-defined I think
- make links use bezier curves
- hide node text until mouse moves close to node ? makes the screen more serene and helps optimize UI drawing
- look at Bitwig 2 for inspiration of node types
- improve node auto un-folding and socket connect behaviors
	- (temporarily) un-fold node when it is the only selected node. allows connecting sockets
	- (temporarily) un-fold hovered over node when connecting sockets. currently based on distance, but should also include hit test
	- move hovered over node in front of others?
- NanoVG includes an interesting blurring algortihm used for blurring fonts. integrate ?
- add ability to add node between nodes ?
- add third 'node minification' option: show only active inputs and outputs
	so we have three options then: show everything, show only active i/o and fully collapsed
- add ability to reference nodes? makes graph organization more easy
- drag link into empty space = open node type selection menu


todo : creativity investigation :
- add scratch mode. scratch over nodes and trigger real-time connection events
- add string mode. drag mouse over screen, let node links react as if they were being pulled. add real-time connection callback for the string location. let links/strings vibrate for a while after being pulled
- add fire mode. slowly 'deteriorate' node input values over time using a random walk. add a real-time connection callback to tell the system a node is burning


todo : nodes :
- add sample.float node
	- output value as float
	- output value as channel
- add sample.image node
	+ output rgba as floats
	+ output rgba as channels
	- specify normalized vs screen coords?
	+ add filter and clamp options
- add sample.channel node
	- filter is enum: auto, linear or off. auto will look at continuous flag of channels input
- add doValuePlotter to ui framework
- add quantize node
- investigate how to render 2D and 3D shapes
	+ add surface node. push surface before tick, pop surface after tick
	- add sequencer node. has multiple any type inputs. inputs are processed in socket order
- investigate ways of composing/decomposing image data and masking
	- is it possible to create a texture sharing data with a base texture and to just change the rgba swizzling?
- timeline node updates:
	- add (re)start input trigger
	+ can be very very useful to trigger effects
	- add time! input trigger. performs seek operation
+ add MIDI node
- add pitch control to oscillators ?
- add pulse size to square oscillator
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
- add note (like C1) to MIDI note
- add adsr node
- perhaps add string names to channels, for more convenient selection ? would reduce remixing capability I fear .. maybe let nodes which produce channels to document in their node description what those channels represent, semantically .. but not let the user use those semantics to select channels
- add image to image_cpu node. default behaviour is to delay by a few frames
- add integral image node. expose integral as 2d channels object
	- add integral node
	- add sample integral node. add normalized sample xy option. if normalized any input can be used for sampling
		- samples a rect. has x1 y1 x2 y2 coords
		- has a filter option?
		- has a normalized coords option
		- has an option to fix coords so it always specified min/max for box or not ?
- add jpeg glitch node. in combination with capturing from image would be awesome!
- add yuvToRgb node
	+ add node and shader
	+ let user select colour space
	- verify color spaces. check what avcodec does, QuickTime player, etc .. there's many ways to go from yuv -> rgb !
- add depth data treshold node. actually this is more like a general purpose channel value treshold node ..
	- select which channel contains depth
	- remove items from channels where depth at index fails test
		- allocate new channels object to store results
	- add range min/max range + pass if inside or outside boolean
- add memory node ? get/set named variables. how to ensure processing order ? or maybe for vfx <-> c++ communication only in which case order is guaranteed
- add queue system for triggers ? ensure predeps have finished processing before handling triggers
- add ability for nodes to trigger again (process a partial time slice ?). but this will re-introduce again the issue of execution order of triggers ..
- add channels.toImage node ?
- add play/stop/pause/resume triggers to video node
- add curl node
- add text list node. next! prev! rand! custom editor for list of strings
- add audioGraph node


todo : UI
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

todo : documentation
- write about design philosophy

todo : framework
- add scoped** objects ? or prefer explicit push/pop ?
- add drawCube and drawSphere?
- add hq shaded triangle to framework
- rewrite audio to use port audio instead of OpenAL
	- rewrite sound effects
	- rewrite music
	- add API for custom sound processing and voices
- check ShaderUtil.txt. add functions


--- ARCHIVED TODOS ---



todo :
+ replace surface type inputs and outputs to image type
+ add VfxImageBase type. let VfxPlug use this type for image type inputs and outputs. has virtual getTexture method
+ add VfxImage_Surface type. let VfxNodeFsfx use this type
+ add VfxPicture type. type name = 'picture'
+ add VfxImage_Texture type. let VfxPicture use this type
+ add VfxVideo type. type name = 'video'
+ add editorValue to node inputs and outputs. let get*** methods use this value when plug is not connected
+ let graph editor set editorValue for nodes. only when editor is set on type definition
+ add socket connection selection. remove connection on BACKSPACE
+ add multiple node selection
# on typing 0..9 let node value editor erase editorValue and begin typing. requires state transition? end editing on ENTER or when selecting another entity
# add ability to increment and decrement editorValue. use mouse Y movement or scroll wheel (?)
+ remember number of digits entered after '.' when editing editorValue. use this information when incrementing/decrementing values
+ add sine, saw, triangle and square oscillators
+ save/load link ids
+ save/load next alloc ids for nodes and links
+ free literal values on graph free
+ recreate DatGui when loading graph / current node gets freed
# prioritize input between DatGui and graph editor. do hit test on DatGui
+ add 'color' type name
+ implement OSC node
+ implement Leap Motion node
+ UI element focus: graph editor vs property editor
+ add ability to collapse nodes, so they take up less space
	+ SPACE to toggle
	+ fix hit test
	+ fix link end point locations
+ passthrough toggle on selection: check if all passthrough. yes? disable passthrough, else enable
+ add socket output value editing, for node types that define it on their outputs. required for literals
+ add enum value types. use combo box to select values
	+ define enums for ease node type
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
+ integrate with UI from libparticle. it supports enums, better color picking, incrementing values up and down in checkboxes
+ add mouse up/down movement support to increment/decrement values of int/float text boxes
+ add option to specify (in UiState) how far text boxes indent their text fields
+ add history of last nodes added
+ insert node on pressing enter in the node type name box
	+ or when pressing one of the suggestion buttons
+ remove 'editor' code
+ allocate literal values for unconnected plugs when live-editing change comes in for input
+ show which nodes and links are actively traversed. add live-connection callback to query activity
	+ add support for one-shot activity
	+ add support for continuous activity
+ show min/max on valueplotter
+ add editor options menu
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
+ make time node use local vfx graph instance time, not process time
+ allow trigger inputs to have multiple incoming connections
+ add OpenGL texture routines/helper object. seems we are doing a lot of duplicate/messy/easy to fuck up texture management in various nodes and other places
+ always to dragAndZoom tick ? regard of state
+ fix node dragging as dragAndZoom updates the viewport
	+ remember where (in node space) the mouse went down
	+ calculate new position based on current mouse position in graph space and initial position in node space
+ add visualizer for multi-channel data
+ add default value to socket definitions
	+ add to XML
	+ add ability to reset values to their default in UI
+ automatically hide UI when mouse/keyboard is inactive for a while
+ add real-time callback to get node description. report tick/draw time and some other stats/info
	+ report cpu image channel count, memory usage, alignment
	+ report video playback time
	+ report number of dots
	+ report analog values for xinput
	+ report list of N latest events OSC send node
	+ report list of N latest events OSC receive node
+ extend channel data to 2D and possibly 3D. add sx, sy, sz in addition to just 'size', which is ALWAYS equal to sx * sy * sz
+ add real-time callback for when a socket value is reset back to its default. allow VFX implementation to clean up automatically allocated literals and to disconnect them. otherwise isConnected() for inputs keeps returning true, which messes up the functioning of some nodes
+ reset socket values that are edited by a text box when text box editing ends with or is an empty value
+ add Z-index to nodes. right now the draw order is fixed
	+ use a continously incrementing counter (akin to node/link alloc id) and assign to nodes upon select ?
	+ apply/increment counter upon SINGLE node selection
	+ upon draw, add nodes to an array, sort, draw
+ add zoom in/out
	+ add basic implementation
	+ improve zoom in and out behavior
		# clamp max zoom level -> actually this is not needed and the negative zoom adds a nice effect and possible source for inspiration
		+ improve font rendering so it's both resolution independent and supports sub-pixel translation
	+ save/load zoom and focus position to/from XML
	+ add option to quickly reset drag and zoom values
	+ use arrow keys to navigate workspace (when no nodes are selected)
+ add per-node profiling data
	+ measure time tick and draw take
	+ report node details. perhaps when hovering above it?
	+ add special colouring mode of the node background ? black (zero cpu) -> red -> orange -> yellow (high cpu)
		+ add editor option to show cpu/gpu cost
	+ add color curve to editor options to use for coloring nodes. from 0 .. 33ms ?
+ render graph edit UI into a separate surface. use fade effect when the UI is being hidden
+ add scroll wheel support to zoom in or out
+ fix socket link mapping parameters UI not refreshing when selecting another link
# add drag and drop support string literals
+ make nodes use rounded rectangles
# add buffer type and add buffer input(s) to fsfx node ?
	-> I'm trying to use mostly textures to improve remixability
+ add ability to route connections
	+ add route point editing
	+ save/load route points
+ report OpenGL texture format and memory usage in node getDescription
+ add ability to store resource data in nodes, so nodes can persist their own data when editing
+ add channels visualizer
+ add ability to add multiple connections to input sockets ? similar to 4dworld ?
+ add ability to specify in/out mapping on a per-link basis. make it operate similar to how adding multiply connections to input sockets works in 4dworld
	+ have a single floating point connection (float pointer). this is how things currently work
	+ have multiply floating point connections. store float pointers in array. allocate storage for calculating sum. make update() method to update summed value
	+ have optional remapping enabled per link. combine this with summing support. store mapping and float pointer in summed value element
+ add non-SSE versions for audio code
+ add new node type selection memu. make it a pop-over ?
+ port compose shader from 4dworld to avgraph. fix graph editor fade when idle
+ add editor option to disable real-time preview
	+ add time dilation effect on no input before stopping responding ?
	+ add way for UI/editor to tell update loop it's animating something (camera..)
+ add node editors. when double clicking a node (or some other gesture/interaction), show the node editor. let the node editor operate on the node's data.
	+ let the node editor be independent of the implementation ? it should be possible to have a fully functional graph, graph node and resource editing environment without a live version of the graph running in the background. this means the saveBegin real-time editing callback should be removed again once we got this working
 + fix issue with unable to close node type selection menu without selecting a node
+ remove registration instance name from VFX_NODE_TYPE
+ add passthrough mode sequence node
+ add custom draw surface node to enable better passthrough behavior
+ add support for showing multiple notifications
+ search node types based on display name. until a dot is included. switch to full type name when a period is present
+ push a dummy surface when drawing nodes not connected to the display node
+ report shader error log (FSFX, FSFX v2 and shader nodes)
+ add 'float' boolean to surface node. default.. true ?
	+ used a format option instead
+ add draw.text class
+ add the ability to have dynamic input sockets. use cases: shaders, sub graphs
	+ add support for recovering connections
	+ add support for recovering immediate values
	+ add dynamic stuff to VfxGraph so as not to bother each and every node with having to store dynamic links, immediate values, etc ?
+ add the ability to have dynamic output sockets. use cases: sub graphs
	+ add support for recovering connections
# add explicit categories to vfx nodes
	# decided I don't like it. use first part of node type name to decide category
	# use last part of node type name as the display name to keep the graph editor UI clean
# add FSFX shaders. use built-in shaders ?
	+ copied gaussian shadr to fsfx/ folder
 + add FSFX shaderSource which exposes main function and uses applyFsfx function from .fsfx file to change pixels. also exposes shared uniforms, applies alpha blended, opacity control, color mode and color post
+ fix issue with link mappings not being restored updating dynamic sockets
+ fix SDL text input when selecting a text field before the previously active one (start before stop). todo : test IME support
+ add mouse cursor to user interface
+ fix issue where freeing texture here can result in issues when drawing visualizer. tick of visualizer should always happen after cpuToGpu tick, but there's no link connecting visualizer to the node it references, so tick order is undefined .. ! -> maybe update visualizer in draw. or never capture references (to textures ID's or whatever) in visualizer. only let it copy values by value (as needed for graph) but capture everything else on draw
	+ add a separate tickVisualizers(..) or similar ? it would run after evaluating the vfx graph, so get the latest values. conceptually, doing the regular tick before the vfx graph also makes sense, as it ensures vfx graph uses the latest version of the graph. so editor.tick, vfxGraph.tick/draw, editor.tickVisualizers. editor.draw, composite vfxGraph/editor
+ add copy and paste text support
+ create separate avGraph and vfxGraph projects with CMake files
+ decouple visualizers from nodes. when hit testing, sort elems by z-key, add ptr to node or visualizer
+ add automated error checking test for existing graph files ?


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
+ add impulse response node. measure input impulse response with oscilator at given frequency
+ add sample and hold node. has trigger for input
+ add simplex noise node
+ add binary counter node, outputting 4-8 bit values (1.f or 0.f)
+ add delay node. 4 inputs for delay. take max for delay buffer. delay buffer filled at say fixed 120 hz. 4 outputs delayed values
+ add gamepad node
+ add restart signal to oscillators ? if input > 0, reset phase
+ add spring node ? does physical simulation of a spring
+ add node which sends a trigger when a value changes. send new value as trigger data
+ add node which sends a trigger when a value crosses a treshold
+ add pitch and semitone nodes
+ video: add loop input
+ video: add playback speed input
+ add FFT analyser node. output image with amplitude per band
	+ output is image. input = ?
+ add base event ID to OSC send node ?
+ add random noise node with update frequency. updates random value N times per second
+ add touch pad node which reads data from the MacBook's touch pad (up to ten fingers..)
+ add node which can analyze images, detect the dots in them, and send the dots as output
	+ add dot detection node
	+ will need a vector socket value type ?
+ add spectrum2d node
+ add CPU image downsample node.
	+ downscale 2x2 or 4x4. would make dot detector operate faster on large video files
	+ maybe should work with maximum size constraints and keep downscaling until met ? makes it possible to have varying sized image data incoming and have some kind of size gaurantee on the output
+ add channels.toGpu node
	+ convert a single channel into a buffer
	# convert all channels for GPU access
		-> this doesn't map well. always convert a single (1D or 2D) channel for now
	# specify which channels. perhaps using swizzle control ?
		# perhaps add index 1, 2, 3, 4 and set them to -1 by default, except for index 1, which should be 0
			-> this has been replaced with a channel select node for now
+ add channel select node. selects one or a range of channels from a channels object. specify channel (default=0) and numChannels (default=1)
+ add channel slice node. make a new channels object from a 2D channels object by selecting only between y (default=0) and numSlices (default=1)
+ kinect node:
	+ don't calculate images when output sockets are not connected (?) or when real-time connection asks for the output ..
	# add player index output ?
	+ add image_cpu output for video data
	+ add channels output for depth data
	+ add point cloud xyz output image. or make a node which can calculate this for us, giving (optional) rgb, depth data, and an enum which controls the projection params (should be set to Kinect1 or Kinect2)
+ add dot tracker node
+ let nodes that allocate a surface push their surface as the current surface, so rendering in dep nodes happens in these surfaces ?
	+ add surface node
+ add channel swizzle node. allow it to reorder one or more channels into a new channels object
+ add channel combiner node. allow multiple input channels to be merged into one
+ add CPU image delay node
	+ use a list of images as history
	+ max history size is set as input. re-allocate history on change
	+ current history delay is set a input [0..1]. default=1
	+ store to jpeg optionally to save memory
	+ make jpeg compression optional
+ add color node. from RGB or HSV (select mode)
+ change math node so operation type becomes a configurable enum
+ add integration node. keeps integrating input value over time and sets it as output
+ add timeline node
+ add OSC history to node description
 + add data table node. read data from CSV, text or XML file
+ remove trigger data
+ add draw.image node. let the user control sizing (similar to object-fit in html)
+ add draw.blend node
+ determine how OSC send and receive nodes should function
	+ would very much like an option to 'learn' OSC event paths. perhaps double click to open custom editor and click 'learn' to let it receive a message and capture to path
	+ decouple OSC endpoints from OSC values and paths? perhaps add an OSC endpoint node which receive messages, add them to a list, let OSC value nodes iterate. add find method to OSC manager. if tick ID != last tick ID, clear old messages, receive new messages, find and return value (if any)
 + improve OSC node
	# purchase and evaluate TouchOSC
	+ purchase and evaluate Lemur (by Liine)
	+ figure out how to best interop with this software
	+ adapt OSC node to fit these products
	+ have a learning function, to setup mappings from inputs to outputs
+ visualize active links
+ add read-only mode. add flags for controlling editor behaviors
+ add image.toChannels node
+ add Wekinator node
+ add passthrough support OSC send and receive nodes
# refactor math node so it's only one node with a subtype selection input
+ add OSC node with a list of paths. output received values as channels
+ add vfxGraph node
+ show src socket value preview when hovering over a link
+ reduce hit size links
+ add FSFX node which lets one select a shader from a drop down list
+ add draw primitive node
	+ xyz channels input
	+ add size literal input (to be used when 3rd channel is missing
	+ screen size input (if true, scaling doesn't affect circle size)
	+ color. use white when missing
+ have the model node also expose channels ?
+ add gen.osc. lt user select shape. take inspiration from audio node version
+ oscilloscope node


todo : fsfx :
+ let FSFX use fsfx.vs vertex shader. don't require effects to have their own vertex shader
+ expose uniforms/inputs from FSFX pixel shader
 iterate FSFX pixel shaders and generate type definitions based on FSFX name and exposed uniforms
	+ OR: allow nodes to specify dynamic input sockets. use real-time callback to get the list of inputs
	+ store inputs by name in nodes (like regular inputs)
	+ let FSFX node  resize its inputs dynamically (?)
	+ match the dynamic sockets by name ? add to VfxNodeBase to try to get socket based on name if index lookup fails ?
+ add standard include file (shaderSource(..)) for FSFX nodes. include params, time, texture1 and 2 and maybe some common functions too


todo : framework :
+ optimize text rendering. use a dynamic texture atlas instead of one separate texture for each glyph. drawText should only emit a single draw call
+ add MSDF font rendering support
+ add ability to save MSDF texture atlas and load/supplement it
+ added HQ rounded rect method
+ add method to push/pop MSDF font rendering bit
+ add easy 3D perspective camera with manual control over input. make it easy to set matrices
+ add drawLine3d, drawRect3d and drawGrid3d
+ add a simple camera class. pushMatrix, popMatrix
+ add a generic way to shade and texture hq primitives. perhaps use texture and shading matrices ?
+ remove stage and UI classes

todo : media player
+ for image analysis we often only need luminance. make it an option to output YUV Y-channel only?
	+ outputting Y+UV is just as cheap as Y only. added planar YUV support.
+ add image_cpu value type ?
	+ extend video node so it can output Y/UV image_cpu data
	+ extend video node so it can output RGB image_cpu data
		# requires rewriting media player a little, so consume (acquire) and release of frame data is possible
		+ add Y and UV pointers to MP::VideoFrame
+ add image_cpu to image (gpu) node. default behaviour is to upload immediately
+ add image_y, image_u, image_v to video node
+ double check image.toGpu node uses optimized code path for converting single channel source to texture
+ add option to disable texture generation
+ add image_cpu to image (gpu) node. default behaviour is to upload immediately
+ add openAsync call which accepts OpenParams

todo : UI
+ add drop down list for (large) enums
+ add load/save notifications to UI., maybe a UI message that briefly appears on the bottom. white text on dark background ?
+ touch zoom on moving fingers treshold distance apart. also, try to convert normalized touch coords into inches or cms
+ fix issue with shift + <char> not resulting in desired character in text fields

*/
