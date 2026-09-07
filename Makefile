# The pre-push gate (think 🎯T8). libgit2pp is a single header; the only
# oracle that ever existed is demo/walk, which CI built under Linux and never
# ran anywhere else. This wires it to the local toolchain.
.PHONY: gate gate-residue demo hooks hooks-check

# Homebrew does not put libgit2 on the default include path.
LIBGIT2 := $(shell brew --prefix libgit2 2>/dev/null)
ifneq ($(LIBGIT2),)
DEMO_FLAGS := CXXFLAGS="-std=c++17 -Wall -Wextra -I$(LIBGIT2)/include" \
              LDLIBS="-L$(LIBGIT2)/lib -lgit2"
endif

gate: hooks-check gate-residue demo
	@scripts/gate-walk.sh

# -Wall -Wextra is not clean today: git2pp.h emits reorder-init-list and
# unused-parameter warnings from the iterator templates. A ratchet, not a
# threshold — the count is locked in both directions, so it cannot creep up
# and a genuine cleanup has to move this number deliberately.
DEMO_WARNINGS := 11

demo:
	@rm -rf demo/out
	@log=$$(mktemp); \
	 if ! $(MAKE) --no-print-directory -C demo ARCH=$$(uname -s | tr 'A-Z' 'a-z') $(DEMO_FLAGS) \
	      > "$$log" 2>&1; then cat "$$log"; rm -f "$$log"; \
	      echo "✗ demo does not build"; exit 1; fi; \
	 n=$$(grep -cE '^[^ ].*: warning: ' "$$log" || true); rm -f "$$log"; \
	 if [ "$$n" != "$(DEMO_WARNINGS)" ]; then \
	   echo "✗ demo build warnings: $$n, ratchet says $(DEMO_WARNINGS)"; \
	   echo "  fewer is good news — lower DEMO_WARNINGS in the Makefile and commit it"; \
	   exit 1; fi; \
	 echo "✓ demo builds ($$n known warnings, ratcheted)"

# git2pp.h is 555 lines of RAII wrappers; walk exercises a handful of them.
gate-residue:
	@echo "================================================================"
	@echo "gate residue — the oracle is one demo program:"
	@echo "  demo/walk touches repository open, revwalk, commit and signature"
	@echo "  the rest of git2pp.h (555 lines) is compiled, never executed"
	@echo "  no test asserts that a wrapper frees what it owns"
	@echo "  -Wall -Wextra is dirty: $(DEMO_WARNINGS) warnings in git2pp.h"
	@echo "  Linux is CI-only (.github/workflows/ccpp.yml)"
	@echo "================================================================"

# Wire the pre-push gate. A relative core.hooksPath resolves against
# whichever worktree the push runs from, so one setting covers them all.
# Redirecting hooksPath disables .git/hooks entirely; scripts/hooks/pre-push
# chains git-lfs's pre-push so LFS uploads would survive if this repo ever
# tracked LFS objects (it does not today).
hooks:
	@git config core.hooksPath scripts/hooks && chmod +x scripts/hooks/* && \
	 echo "✓ core.hooksPath=scripts/hooks (pre-push runs make gate)"

# Warning only: the gate must still run on a runner that has no hooks.
hooks-check:
	@if [ -z "$$GITHUB_ACTIONS" ] && [ "$$(git config --get core.hooksPath)" != "scripts/hooks" ]; then \
	  echo "⚠  pre-push gate is not wired in this clone: run 'make hooks'"; fi
