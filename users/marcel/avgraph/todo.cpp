/*

top priority items from list below:
+ add channels visualizer
+ add node description callback
- add buffer type and channels.toGpu node
- add timeline node
+ add OSC history to node description

todo :
+ add default value to socket definitions
	+ add to XML
	- add ability to reset values to their default in UI
- add zoom in/out
	+ add basic implementation
	- improve zoom in and out behavior
		- clamp max zoom level
		- improve font rendering so it's both resolution independent and supports sub-pixel translation
	+ save/load zoom and focus position to/from XML
	+ add option to quickly reset drag and zoom values
	+ use arrow keys to navigate workspace (when no nodes are selected)
- add undo/redo support. just serialize/deserialize graph for every action?
	- note : serialize/deserialize entire graph doesn't work nicely with real-time connection
			 we will need to serialize node on remove and re-add/restore it during undo (also invoking real-time connection)
			 same for links and all other actions. we need to perform the opposite action on undo
- add ability to randomize input values
- add drag and drop support string literals
- add suggestion based purely on matching first part of string (no fuzzy string comparison)
	- order of listing should be : pure matches, fuzzy matches, history. show history once type name text box is made active
	- clear type name text box when adding node
- automatically hide UI when mouse/keyboard is inactive for a while
- improve OSC node
	# purchase and evaluate TouchOSC
	- purchase and evaluate Lemur (by Liine)
	- figure out how to best interop with this software
	- adapt OSC node to fit these products
	- have a learning function, to setup mappings from inputs to outputs
- visualize active links and show direction of data flow
- add buttons to manually trigger nodes
	- like the BANG node in max
	- add ability to trigger any input/output trigger (?)
- investigate VVVV's ability to turn everything into vectors of values and to combine lists
- double click node to perform node-specific action
	- add real-time editing callback for double click event
	- open text editor for ps/vs when double clicking fsfx node
	- open container when double clicking container node
- add sub-graph container node. to help organize complex graphs
- add mouse cursor to user interface
- make nodes use rounded rectangles
- make links use bezier curves
- add buffer type and add buffer input(s) to fsfx node ?
	- add channels.toGpu node
- add editor option to disable real-time preview
	- add time dilation effect on no input before stopping responding ?
	- add way for UI/editor to tell update loop it's animating something (camera..)
- hide node text until mouse moves close to node ? makes the screen more serene and helps optimize UI drawing
- look at Bitwig 2 for inspiration of node types
- add per-node profiling data
	+ measure time tick and draw take
	+ report node details. perhaps when hovering above it?
	- add special colouring mode of the node background ? black (zero cpu) -> red -> orange -> yellow (high cpu)
		- add editor option to show cpu/gpu cost
		- add GPU performance markers
- add real-time callback to get node description. report tick/draw time and some other stats/info
	- report texture format, memory usage
	+ report cpu image channel count, memory usage, alignment
	+ report video playback time
	+ report number of dots
	+ report analog values for xinput
	- report list of N latest events OSC send node
	- report list of N latest events OSC receive node
- automatically un-fold nodes (temporarily) when the mouse hovers over them ?
	- (temporarily) un-fold node when it is the only selected node. allows connecting sockets
	- (temporarily) un-fold hovered over node when connecting sockets
- extend channel data to 2D and possibly 3D. add sx, sy, sz in addition to just 'size', which is ALWAYS equal to sx * sy * sz

todo : nodes :
- add sample.float node
- add sample.image node. outputs r/g/b/a. specify normalized vs screen coords?
- add doValuePlotter to ui framework
- add quantize node
- investigate how to render 2D and 3D shapes
- investigate ways of composing/decomposing image data and masking
	- is it possible to create a texture sharing data with a base texture and to just change the rgba swizzling?
- add timeline node (?). trigger events based on markers on a timeline
	- add (re)start input trigger
	- can be very very useful to trigger effects
	- add time! input trigger. performs seek operation
- add MIDI node
- kinect node:
	+ don't calculate images when output sockets are not connected (?) or when real-time connection asks for the output ..
	- add player index output ?
	+ add image_cpu output for video data
	- add channels output for depth data
	- add point cloud xyz output image. or make a node which can calculate this for us, giving (optional) rgb, depth data, and an enum which controls the projection params (should be set to Kinect1 or Kinect2)
- add pitch control to oscillators ?
- add 'window' size to square oscillator
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
+ add CPU image downsample node.
	+ downscale 2x2 or 4x4. would make dot detector operate faster on large video files
	+ maybe should work with maximum size constraints and keep downscaling until met ? makes it possible to have varying sized image data incoming and have some kind of size gaurantee on the output
- add dot tracker node
- let nodes that allocate a surface push their surface as the current surface, so rendering in dep nodes happens in these surfaces ?
- add channels.toGpu node
	- convert a single channel into a buffer
	- convert all channels for GPU access
	- specify which channels. perhaps using swizzle control ?
		- perhaps add index 1, 2, 3, 4 and set them to -1 by default, except for index 1, which should be 0
		- perhaps add a string input which specifies channels ..
- add image to image_cpu node. default behaviour is to delay by a few frames
- add integral image node. expose integral as 2d channels object
	- add integral node
	- add sample integral node. add normalized sample xy option. if normalized any input can be used for sampling
		- samples a rect. has x1 y1 x2 y2 coords
		- has a filter option?
		- has a normalized coords option
		- has an option to fix coords so it always specified min/max for box or not ?

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
- add MSDF font rendering support

todo : media player
+ add option to disable texture generation
+ add image_cpu to image (gpu) node. default behaviour is to upload immediately
- add openAsync call which accepts OpenParams
- add yuvToRgb node
	+ add node and shader
	+ let user select colour space
	- verify color spaces. check what avcodec does, QuickTime player, etc .. there's many ways to go from yuv -> rgb !

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
+ touch zoom on moving fingers treshold distance apart. also, try to convert normalized touch coords into inches or cms



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

todo : framework :
+ optimize text rendering. use a dynamic texture atlas instead of one separate texture for each glyph. drawText should only emit a single draw call

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

todo : UI
+ add drop down list for (large) enums
+ add load/save notifications to UI., maybe a UI message that briefly appears on the bottom. white text on dark background ?

*/
