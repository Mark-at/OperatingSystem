#!/bin/bash

# ─── Config ───────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/project/src"
TC_DIR="$SCRIPT_DIR/test-cases"
BINARY="$SRC_DIR/mysh"

# ─── Colors ───────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ─── Build ────────────────────────────────────────────────────────────────────
echo "=========================================="
echo " Building..."
echo "=========================================="
cd "$SRC_DIR" && make clean && make mysh
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed. Exiting.${NC}"
    exit 1
fi
echo ""

# ─── Run Tests ────────────────────────────────────────────────────────────────
PASS=0
FAIL=0
SKIP=0
FAILED_TESTS=()

echo "=========================================="
echo " Running test cases..."
echo "=========================================="

for test_file in "$TC_DIR"/T_*.txt; do
    # Skip result files
    [[ "$test_file" == *_result* ]] && continue

    test_name=$(basename "$test_file" .txt)

    # Find result file(s): prefer _result.txt, also check _result2.txt
    result1="$TC_DIR/${test_name}_result.txt"
    result2="$TC_DIR/${test_name}_result2.txt"

    if [ ! -f "$result1" ] && [ ! -f "$result2" ]; then
        echo -e "${YELLOW}SKIP${NC}  $test_name  (no result file found)"
        ((SKIP++))
        continue
    fi

    # Run the test (with a timeout to avoid hangs)
    actual=$(cd "$TC_DIR" && timeout 10s "$BINARY" < "$test_file" 2>/dev/null)

    if [ $? -eq 124 ]; then
        echo -e "${RED}TIMEOUT${NC} $test_name"
        ((FAIL++))
        FAILED_TESTS+=("$test_name (TIMEOUT)")
        continue
    fi

    # Normalize: lowercase + collapse whitespace
    normalize() {
        echo "$1" | tr '[:upper:]' '[:lower:]' | tr -s ' \t\n' ' ' | sed 's/^ //;s/ $//'
    }

    actual_norm=$(normalize "$actual")

    # Check against result1 and optionally result2
    matched=false

    if [ -f "$result1" ]; then
        expected1=$(cat "$result1")
        expected1_norm=$(normalize "$expected1")
        [ "$actual_norm" = "$expected1_norm" ] && matched=true
    fi

    if [ "$matched" = false ] && [ -f "$result2" ]; then
        expected2=$(cat "$result2")
        expected2_norm=$(normalize "$expected2")
        [ "$actual_norm" = "$expected2_norm" ] && matched=true
    fi

    if [ "$matched" = true ]; then
        echo -e "${GREEN}PASS${NC}  $test_name"
        ((PASS++))
    else
        echo -e "${RED}FAIL${NC}  $test_name"
        ((FAIL++))
        FAILED_TESTS+=("$test_name")

        # Show diff against result1 for debugging
        if [ -f "$result1" ]; then
            echo "  --- expected (result1) vs actual ---"
            diff <(echo "$expected1") <(echo "$actual") | head -20
        fi
        echo ""
    fi
done

# ─── Summary ──────────────────────────────────────────────────────────────────
echo ""
echo "=========================================="
echo " Results: ${PASS} passed, ${FAIL} failed, ${SKIP} skipped"
echo "=========================================="

if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
    echo -e "${RED}Failed tests:${NC}"
    for t in "${FAILED_TESTS[@]}"; do
        echo "  - $t"
    done
fi

echo ""
[ $FAIL -eq 0 ] && exit 0 || exit 1
