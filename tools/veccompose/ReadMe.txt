----------------------------------------
VECCOMPOSE
----------------------------------------

Table of Contents:

- Features
- Limitations
- Settings
- Workflow
- Managing Shapes
- Adding Links
- Removing Links
- Editing Properties
- Previewing Your Work
- Saving Your Work
- Hand-Editing Vector Compositions

----------------------------------------
Features
----------------------------------------

- Add and edit vector compositions and shape libraries.
- Load / save your work.

----------------------------------------
Limitations
----------------------------------------

There is no built in preview application.

There is no MRU. [todo]

----------------------------------------
Settings
----------------------------------------

There are no application settings.

----------------------------------------
Workflow
----------------------------------------

VecCompose is an application to create compositions
from individual vector shapes. These compositions
consist of interconnected links creating a hierarchy
of shapes.

Generally, the work flow looks as follows:

- The artist creates a design (on paper or using a drawing app)
  of a vector composition.
- Individual elements of the design are crafted using
  VecDraw. Result: vector graphics of each individual shape.
- The artist creates a new composition using VecCompose.
- The artist adds his/her vector shapes to the shape library.
- The artist creates a hierarchy of links and assigns shapes
  from the shape library to these links.

Finally:
- The work is saved and compiled during the build process.

----------------------------------------
Managing Shapes
----------------------------------------

- Select 'Shapes' from the tab control on the right.
- Right click -> 'Add..'.
- Enter the shape's details.
	NOTE: Minimally, the path and name must be set.
	      All other options are optional (and not generally recommended).

During your workflow, you may decide to modify vector shapes.
- To reload the vector shapes used by VecCompose, use
  the 'Refresh Shapes' option from the toolbar.

----------------------------------------
Adding Links
----------------------------------------

- Select the link which will act as parent to the new link.
- Use the toolbar or '<ALT> + 1' to create the new link.

----------------------------------------
Removing Links
----------------------------------------

- Select the link you wish to delete.
- Press 'delete' to remove the link.

----------------------------------------
Editing Properties
----------------------------------------

Edit links by selecting them.

The property editor will show all available options.

- Name: The name of the link. Used to construct the link hierarchy.
- Parent: The name of the parent link.
- Location: The relative location from the root link.
- Base Angle: The base rotation used to orient the link.
- Angle: The varying rotation applied to the link's shape and child links.
- Minimum Angle: Minimum angle constraint.
- Maximum Angle: Maximum angle constraint.
- Shape Name: Name of the shape in the shape library.
- Level: Level at which the link becomes active. The link is suppressed at lower levels.
- Flags: Boolean options which can be toggled ON and OFF.

----------------------------------------
Previewing Your Work
----------------------------------------

There is no preview option build in.

The editor should give a good impression of the final result.

----------------------------------------
Saving Your Work
----------------------------------------

Use the 'Save' and 'Load' menu items (ALT+S, ALT+L). Alternatively, use 'Save as..' (ALT+A).

----------------------------------------
Hand-Editing Vector Compositions
----------------------------------------

Vector composition files (.vc) and shape libries (.slib)
are stored in a plain-text format.

You may open these files using your favorite text editor to
edit them by hand.
