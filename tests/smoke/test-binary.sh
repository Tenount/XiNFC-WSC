#!/bin/sh
# Smoke test: verify xinfc-wsc binary runs and reports correct version
# Usage: test-binary.sh <path-to-binary> <expected-version>

set -e

BINARY="${1:-xinfc-wsc}"
VERSION="${2:-}"
[ -n "$VERSION" ] || {
    echo "FAIL: no version argument" >&2
    exit 1
}

pass() { echo "PASS: $*"; }
fail() {
    echo "FAIL: $*" >&2
    exit 1
}

# No args: exits non-zero, check usage + flags
usage=$("$BINARY" 2>&1) || echo "INFO: binary exits non-zero without args (expected)"
echo "$usage" | grep -q "USAGE" || fail "no USAGE in output"
pass "usage output present"
ver=$("$BINARY" --version 2>&1) || fail "--version should exit 0"
echo "$ver" | grep -q "$VERSION" || fail "version $VERSION not found in --version"
pass "version $VERSION found"
for flag in "--read --json" "--clear" "--write-url" "--write-text" "--scan"; do
    echo "$usage" | grep -qF -- "$flag" || fail "flag not found: $flag"
    pass "flag found: $flag"
done

# --help / -h
"$BINARY" --help 2>&1 | grep -q "USAGE" || fail "--help no USAGE"
pass "--help exits 0"
pass "--help shows usage"
"$BINARY" -h >/dev/null 2>&1 || fail "-h should exit 0"
pass "-h exits 0"

# --version / -V
ver=$("$BINARY" --version 2>&1) || fail "--version should exit 0"
pass "--version exits 0"
for pattern in "Copyright" "GPL" "NO WARRANTY"; do
    echo "$ver" | grep -q "$pattern" || fail "--version missing: $pattern"
    pass "--version contains $pattern"
done
"$BINARY" -V 2>&1 | grep -q "Copyright" || fail "-V missing Copyright"
pass "-V contains copyright"

echo "All smoke tests passed."
