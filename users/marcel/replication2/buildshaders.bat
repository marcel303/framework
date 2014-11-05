SRCS = \
	data/shaders/deferred_composite_ps.cg \
	data/shaders/deferred_composite_vs.cg \
	data/shaders/deferred_geometry_ps.cg \
	data/shaders/deferred_geometry_vs.cg \
	data/shaders/deferred_light_ps.cg \
	data/shaders/deferred_light_vs.cg

DSTS = $(SRCS:.cg=.shader)

all : $(DSTS)

.PHONY : dummy

%.shader : %.cg dummy
	../../../tools/hlslcompiler/bin/Debug/hlslcompiler -c $< -o $@