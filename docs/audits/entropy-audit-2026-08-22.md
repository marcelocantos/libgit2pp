# Entropy audit — libgit2pp

Date: 2026-08-22
Mode: full (entropy + explicit hygiene validation)
Auditor snapshot: working tree only; remotes were not fetched or pushed.

## Executive summary

- **Snapshot:** `/Users/marcelo/work/github.com/marcelocantos/libgit2pp`
  - Branch: `master` (tracks `origin/master`)
  - HEAD: `360e2dc4f6823480d58f1d49ed1c5b67326caa2d` — `Fix missing \n in usage message` (2019-12-08)
  - Initial `git status --porcelain=v1 -b`: `## master...origin/master` (clean working tree; no staged, unstaged, or untracked files)
  - Local `origin/master` matches this HEAD. GitHub default-branch HEAD is `2188fcc230ef3e031e8036aafa84437c3d5e3bbe` (2026-06-20, CI-only: `actions/checkout@v6` + `fetch-depth: 0`), one commit ahead of this clone. This audit cites the working-tree snapshot and treats GitHub `2188fcc` as counterevidence where it remediates a finding.
- **Scope:** the whole repository (8 tracked files). C++17 header-only wrapper + demo, Makefile, Dockerfile, GitHub Actions. No `AGENTS.md`, `CLAUDE.md`, `hygiene.yaml`, or `bullseye.yaml`.
- **Exclusions:** no generated, vendored, or fixture trees in-repo. Homebrew libgit2 1.9.4 headers were used only as an auxiliary API inventory / compile probe, not as a shipped dependency.
- **Headline mechanism:** a single-header metaprogramming façade over libgit2 that *does* avoid cloning the C call surface, but still hand-maintains type/lifetime/iterator tables, and whose only executable oracle is a demo that walks *whatever git repository `open_ext` finds* with no assertions.
- **Highest-consequence findings:**
  - **ENT-001 (P1):** `demo` `make test` is the shipped correctness gate and is not hermetic; GitHub Actions run 27488956604 aborted with `parent 0 does not exist` on a shallow clone.
  - **ENT-002 (P1):** `UniquePtr::operator[]` / `Session::operator[]` throw via `check()` only for `T **` / `git_oid *` out-params; int-returning libgit2 methods are returned unchecked.
- **Unverified residue:** no consumer corpus outside this repo; no sanitizer/fuzz run; Session copy/move never exercised; `as<U>()` type validity unenforced; GitHub `ubuntu-latest` libgit2 package will drift.

## Scope and exclusions

Tracked files at HEAD:

| Path | Role |
|---|---|
| `git2pp.h` | Entire library |
| `demo/walk.cc` | Example + sole test binary |
| `demo/Makefile` | Build + `make test` |
| `demo/.gitignore` | Ignores `out/` |
| `.github/workflows/ccpp.yml` | Shipped CI |
| `Dockerfile` | Alternate, unused-by-CI image |
| `README.md`, `LICENSE` | Docs / Apache-2.0 |

Skipped as not in-tree: Homebrew libgit2 sources, GitHub-only commit `2188fcc` (read via `gh api`, not checked out).

Languages judged: C++17 (read `~/.claude/cpp.md`), POSIX Make / CI shell (read `~/.claude/bash.md`). No Python, Go, Rust, SQL, or web product surface.

## Commands run

| Command | Version | Exit | Path | Notes |
|---|---|---|---|---|
| `git status --porcelain=v1 -b` | git 2.55.0 | 0 | provenance | `## master...origin/master`; clean |
| `git rev-parse HEAD` | git 2.55.0 | 0 | provenance | `360e2dc4f6823480d58f1d49ed1c5b67326caa2d` |
| `git log --oneline --all`; `git log --name-only` | git 2.55.0 | 0 | history | 23 commits touch `git2pp.h`, 20 touch `demo/walk.cc`; last library change 2019-12-08 |
| `git ls-files` | git 2.55.0 | 0 | inventory | 8 paths |
| `gh run list`; `github__actions_list` | gh 2.97.0 | 0 | shipped CI | 3 runs; latest GitHub `master` success on `2188fcc`; failure 27488956604 on shallow clone |
| `gh api repos/marcelocantos/libgit2pp/commits/master` | gh 2.97.0 | 0 | provenance | GitHub master `2188fcc…`, 2026-06-20 |
| `gh api …/compare/360e2dc…2188fcc` | gh 2.97.0 | 0 | provenance | 1 file: `.github/workflows/ccpp.yml` |
| `pkg-config --modversion libgit2` | libgit2 1.9.4 | 0 | auxiliary | Homebrew, not the product link line |
| `clang++ -std=c++17 -Wall -Wextra -Wpedantic -Wshadow -c demo/walk.cc` (output under `/tmp/libgit2pp-audit-2026-08-22`, pkg-config cflags/libs) | Homebrew clang 22.1.8 | 0 | auxiliary | 11 warnings; links and `walk HEAD` exits 0 on this repo |
| same with `-DGIT_DEPRECATE_HARD` | clang 22.1.8 | 1 | auxiliary | `giterr_last`, `git_buf_free`, `git_oidarray_free`, `git_strarray_free` gone |
| `clang-tidy demo/walk.cc -checks='clang-analyzer-*,bugprone-*,…'` | LLVM 22.1.8 | 0 | auxiliary | Confirms broken `enable_if`, Session/UniquePtr special members. No repo `.clang-tidy`; libgit2 header noise ignored |
| `~/.claude/skills/hygiene/hygiene_check.py` | uv/python 3.13.0 | 1 | hygiene | `FileNotFoundError: …/hygiene.yaml` |
| `docker build` / `ARCH=linux make` in-tree | — | not run | — | Would write `demo/out/` or pull EOL Ubuntu; compile probe used `/tmp` instead |

Shipped-path evidence is GitHub Actions `C/C++ CI` (`ccpp.yml`) plus the workflow YAML in this tree. Local clang++ against Homebrew libgit2 is **auxiliary** (also conflicts with the fleet C++ rule “never link Homebrew C/C++ libraries”; this product is a header consumers compile against *their* libgit2).

Limitations: no matrix of libgit2 0.24–1.9; no ASan/UBSan; no clone detector configured; `jscpd`/`cppcheck` not installed (not added).

## Observed architecture

```
consumer .cc
    │  #include "git2pp.h"   (in lieu of git2.h)
    ▼
git2pp.h  ──includes──►  <git2.h> (system / pkg libgit2)
    │
    ├── git2pp::Session          RAII git_libgit2_init/shutdown
    ├── git2pp::check / Error    int rc → exception
    ├── git2pp::UniquePtr<T>     unique_ptr + obj_free<T> + operator[] wrap
    ├── git2pp::detail::wrap / wrapOid
    └── Iterator* / Iterable / MaybeIterable<UniquePtr<…>>
            ▲
demo/walk.cc  (only in-repo consumer)
demo/Makefile  (ARCH-required; links -lgit2)
.github/workflows/ccpp.yml  (apt libgit2-dev, ARCH=linux make test)
Dockerfile  (ubuntu:18.04, apt libgit2-dev + unpinned libgit2 clone)
```

**Declared rules** (README): one header; metaprogramming so the C API is not re-declared; `Session` + `UniquePtr` + `operator[]`; `as<U>()` is an unchecked cast; Apache-2.0.

**Observed rules that agree:** `git2pp.h` is the only library translation unit; function wrapping is signature-driven rather than a per-function trampoline; free/dup still need explicit specializations; demo exercises Session, UniquePtr, peel/`as`, revwalk/refs/branch/config/index/notes/rebase iterators.

**Observed, inferred:** error throwing is coupled to out-parameter shape, not to libgit2’s `int rc` convention (ENT-002). `Iterable::begin` downcasts the mixin `this` to `UniquePtr<git_iterator>*` (`git2pp.h:492`). `Session` is a process-wide refcount ticket, not a session object (no members; `operator[]` captures unused `this`).

**Contradictions:** README claims the design avoids replicating the API; `GIT2PP_OBJ_FREE_` / `OBJ_DUP_` / `MaybeIterable` specializations *are* a hand-maintained replica of libgit2 types (ENT-003). README “before” sample calls `git_reference_dwim(&ref, "master")` with no repository (ENT-007). Dockerfile builds libgit2 from `master` then `make test` still links `-lgit2` from apt (ENT-006).

**Unknown intent:** whether 0.24-era gates are still a support promise; whether `Dockerfile` is load-bearing; whether `as<U>()` is meant to stay unchecked forever.

No cycles. High fan-in: `git2pp.h` (the whole product). High fan-out: `UniquePtr::operator[]` (every libgit2 method a caller binds).

## Dimension vector

| Dimension | State | Evidence summary | Change from baseline |
|---|---|---|---|
| Architecture topology | healthy | Single header, one demo, one C library; direction is consumer → git2pp → libgit2. Mixin iterators are local complexity, not a layering violation. | n/a (first full audit) |
| Redundancy / sources of truth | concern | Type free/dup tables vs libgit2 headers; README vs `demo/walk.cc`; Dockerfile vs Actions vs Makefile for “how to get libgit2”. | n/a |
| Change amplification | concern | New libgit2 object type or `_free`→`_dispose` rename requires `git2pp.h` macros; new iterator shape needs a new `*Iterator` class. | n/a |
| Local code quality | concern | Session Rule of 5; dummy `std::enable_if` (not `::type`); `as<U>()` C cast; iterator `operator*` moves from `const`; 11 clang++ warnings on the demo. | n/a |
| Correctness / verification | critical | Sole shipped test is `out/$(ARCH)/walk HEAD` with zero assertions; depends on host repo having a parent commit; rebase path is catch-and-ignore. | n/a |
| Security / dependencies | concern | Unpinned `libgit2-dev` / `git clone https://github.com/libgit2/libgit2.git`; EOL `ubuntu:18.04`; no scanners; no secrets in tree. | n/a |
| Build / release / operations | concern | This snapshot’s Actions still `checkout@v1` and `on: [push]` only; no tags/releases; Dockerfile not used by CI. GitHub master already bumped checkout. | n/a |
| Documentation / governance | concern | README drift; open issue #6 since 2019; no CONTRIBUTING/CODEOWNERS/hygiene/AGENTS. LICENSE present. | n/a |

Do not collapse this vector to a score.

## Findings

### ENT-001: The only oracle is a non-hermetic demo that requires HEAD^ in whatever repo `open_ext` finds

- **Priority:** P1
- **Dimensions:** Correctness / verification; Build / release / operations
- **Status:** observed fact
- **Evidence:**
  - `demo/Makefile:15-16` — `test: $(WALK)` then `$< HEAD` (no expected output, no fixture).
  - `demo/walk.cc:8-12` — `git_repository_open_ext(".", 0, nullptr)` (search parents) then `git_commit_parent(…, 0)` unconditionally.
  - `.github/workflows/ccpp.yml:17-19` — shipped job is `ARCH=linux make test` in `demo/`.
  - GitHub Actions run [27488956604](https://github.com/marcelocantos/libgit2pp/actions/runs/27488956604) (`t4-actions-node24`, checkout@v6 default depth 1): compile succeeded against Ubuntu `libgit2-dev` 1.7.2; `walk HEAD` aborted `git2 error -3/3: parent 0 does not exist`.
  - Follow-up commit `5f6111e` / GitHub `2188fcc` set `fetch-depth: 0` so the **library’s own history** becomes the fixture. That is a workaround, not a test.
  - Auxiliary: `/tmp/libgit2pp-audit-2026-08-22/walk HEAD` exit 0 on this clone because HEAD `360e2dc` has parent `0b245db`.
- **Mechanism:** the gate decides “demo did not throw and HEAD has a parent in the checkout,” not any library invariant. Shallow clones, orphan branches, or running the binary outside a git work tree change the result without touching `git2pp.h`. Notes and rebase failures are swallowed (`demo/walk.cc:56-74`).
- **Blast radius:** every libgit2pp change can ship with a green badge while UniquePtr, iterators, `check()`, and `as<>` are unasserted. CI is coupled to this repository’s commit graph.
- **Counterevidence checked:** README presents `demo/` as an example, not a suite — but CI names the step `make test`. No `tests/` tree, no doctest (fleet C++ default), no golden output. Issue #6 acknowledges rebase is untested.
- **Smallest coherent remediation:** build a throwaway repo in the test recipe (`git init` + two commits, or a tracked fixture) and assert parent oid, ref walk, and at least one error path. Stop using the libgit2pp checkout as data.
- **Verification:** CI with `fetch-depth: 1` (or `GIT_DIR` pointing at an empty repo) must still pass; a broken `check()` or parent wrap must fail.
- **Ratchet candidate:** workflow step on a fixture repo; later `hygiene.yaml` `correctness.demo-test` → `ci_job: ccpp.yml#build` plus a `command:` that fails if `walk` is invoked on `$GITHUB_WORKSPACE` history.

### ENT-002: Error policy is overloaded on C signature shape, not on libgit2 `int rc`

- **Priority:** P1
- **Dimensions:** Correctness / verification; Local code quality
- **Status:** observed fact
- **Evidence:**
  - `git2pp.h:50-58` — `check(int rc)` throws `Error` when `rc < 0`.
  - `git2pp.h:276-280` / `283-287` — `wrap` / `wrapOid` call `check`.
  - `git2pp.h:224-236` — `T **` methods go through `wrap` (throwing).
  - `git2pp.h:245-257` — `R (*method)(T *, …)` returns `method(...)` **unchecked**.
  - `demo/walk.cc:18-19` — `revwalk[git_revwalk_sorting](GIT_SORT_TIME)` and `revwalk[git_revwalk_push](…)` are int-returning and ignored.
  - README “`check()` throws Error when a libgit2 result indicates failure” and `operator[]` “Wraps a libgit2 function” do not mention the split.
- **Mechanism:** callers reasonably treat `obj[fn](args)` as the throwing façade. Failures of status-only methods become empty walks or later unexplained `Error`s. `check` already accepts `rc >= 0`, so wrapping int returns would not crush positive “truncated” codes.
- **Blast radius:** every non-constructor libgit2 call bound through `UniquePtr::operator[]` (sort, push, set-*, checkout, etc.).
- **Counterevidence checked:** some libgit2 getters return pointers/`int` counts rather than status; those use the `R (T const *, …)` overload and should stay unchecked. clang-tidy did not flag the missing `check` (signature-dependent). Demo success does not exercise a failed `git_revwalk_push`.
- **Smallest coherent remediation:** for overloads where `R` is `int`, call `check(method(...))` and return `void` (or the rc if a caller must see `GIT_ENOTFOUND` without exception — then document that one policy). Keep non-int returns raw.
- **Verification:** unit test: `revwalk[git_revwalk_push]` with a zeroed oid throws `git2pp::Error`. A regression that drops `check` fails that test.
- **Ratchet candidate:** compile-time `static_assert` or a small test binary in CI; not a regex.

### ENT-003: Lifetime/dup/iterator tables are a second libgit2 ABI, already behind 1.x

- **Priority:** P2
- **Dimensions:** Redundancy / sources of truth; Change amplification; Security / dependencies
- **Status:** observed fact (inventory); inference (future libgit2 minors keep adding types)
- **Evidence:**
  - `git2pp.h:15-24` — feature gates only `LIBGIT2_VER_MINOR < 28` / `< 25` on **major == 0**.
  - `git2pp.h:79-170` — `GIT2PP_OBJ_FREE_`, commented `cred`/`hashsig`, `GIT2PP_OBJ_DUP_`, `GIT2PP_OBJ_OBJECT_DUP_`.
  - `git2pp.h:499-549` — per-type `MaybeIterable` specializations.
  - Auxiliary inventory of `/opt/homebrew/include/git2` 1.9.4: public `*_free` not specialized in git2pp include `mailmap`, `worktree`, `transaction`, `refspec`, `credential`, `message_trailer_array`, `commit_graph` (plus sys writers). `git_submodule_dup` exists; git2pp has no `OBJ_DUP_(submodule)`.
  - `-DGIT_DEPRECATE_HARD` compile exit 1: `giterr_last` (`git2pp.h:54`), `git_buf_free` (`git2pp.h:88`), `git_oidarray_free` (`git2pp.h:113`), `git_strarray_free` (`git2pp.h:128`) — replaced by `git_error_last` / `*_dispose`.
  - Default Ubuntu CI still compiles because `deprecated.h` aliases remain (`GIT_DEPRECATE_HARD` unset).
- **Mechanism:** README’s “don’t replicate the API” is true for *calls* and false for *ownership*. Each libgit2 type still needs a deleter row or `UniquePtr<T>` does not compile. `_free` → `_dispose` is a silent break under hard deprecation. Version macros used (`LIBGIT2_VER_*`) themselves live in `deprecated.h`.
- **Blast radius:** any consumer on libgit2 1.x using worktree/mailmap/credential/transaction UniquePtrs; any build that sets `GIT_DEPRECATE_HARD`.
- **Counterevidence checked:** wrapping via `operator[]` still works for functions whose types *are* registered (demo compiled on 1.9.4 and CI 1.7.2). `GIT_OBJ_COMMIT` remains an alias (`deprecated.h:640`). Incomplete coverage is partly the original point of the library.
- **Smallest coherent remediation:** map `_free` vs `_dispose` with version/`GIT_DEPRECATE_HARD` ifdefs; add the public 1.x types that have `*_free`; optionally add a CI job `-DGIT_DEPRECATE_HARD`. Do not generate the whole C API.
- **Verification:** CI compile of the demo with `-DGIT_DEPRECATE_HARD` and a TU that `UniquePtr<git_worktree>`/`git_mailmap`/`git_credential` compiles.
- **Ratchet candidate:** `g++ … -DGIT_DEPRECATE_HARD` step; a typed static list test.

### ENT-004: `Session` destructor pairs `init`/`shutdown` but copies are implicit and empty

- **Priority:** P2
- **Dimensions:** Local code quality; Correctness / verification
- **Status:** observed fact (type); inference (runtime imbalance if copied)
- **Evidence:**
  - `git2pp.h:292-307` — ctor calls `git_libgit2_init()`, dtor `git_libgit2_shutdown()`, no copy/move =delete; `operator[]` captures unused `this`.
  - `git_libgit2_init` / `_shutdown` are process-wide refcounts (`git2/global.h`); extra shutdown without init drops the global count.
  - clang-tidy `cppcoreguidelines-special-member-functions` on `git2pp.h:292`.
  - `git_libgit2_init` return (error or count) is ignored.
- **Mechanism:** copy/move produce extra destructor shutdowns without extra inits. Typical `Session git2;` on the stack is fine; `Session a = b` is not.
- **Blast radius:** any caller that passes `Session` by value or stores it in a container.
- **Counterevidence checked:** demo and README construct one automatic `Session`. No test copies it. libgit2 documents matched init/shutdown, not “shutdown is idempotent at zero.”
- **Smallest coherent remediation:** delete copy/move; `check(git_libgit2_init())` (treat negative as error).
- **Verification:** `static_assert(!std::is_copy_constructible_v<git2pp::Session>)` in a CI TU.
- **Ratchet candidate:** that `static_assert` in the demo compile, or a tiny `session_traits.cc`.

### ENT-005: `as<U>()` is an unchecked C cast; the rebase demo is known-broken and the only “test” of that iterator

- **Priority:** P2
- **Dimensions:** Correctness / verification; Local code quality; Documentation / governance
- **Status:** observed fact
- **Evidence:**
  - `git2pp.h:259-267` — `as() &&` does `(U *)t_.release()`; const `as()` does `(U *)t_.get()`.
  - README.md:72-75 document “WARNING: Will succeed for any pair of types.”
  - `demo/walk.cc:10` — peel to `GIT_OBJ_COMMIT` then `.as<git_commit>()` (the intended path).
  - `demo/walk.cc:65-74` — `parent0.as<git_reference>()` where `parent0` is `UniquePtr<git_commit>`, passed to `git_annotated_commit_from_ref`; comment “Too lazy to test this properly”; catch-all prints `failure not unexpected`.
  - GitHub issue [#6](https://github.com/marcelocantos/libgit2pp/issues/6) (open since 2019-09-08): “Implemented, but not yet tested.”
- **Mechanism:** the peel path is a type-pun the C API actually uses (`git_object` vs `git_commit`). The rebase path is a different pun (`commit` as `reference`) and is not a test of `RebaseIterable`; CI still passes because the exception is expected.
- **Blast radius:** any `as<U>()`; rebase iterator has no other caller in-tree.
- **Counterevidence checked:** peel-to-commit in the README example is the documented use. Issue #6 is explicit owner residue, not a forgotten crash.
- **Smallest coherent remediation:** keep `as` for object-family peels; remove or rewrite the rebase block to `git_annotated_commit_lookup` / a real rebase fixture; close or implement #6.
- **Verification:** a test that `as<git_commit>()` after `git_reference_peel(GIT_OBJECT_COMMIT)` yields `git_commit_id`; a test that rebase iteration over a known two-commit rebase emits one op. Demo must not catch-all.
- **Ratchet candidate:** fail CI if `walk.cc` contains `failure not unexpected` / `Too lazy to test`.

### ENT-006: Three competing stories for “how this is built and which libgit2”

- **Priority:** P2
- **Dimensions:** Redundancy / sources of truth; Build / release / operations; Security / dependencies
- **Status:** observed fact
- **Evidence:**
  - `demo/Makefile:1-2,5-6` — `CXXFLAGS += -std=c++17`, `LDLIBS = -lgit2`, `$(error missing ARCH)`.
  - `.github/workflows/ccpp.yml:8,12-19` — `ubuntu-latest`, unpinned `apt-get install libgit2-dev`, `ARCH=linux make test`.
  - `Dockerfile:1-27` — `FROM ubuntu:18.04` (EOL); `python` (2.x on that image); `apt` `libgit2-dev` **and** `git clone https://github.com/libgit2/libgit2.git` with no pin, `cmake --build` of that clone; `WORKDIR /src/demo` then `ARCH=linux make test` which still links `-lgit2` (apt), not the just-built tree.
  - CI does not reference `Dockerfile`. No image publish, no tags (`gh release list` empty).
- **Mechanism:** the cloned libgit2 build is dead work; apt and source can disagree; 18.04/python2 will not reproduce GitHub `ubuntu-latest` (currently libgit2 1.7.x per run 27488956604). Unpinned clone is a supply-chain moving target.
- **Blast radius:** anyone following Dockerfile vs badge vs Makefile gets different libgit2; Dockerfile does not validate the cmake’d library.
- **Counterevidence checked:** header-only distribution is intentional (`README.md:5-6`); not vendoring libgit2 is appropriate for a wrapper (fleet `cpp.md` vendor rule applies to *shipping binaries*, not this drop-in header). Makefile `ARCH` split is deliberate (`966217e`).
- **Smallest coherent remediation:** delete Dockerfile or make it the CI image with a pinned `libgit2-dev` and no unused clone; pin the apt package in Actions; one documented compile line.
- **Verification:** grep CI and README for a single install command; `docker build` (if kept) must `ldd` the demo against the intended libgit2.
- **Ratchet candidate:** `absent:` Dockerfile if retired; otherwise a CI `docker build` job.

### ENT-007: README and demo have drifted; the “before” C sample is not valid libgit2

- **Priority:** P2
- **Dimensions:** Documentation / governance; Redundancy / sources of truth
- **Status:** observed fact
- **Evidence:**
  - `README.md:56-58` — raw sample `git_reference_dwim(&ref, "master")` omits the `git_repository *` argument (libgit2: `git_reference_dwim(git_reference **out, git_repository *repo, const char *shorthand)`).
  - `README.md:141-146` uses `git_repository_open`; `demo/walk.cc:8` uses `git_repository_open_ext`.
  - `README.md:146` / `demo/walk.cc:10` still use `GIT_OBJ_COMMIT` (compat alias).
  - `demo/walk.cc:79` prints `Usage: demo <branch>` but the binary is `walk` and `make test` passes `HEAD`.
  - Status badge (`README.md:3`) points at `C/C++ CI`; that workflow on this snapshot is still `checkout@v1`.
- **Mechanism:** two “how to call this” sources. The C “before” snippet cannot be copied. Usage string vs Makefile target confuse the only runnable example.
- **Blast radius:** new users; issue #2/#4 were already documentation requests (closed).
- **Counterevidence checked:** the “after” README snippet is still a fair subset of the wrapper. Badge URL matches the workflow `name:`.
- **Smallest coherent remediation:** fix `git_reference_dwim` arity; align open vs open_ext or document both; usage → `walk <shorthand>`.
- **Verification:** compile the README “before” and “after” as snippets in CI (or extract them).
- **Ratchet candidate:** a `README` example compile job; later `docs.examples-compile` hygiene item.

### ENT-008: This snapshot’s CI is `checkout@v1` / push-only; GitHub default branch already moved

- **Priority:** P2
- **Dimensions:** Build / release / operations
- **Status:** observed fact
- **Evidence:**
  - `.github/workflows/ccpp.yml:4,11` — `on: [push]`; `uses: actions/checkout@v1`.
  - GitHub master `2188fcc` (PR [#7](https://github.com/marcelocantos/libgit2pp/pull/7), 2026-06-20) and local branch `t4-actions-node24` (`5f6111e`) change that to `actions/checkout@v6` + `fetch-depth: 0`. Latest run on GitHub master: [27885095881](https://github.com/marcelocantos/libgit2pp/actions/runs/27885095881) success.
  - No `pull_request` trigger: PR CI only ran because the head branch received a push.
  - `apt-get install libgit2-dev` has no `apt-get update` (worked on the 2026-06-14 runner; not guaranteed).
- **Mechanism:** this clone’s HEAD is not GitHub `master`. Re-pushing `360e2dc` would restore Node-12-era checkout. Push-only workflows skip forks that do not write the branch.
- **Blast radius:** Actions on this exact commit; PR checks from forks.
- **Counterevidence checked:** GitHub production already remediates checkout/depth. Failure 27488956604 is the shallow-clone interaction with ENT-001, not a compile break.
- **Smallest coherent remediation:** fast-forward this clone to `2188fcc` when desired; add `pull_request`; `apt-get update` before install. Do not treat `fetch-depth: 0` as a substitute for ENT-001.
- **Verification:** `actions/checkout@v1` absent from `.github/workflows/*`; a PR from a fork still runs the job.
- **Ratchet candidate:** workflow-lint / `hygiene` `ci_job` evidence once `hygiene.yaml` exists.

### ENT-009: Local quality nits already produced segfault-fix churn; several remain

- **Priority:** P3
- **Dimensions:** Local code quality
- **Status:** observed fact
- **Evidence:**
  - History 2019-09-28: `24cc2fe`, `6f75784`, `0f92cc3` “Fix segfault” while iterating on the demo — verification was “run walk and see.”
  - `git2pp.h:224,245` — `typename = std::enable_if<!std::is_const_v<T>>` (clang-tidy `bugprone-incorrect-enable-if`); SFINAE never fails. Const vs non-const still disambiguated by the function-pointer type, so this is dead intent, not a live wrong overload.
  - `git2pp.h:316` vs `338-339` — `i_` initialized before `next_` but declared after (`-Wreorder-ctor`, 8 instantiations).
  - `git2pp.h:360` — `operator*() const` does `std::move(t_)` from `mutable` storage; second dereference yields an empty `UniquePtr`.
  - `git2pp.h:384` — `StructIterator::operator->` returns `T &`, not a pointer.
  - `git2pp.h:196-209` — copy “ctor” is a template (not a real copy ctor); copy assign always instantiates `obj_dup<T>::dup` (no `HAVE_REFERENCE_DUP` SFINAE). clang-tidy special-member warning on `git2pp.h:190`.
  - Auxiliary clang++: unused `obj_no_free` parameter; unused `this` capture in `Session::operator[]`.
- **Mechanism:** none of these is the present crash (demo ran). They are the same class of lifetime/iterator mistakes that already shipped as segfaults in 2019.
- **Blast radius:** iterator users who dereference twice; copy of `UniquePtr<git_reference>` on pre-0.25 libgit2.
- **Counterevidence checked:** range-for `auto &&` in the demo dereferences once per increment. `LIBGIT2PP_HAVE_REFERENCE_DUP` is 1 on libgit2 1.x because `MAJOR == 0` is false.
- **Smallest coherent remediation:** fix enable_if to `enable_if_t`; delete copy assign when `obj_dup` is missing; make `operator*` non-consuming or document input-iterator consume; reorder fields.
- **Verification:** `-Werror` on the demo job; a test that `*it` twice on a reference iterator still owns a ref (if consume is rejected).
- **Ratchet candidate:** `ARCH=linux make CXXFLAGS='-Werror …'` once warnings are cleared.

## Redundancy and competing-source-of-truth inventory

| Fact | Owners | Drift already? | Disposition |
|---|---|---|---|
| libgit2 object free/dup | libgit2 headers vs `GIT2PP_OBJ_*` macros | Yes — 1.x types and `_dispose` | ENT-003 |
| How to obtain libgit2 | Makefile `-lgit2`, Actions apt, Dockerfile apt+clone | Yes — clone unused | ENT-006 |
| How to call the wrapper | README vs `demo/walk.cc` | Yes — open vs open_ext, dwim arity | ENT-007 |
| Error handling | `check()` vs raw `int` returns | Yes — two policies in one `operator[]` | ENT-002 |
| What CI is | this tree `checkout@v1` vs GitHub `2188fcc` `checkout@v6` | Yes — clone stale vs GitHub | ENT-008 |
| `GIT_OBJ_*` / `giterr_*` | git2pp + demo vs libgit2 1.x names | Compat aliases only | accepted until `GIT_DEPRECATE_HARD` |
| Iterator “Entry” structs | Branch / IndexConflict / Note | Similar shape, different `next` arity | deliberate duplication |

No duplicate runtime authority for git object bytes (libgit2 remains the store). No generated code.

## Healthy structure and deliberate exceptions

- **One header is the product.** `git2pp.h` + `#include <git2.h>` matches README; there is no parallel `src/` tree to rot. Failed to invalidate: `git ls-files` is eight paths.
- **Signature-driven wrap is the right abstraction** for not cloning every `git_*` prototype. `detail::wrap` / `wrapOid` + `operator[]` is the stable idea; keep it when fixing ENT-002.
- **RAII UniquePtr + specialized deleter** is the correct ownership model for libgit2 heap objects. Null copy was fixed (`aa5beff`). Types without `obj_dup` fail at compile rather than double-free.
- **Iterator mixin** (`MaybeIterable<UniquePtr<T>>`) is a single extension point; the demo’s for-range over revwalk/refs/branches/config/index is evidence it works on libgit2 1.7–1.9.
- **Version ifdefs** for index iterator (0.28) and `git_reference_dup` (0.25) are real compatibility, not fashion.
- **Not vendoring libgit2** is correct for a header wrapper; fleet `cpp.md` Homebrew/vendor rules apply to shipped binaries. Consumers bring libgit2.
- **Apache-2.0** `LICENSE` + SPDX-style header comment (`git2pp.h:1-8`).
- **`as<U>()` warning** in README is honest; the peel-to-commit use is legitimate.
- **`demo/.gitignore` `/out`** matches Makefile output. `ARCH` required is explicit.

## Hygiene posture

`hygiene.yaml` is **absent**. Hygiene posture **not declared**. It was not initialized.

Validator invocation from repo root:

```
/Users/marcelo/.claude/skills/hygiene/hygiene_check.py
```

Exit 1:

```
FileNotFoundError: [Errno 2] No such file or directory:
'/Users/marcelo/work/github.com/marcelocantos/libgit2pp/hygiene.yaml'
```

No per-dimension held tiers or floors. Overlap with entropy: ENT-001/008 would be `correctness` / `build` items if declared; ENT-006 Dockerfile would be `build` or `absent:`; LICENSE/README would satisfy a tier-1 `docs`/`governance` floor if someone onboards later. Do not treat this audit’s finding list as a hygiene denominator.

## Oracle coverage and residue

| Property | Decided by |
|---|---|
| Demo compiles and links on GitHub `ubuntu-latest` + apt `libgit2-dev` | Shipped Actions on GitHub `2188fcc` (success). This snapshot’s workflow still `checkout@v1` (ENT-008). |
| Demo compiles on libgit2 1.9.4 / clang 22 | Auxiliary `/tmp` compile, exit 0 |
| Demo `walk HEAD` on a history-rich repo | Auxiliary run exit 0; shipped `make test` only if checkout is deep (ENT-001) |
| Header builds with `GIT_DEPRECATE_HARD` | Auxiliary: **fails** (ENT-003) |
| `check()` throws on libgit2 failure | Partial: notes missing-ref path prints an error (`walk.cc:61-62`); no assert |
| Int-returning methods throw | **Nothing** (ENT-002) |
| `Session` copy safety | **Nothing** (ENT-004) |
| `as<U>()` / rebase iterator | Manual/issue #6; demo accepts failure (ENT-005) |
| UniquePtr free of every libgit2 type | **Nothing** beyond types the demo constructs |
| ASan/UBSan, fuzz, ABI vs 0.24 | **Nothing** |
| Dependency CVEs / secret scan / SBOM | **Nothing** |
| Hygiene floors | Undeclared |

Failed/skipped checks: `GIT_DEPRECATE_HARD` compile (fail); in-tree `make` (skipped: would write `demo/out/`); `docker build` (skipped: EOL pull, dirties nothing useful); `hygiene_check.py` (no yaml).

### Owner residue (intent only)

1. Is libgit2 **1.x** the support floor, or must 0.24–0.28 still compile?
2. Should `as<U>()` stay an unchecked cast, or be constrained to the `git_object` family?
3. Keep `Dockerfile` at all?
4. Onboard `hygiene.yaml`, or leave hygiene undeclared on purpose for this tiny header?

Mechanical work (fixture test, `check` on `int`, delete Session copies, fix README dwim) is not residue.

## Remediation sequence

1. **Oracle seam (ENT-001, ENT-005).** Replace `walk HEAD` against the libgit2pp checkout with a fixture repository and assertions (parent, revwalk count, missing-ref error). Rewrite or drop the rebase catch-all; that is what closes issue #6. Keep `fetch-depth: 0` only if something else needs history.
2. **Public contract (ENT-002, ENT-004).** `check()` int-returning `operator[]` overloads; delete `Session` copy/move; check `git_libgit2_init`. Add a TU `static_assert` and one throwing test.
3. **Single build story (ENT-006, ENT-008).** Align this clone with GitHub `2188fcc` or equivalent; add `pull_request`; pin or document libgit2-dev; retire or repair Dockerfile so it does not clone-and-ignore libgit2.
4. **ABI table (ENT-003).** `_dispose` ifdefs + public 1.x types; optional `-DGIT_DEPRECATE_HARD` job. Do not autogenerate the whole C API.
5. **Docs (ENT-007) and nits (ENT-009)** after the oracle exists so README snippets can compile in CI. `-Werror` only once warnings are gone.
6. **Ratchet.** If the owner wants hygiene later, declare floors that match reality (likely `correctness: 1` after a real test job, `docs: 1` for LICENSE/README, others 0/`planned`). Re-run this audit against the same finding IDs.

No architectural rewrite is required. The header-as-façade is the design to keep.
