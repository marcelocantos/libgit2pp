CXXFLAGS += -std=c++17
LDLIBS = -lgit2
LINK.o = $(LINK.cc)

ifeq "$(ARCH)" ""
$(error missing ARCH)
endif

OUT=out/$(ARCH)

WALK = $(OUT)/walk

all: $(WALK)

# Not a test: there are no assertions here, and the repository it reads is
# whatever git_repository_open_ext finds above the cwd. The suite lives in
# ../test. This target just runs the example.
run: $(WALK)
	$< HEAD

$(WALK): $(WALK).o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(WALK).o: walk.cc ../git2pp.h $(OUT)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(OUT):
	mkdir -p $@
