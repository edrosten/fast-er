CXXFLAGS+=-Wall -Wextra -O3 -march=pentium-m
CXX=colorgcc
LDFLAGS=-lGVars3 -lcvd 

PROGS=learn_detector learn_detector_orig

.PHONY: all

all:$(PROGS)

learn_detector_orig:learn_detector_orig.o
	$(CXX) -o $@ $^ $(LDFLAGS) 

learn_detector:offsets.o faster_bytecode.o faster_tree.o learn_detector.o load_data.o	
	$(CXX) -o $@ $^ $(LDFLAGS) 

clean:
	rm -f $(PROGS) *.o
