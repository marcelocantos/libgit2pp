CXXFLAGS += -std=c++17
LDLIBS = -lgit2
LINK.o = $(LINK.cc)

ifeq "$(ARCH)" ""
$(error missing ARCH)
endif

OUT=out/$(ARCH)

WALK = $(OUT)/walk

all: $(WALK)

test: $(WALK)
	$< HEAD

$(WALK): $(WALK).o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(WALK).o: walk.cc ../git2pp.h $(OUT)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(OUT):
	mkdir -p $@
