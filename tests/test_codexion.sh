#!/usr/bin/env bash

set -u

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BINARY="$ROOT_DIR/codexion"
RUNS=${RUNS:-3}
TIMEOUT_SECONDS=${TIMEOUT_SECONDS:-40}
TMP_DIR=$(mktemp -d)
PASS_COUNT=0
FAIL_COUNT=0

cleanup() {
	rm -rf "$TMP_DIR"
}
trap cleanup EXIT

pass() {
	PASS_COUNT=$((PASS_COUNT + 1))
	printf 'PASS: %s\n' "$1"
}

fail() {
	FAIL_COUNT=$((FAIL_COUNT + 1))
	printf 'FAIL: %s\n' "$1" >&2
}

run_case() {
	local name=$1
	local expected=$2
	local deadline=$3
	shift 3
	local output="$TMP_DIR/${name}.log"
	local status

	timeout "$TIMEOUT_SECONDS" "$BINARY" "$@" >"$output" 2>&1
	status=$?
	if [ "$status" -ne 0 ]; then
		fail "$name: exit status $status"
		cat "$output" >&2
		return
	fi
	if [ ! -s "$output" ]; then
		fail "$name: empty output"
		return
	fi

	if [ "$expected" = "burnout" ]; then
		if ! tail -n 1 "$output" | awk -v deadline="$deadline" \
			'$3 == "burned" && $4 == "out" && $1 >= deadline - 10 && $1 <= deadline + 10 { found = 1 } END { exit !found }'; then
			fail "$name: burnout is not the final line within tolerance"
			cat "$output" >&2
			return
		fi
	else
		if grep -q ' burned out$' "$output"; then
			fail "$name: unexpected burnout"
			cat "$output" >&2
			return
		fi
	fi
	pass "$name"
}

check_compile_protocol() {
	local name=$1
	local output=$2

	if ! awk '
		BEGIN { valid = 1 }
		$3 == "is" && $4 == "compiling" {
			if (!prev_take1 || !prev_take2 || prev_id1 != $2 || prev_id2 != $2)
				valid = 0
		}
		{
			prev_take2 = prev_take1; prev_id2 = prev_id1
			prev_take1 = ($3 == "has" && $4 == "taken" && $5 == "a" && $6 == "dongle")
			prev_id1 = $2
		}
		END { exit !valid }
	' "$output"; then
		fail "$name: compile was not preceded by two matching dongle logs"
		return 1
	fi
	return 0
}

check_compile_counts() {
	local name=$1
	local output=$2
	local required=$3
	local coders=$4

	if ! awk -v required="$required" -v coders="$coders" '
		$3 == "is" && $4 == "compiling" { count[$2]++ }
		END {
			for (id = 1; id <= coders; id++)
				if (count[id] < required) exit 1
		}
	' "$output"; then
		fail "$name: not every coder reached $required compiles"
		return 1
	fi
	return 0
}

run_success_case() {
	local name=$1
	local required=$2
	local scheduler=$3
	local output="$TMP_DIR/${name}.log"
	local status

	timeout "$TIMEOUT_SECONDS" "$BINARY" 5 2000 200 200 200 "$required" 0 "$scheduler" >"$output" 2>&1
	status=$?
	if [ "$status" -ne 0 ]; then
		fail "$name: exit status $status"
		return
	fi
	if grep -q ' burned out$' "$output"; then
		fail "$name: unexpected burnout"
		cat "$output" >&2
		return
	fi
	if ! check_compile_protocol "$name" "$output"; then
		cat "$output" >&2
		return
	fi
	if ! check_compile_counts "$name" "$output" "$required" 5; then
		cat "$output" >&2
		return
	fi
	pass "$name"
}

cd "$ROOT_DIR" || exit 1
if ! make >/dev/null; then
	printf 'Build failed\n' >&2
	exit 1
fi

for run in $(seq 1 "$RUNS"); do
	run_case "easy-burnout-$run" burnout 800 1 800 200 200 200 10 0 fifo
	run_case "infeasible-$run" burnout 500 5 500 200 200 200 10 0 fifo
	run_success_case "easy-fifo-$run" 10 fifo
	run_success_case "easy-edf-$run" 7 edf
	done

printf '\n%d passed, %d failed\n' "$PASS_COUNT" "$FAIL_COUNT"
[ "$FAIL_COUNT" -eq 0 ]