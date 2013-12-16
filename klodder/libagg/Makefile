LIBAGG = libagg.a

LIBAGG_SRC = \
	agg_image_filters.cpp \
	agg_trans_affine.cpp
LIBAGG_DST = $(LIBAGG_SRC:.cpp=.o)

LIBAGG : $(LIBAGG_DST)
	ar -rcs $(LIBAGG) $(LIBAGG_DST)

all : $(LIBAGG)

%.o : %.cpp
	gcc -c $< -o $@