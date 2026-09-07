# Hermetic test suite for git2pp.h.
#
# `make test` builds a throwaway fixture repository in a temp directory and runs
# the suite against that path explicitly. It does not read the ambient working
# directory, so it passes from anywhere and at any clone depth.

ARCH ?= $(shell uname -s)
OUT = out/$(ARCH)
TEST = $(OUT)/git2pp_test

# libgit2 flags come from pkg-config where it knows the package (macOS
# Homebrew, most distros with the -dev package); otherwise fall back to the
# bare linker flag that a default /usr/lib install provides.
GIT2_CFLAGS := $(shell pkg-config --cflags libgit2 2>/dev/null)
GIT2_LIBS := $(shell pkg-config --libs libgit2 2>/dev/null || echo -lgit2)

CXXFLAGS += -std=c++17 -I../vendor/include $(GIT2_CFLAGS)
LDLIBS += $(GIT2_LIBS)
LINK.o = $(LINK.cc)

.PHONY: all test clean

all: $(TEST)

test: $(TEST) fixture.sh
	@set -eu; \
	fixture_parent="$$(mktemp -d)"; \
	trap 'chmod -R u+w "$$fixture_parent" 2>/dev/null || true; rm -rf "$$fixture_parent"' EXIT; \
	./fixture.sh "$$fixture_parent/repo"; \
	GIT2PP_FIXTURE_REPO="$$fixture_parent/repo" ./$(TEST)

$(TEST): $(TEST).o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(TEST).o: git2pp_test.cc ../git2pp.h ../vendor/include/doctest/doctest.h | $(OUT)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(OUT):
	mkdir -p $@

clean:
	rm -rf out
