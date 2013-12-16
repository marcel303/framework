GGCOMPARE = gg-compare
GGCOMPARE_SRC = main.cpp
GGCOMPARE_DST = $(GGCOMPARE_SRC:.cpp=.o)

LIBGG = ../../prototypes/libgg/libgg.a
LIBGG_INCLUDE = -I../../prototypes/libgg

all : $(GGCOMPARE)

$(GGCOMPARE) : $(GGCOMPARE_DST) $(LIBGG)
	gcc -o $(GGCOMPARE) $(GGCOMPARE_DST) $(LIBGG) -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ -Wall $(LIBGG_INCLUDE)

