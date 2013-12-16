GGCOPY = gg-copy
GGCOPY_SRC = main.cpp
GGCOPY_DST = $(GGCOPY_SRC:.cpp=.o)

LIBGG = ../../prototypes/libgg/libgg.a
LIBGG_INCLUDE = -I../../prototypes/libgg

all : $(GGCOPY)

$(GGCOPY) : $(GGCOPY_DST) $(LIBGG)
	gcc -o $(GGCOPY) $(GGCOPY_DST) $(LIBGG) -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ -Wall $(LIBGG_INCLUDE)

