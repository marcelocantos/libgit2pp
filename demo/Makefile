CXXFLAGS += -std=c++17
LDLIBS = -lgit2
LINK.o = $(LINK.cc)

all: walk

test: walk
	./walk master

walk:
walk.o: ../git2pp.h
