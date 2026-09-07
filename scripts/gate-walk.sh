#!/usr/bin/env bash
# Run demo/walk against a scratch repository built here, and assert the shape
# of what it prints. Running it against the checkout it lives in would make
# the assertions depend on this repo's history, and would be one more thing
# reading the real .git from inside a hook.
set -euo pipefail

arch="$(uname -s | tr 'A-Z' 'a-z')"
walk="$PWD/demo/out/$arch/walk"
[[ -x "$walk" ]] || { echo "✗ $walk not built" >&2; exit 1; }

scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT

git -C "$scratch" init -q
git -C "$scratch" -c user.name=gate -c user.email=gate@example.invalid \
	commit -q --allow-empty -m "first"
git -C "$scratch" -c user.name=gate -c user.email=gate@example.invalid \
	commit -q --allow-empty -m "second"

out="$(cd "$scratch" && "$walk" HEAD)"

fail=0
check() {
	if grep -qE "$2" <<<"$out"; then echo "✓ walk prints $1"
	else echo "✗ walk did not print $1" >&2; fail=1; fi
}
# walk prints HEAD^ and then the PARENT commit's author and message,
# not HEAD's. Asserting HEAD's would pass by accident on a one-commit
# repo and fail here, so the scratch repo has two commits on purpose.
check "the parent sha"  '^HEAD\^ = [0-9a-f]{40}$'
check "the author"      '^author = gate$'
check "the message"     '^message = first$'

revs="$(sed -n '/^revs:$/,$p' <<<"$out" | grep -cE '^  [0-9a-f]{40}$' || true)"
if [[ "$revs" -eq 2 ]]; then echo "✓ walk lists both revisions"
else echo "✗ walk listed $revs revisions, expected 2" >&2; fail=1; fi

exit "$fail"
