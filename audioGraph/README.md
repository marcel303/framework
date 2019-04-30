# libaudiograph
## Open source node based sound synthesis
`libaudiograph` is an open source source node based sound synthesis library.

Core features at a glance,

- Embeddable,
	- Use audio graphs where you see fit, inside your own c++ applications,
	- With or without optional (real-time) editing support,
	- With the audio api of your liking.
- Extendable,
	- Completely open source,
	- Easily create your own nodes, or hack away at existing nodes,
	- Combine `libaudiograph` with other libraries such as Box2D to create physisically triggered effects,
	- Optimized compile times for fast iteration.
- Optimized for performance,
	- Optimized hybrid scalar and vector 'audio floats',
	- SIMD optimized audioBuffer functions, used for mixing and working with 'audio floats'.
- Real-time editing,
	- Uses avgraph as the underlying graph representation,
	- Implements a real-time editing interface.

## Design philosophy
Node based editing is great for experimentation,
for setting up signal flow,
for experimenting without thinking so much about structure, where the creative process takes you to exploring a possibility space. Especially when combined with tools such as visual interface elements like dials and sliders which allow for quickly moving about.

Recognizing it's strengths one must also consider it's downsides. Popular node based systems come with two major downsides.

1. They are closed source. Extending them is difficult. Coding is not part of the day to day process,
2. They place the graph center stage. Sometimes it makes sense to think about coding as setting up a structure. Forcing one to do this through the node based system leads to a less than ideal workflow for people who know about coding.

Graphs are not great for setting up structure. Programming languages such as C++ are more suited for this.

The main philosophy behind the avgraph project is to combine languages and use a language where it is strong. This requires one to build bridges where necessary to let these worlds communicate. This bridge could be an interface made using control values, where parameters sets are used as a control interface from one realm to the other. It could also be a custom node inside the graph system which communicates directly with an underlying system in C++ and speaks a different as part of the graph.

## Embeddable

## Extendable

## Examples of extendability
### Spatial sound applications
### Physics driving sound excitation
### Combining libaudiograph with other systems

## Examples