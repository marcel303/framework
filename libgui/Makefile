LIBGUI = libgui.a
LIBGUI_SRC = \
	GuiArea.cpp \
	GuiConstraintSolver.cpp \
	GuiContext.cpp \
	Gui.cpp \
	GuiDebug.cpp \
	GuiGraphicsCanvas.cpp \
	GuiGraphics.cpp \
	GuiGraphicsCursor.cpp \
	GuiGraphicsFont.cpp \
	GuiGraphicsImage.cpp \
	GuiGraphicsShape.cpp \
	GuiInterfaceList.cpp \
	GuiObject.cpp \
	GuiPoint.cpp \
	GuiProperty.cpp \
	GuiPropertyList.cpp \
	GuiRect.cpp \
	GuiSystem.cpp \
	GuiUtilActions.cpp \
	WidgetButton.cpp \
	Widget.cpp \
	WidgetImage.cpp \
	WidgetRoot.cpp
LIBGUI_DST = $(LIBGUI_SRC:.cpp=.o)

all : $(LIBGUI)

$(LIBGUI) : $(LIBGUI_DST)
	ar -rcs $(LIBGUI) $(LIBGUI_DST)

%.o : %.cpp
	gcc -c $< -o $@ -I../libgg -I../libiphone

