# README #

Hi! This is the origin of the Framework Creative Coding Library.

In addition to the creative coding library itself, the code available here also includes a project called 'AV Graph'. AV Graph is a node based editor similar to VVVV and TouchDesigner. I'm developing AV Graph as a means to explore the possibilities and ingredients necessary to create a highly creative coding environment.

See audioGraph and vfxGraph for backend implementations for this graph system. audioGraph is tailored towards audio synthesis, while vfxGraph is oriented towards video synthesis and controlling other systems.

Also included is a project called '4D World'. 4D World is an application built using the graph editor included in AV Graph. 4D World was used during my residency at 4DSOUND as a sound synthesis and performance tool. See http://centuryofthecat.nl/4dsound/ for some media and documentation of the project.

### Framework; A C++ Creative coding Library ###

* Framework is a creative coding library being developed in C++.
* It includes support for drawing images and playing sounds with one line of code, facilitating very rapid prototyping during creative code jams or workshops.
* It also includes easy access to keyboard, mouse and up to four gamepads.
* Shader programming is easy using the Shader class.
* All resources (images, sounds, shaders and more) can be tracked in real-time and re-loaded when changed.
* Framework is used for the higher level AV Graph node editor and UI system.

### AV Graph; Node based coding environment ###

* AV Graph is a visual node based editor, adopting the data-flow paradigm to programming.
* AV Graph makes it easy to add new node types by either editing the included types.xml or defining them in code and adding the C++ implementation of the node.
* The environment allows one to inspect and preview any input or output socket. Persistent socket value visualizers can be added by right clicking on a socket.
* Graph changes are applied in real-time and previewed behind the editor, providing immediate visual feedback of the work you're doing.

The graph editor included in AV Graph is intended as a general-purpose, re-usable editor for various types of backends. Right now there's a visuals synthesis backend included in AV Graph, and a sound synthesis backend in 4D World. But it's not restricted to visuals or sound synthesis. Possible new backends include animation blending, state machines and shader and material systems.

### (Vfx Graph) How can I check it out? ###

* The current build has been tested on MacOS using XCode and on Windows using Visual Studio 2013.
* Get a recent version of CMake. Version 3.8 or up is recommended.
* Point CMake to 'vfxGraph-examples'. Select an output folder to generate the project files.
* Open the project file in XCode or Visual Studio.
* To run Vfx Graph, make sure to build 'examples' in your environment and make it your run target.
* The working directory MUST be set to 'vfxGraph-examples/data', otherwise it will not find the assets used by Vfx Graph, and you'll end up with a mostly empty screen due to missing fonts and images.

### 4D World (Audio Graph); Sound synthesis and performance demo ###

* 4D World is a sound synthesis and performance tool, using the graph editor included with AV Graph to modularly design sounds and effects.
* 4D World includes an audio synthesis backend which makes it easy to experiment with sound design real-time. It extends real-time editing by propagating changes to all instances of a graph.
* This demo app shows how to create multiple instances from one graph. Each bird and machine uses its own instance for its sound synthesis.

### (4D World) How can I check it out? ###

* The current build has been tested on MacOS using XCode and on Windows using Visual Studio 2013.
* Get a recent version of CMake. Version 3.8 or up is recommended.
* Point CMake to 'audioGraph-examples'. Select an output folder to generate the project files.
* Open the project file in XCode or Visual Studio.
* To run 4D World, make sure to build '4dworld' in your environment and make it your run target.
* The working directory MUST be set to 'audioGraph-examples/4dworld/data', otherwise it will not find the assets used by 4D World, and you'll end up with a mostly empty screen due to missing fonts and images.

### Contacting the author ###

* For questions or suggestions, e-mail me at marcel303 [at] gmail.com or reach out to me on Facebook.
