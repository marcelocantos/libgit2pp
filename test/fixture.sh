#!/usr/bin/env bash
#
# Build a throwaway git repository whose object ids are fully deterministic, so
# the C++ test suite can assert exact oids instead of asserting "the ambient
# repository happened to have a parent commit".
#
# Everything that feeds the commit hash is pinned: tree contents, author and
# committer identity, both timestamps, the message, and the branch name. The
# host's global/system gitconfig is neutralised so a stray user setting cannot
# perturb the hashes.
#
# Usage: fixture.sh <destination-directory>
# The destination is created fresh; an existing directory is an error.

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <destination-directory>" >&2
    exit 2
fi

dest=$1

if [[ -e "$dest" ]]; then
    echo "$0: $dest already exists; refusing to overwrite" >&2
    exit 1
fi

# Pinned so the resulting oids never move. Keep in sync with the expected
# constants in test/git2pp_test.cc.
readonly FIXTURE_BRANCH=master
readonly FIXTURE_NAME='Fixture Author'
readonly FIXTURE_EMAIL='fixture@example.invalid'
readonly FIXTURE_DATE='2000-01-01T00:00:00+00:00'

export GIT_CONFIG_GLOBAL=/dev/null
export GIT_CONFIG_SYSTEM=/dev/null
export GIT_AUTHOR_NAME=$FIXTURE_NAME
export GIT_AUTHOR_EMAIL=$FIXTURE_EMAIL
export GIT_AUTHOR_DATE=$FIXTURE_DATE
export GIT_COMMITTER_NAME=$FIXTURE_NAME
export GIT_COMMITTER_EMAIL=$FIXTURE_EMAIL
export GIT_COMMITTER_DATE=$FIXTURE_DATE

mkdir -p "$dest"
git -C "$dest" init --quiet
git -C "$dest" symbolic-ref HEAD "refs/heads/$FIXTURE_BRANCH"

printf 'first\n' > "$dest/a.txt"
git -C "$dest" add a.txt
git -C "$dest" commit --quiet -m 'first commit'

printf 'second\n' > "$dest/b.txt"
git -C "$dest" add b.txt
git -C "$dest" commit --quiet -m 'second commit'
