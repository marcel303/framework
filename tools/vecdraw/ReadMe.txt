----------------------------------------
VECDRAW
----------------------------------------

Table of Contents:

- Features
- Limitations
- Settings
- Settings: Canvas Size
- Settings: Preview Executable
- Adding Shapes
- Editing Shapes
- Editing Properties
- Duplicating Shapes
- Adding and Removing Rulers
- Shape Z-Order
- Collision Shapes
- Zooming
- Previewing Your Work
- Saving Your Work
- Hand-Editing Vector Graphics

----------------------------------------
Features
----------------------------------------

- Add and edit circle, polygon, curve and tag shapes.
- Load / save your work.
- Preview your work using the vecview utility.

----------------------------------------
Limitations
----------------------------------------

Collision shapes are only supported for the following shapes:
- Polygons
- Circles

All other shapes (curves and tags) will not create a collision
shape. Use hidden shapes to define the collision shape
should you need more control.

----------------------------------------
Settings
----------------------------------------

Canvas Size:
The canvas size is an area to indicate the size of the graphic you're creating.

Preview Executable:
When you preview a graphic, and external tool is invoked to preview the graphic. Select the preview tool here.

----------------------------------------
Adding Shapes
----------------------------------------

Use the menu bar to add shapes.

Alternatively, use ALT+<n> where <n> is the number of the shape (see menu bar).

----------------------------------------
Editing Shapes
----------------------------------------

Select shapes using the left mouse button.

Select polygon vertices using the left mouse button.

Move shapes and vertices around using the right mouse button.

Add vertices to polygons using the middle mouse button or by pressing 'I'.

Erase vertices from polygons by selecting a vertex and pressing 'E'.

Remove shapes by pressing delete. The selected shape will be removed.

----------------------------------------
Editing Properties
----------------------------------------

Edit shape or vertex properties by selecting the element
and changing its' property values through the property editor.

----------------------------------------
Duplicating Shapes
----------------------------------------

To duplicate a shape, select the shape you wish to duplicate and press 'D'.

----------------------------------------
Adding and Removing Rulers
----------------------------------------

Rulers are lines which help align graphic features. Add rulers by pressing 'R'. Remove a ruler by pressing 'R' again while hovering over an existing ruler's center.

----------------------------------------
Shape Z-Order
----------------------------------------

The Z-Order defined the order in which shapes are drawn onto the final texture.

The Z-Order defines whether a shape is on-top or below other shapes.

Adjust the Z-order of the shapes using the up and down arrows on
the right hand side. The selected shape will be moved up or down the list.

Another way to adjust the Z-Order is by using Page-Up and Page-Down from the editing view.

----------------------------------------
Collision Shapes
----------------------------------------

It's possible to create geometry optimized for collision detection purposes.

To add a collision shape, create a new shape and uncheck 'Visible' while checking 'Collision'.

The shape won't be drawn, but will participate in collision detection.

Circles are the most optimal collision shapes. Polygons with high vertex counts the least.

Another way to manage visibility and collision parameters is through the Shape List's context menu. Right click on a shape in this list to quickly modify its' settings.

----------------------------------------
Zooming
----------------------------------------

Use the zoom slider to zoom in or out.

[todo: panning]

----------------------------------------
Previewing Your Work
----------------------------------------

Press the preview button from the menu bar or use ALT+V.

If no preview tool has been selected, a dialog will be shown
prompting you to select preview app.

----------------------------------------
Saving Your Work
----------------------------------------

Use the 'Save' and 'Load' menu items (ALT+S, ALT+L). Alternatively, use 'Save as..' (ALT+A).

Use the 'Recent' menu to open a recent file.

----------------------------------------
Hand-Editing Vector Graphics
----------------------------------------

Vector graphic files (.vg) are stored in a plain-text format.

You may open these files using your favorite text editor to
edit them by hand.
