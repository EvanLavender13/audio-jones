#!/usr/bin/env bash
# Run clang-tidy, cppcheck, and lizard, save results to tidy.log
set -euo pipefail

OUT="tidy.log"
JOBS="${1:-$(nproc 2>/dev/null || echo 4)}"
COMPILE_DB="build/compile_commands.json"
CPPCHECK_BUILD_DIR="build/cppcheck"

echo "=== Code Quality Analysis ===" | tee "$OUT"
echo "Date: $(date)" | tee -a "$OUT"
echo "" | tee -a "$OUT"

# --- Phase 1: clang-tidy (parallel via xargs) ---
echo "=== Phase 1: clang-tidy (${JOBS} threads) ===" | tee -a "$OUT"

mapfile -t SRC_FILES < <(find ./src -name '*.cpp' -type f)

printf '%s\n' "${SRC_FILES[@]}" \
  | xargs -P "$JOBS" -I {} clang-tidy.exe -p build {} 2>&1 \
  | grep -E 'AudioJones\\src\\.*warning:' \
  | tee -a "$OUT" || true

echo "" | tee -a "$OUT"

# --- Phase 2: cppcheck ---
echo "=== Phase 2: cppcheck ===" | tee -a "$OUT"

if [ ! -f "$COMPILE_DB" ]; then
  echo "ERROR: compile_commands.json not found at $COMPILE_DB" | tee -a "$OUT"
else
  mkdir -p "$CPPCHECK_BUILD_DIR"
  WIN_COMPILE_DB="$(wslpath -w "$COMPILE_DB")"
  WIN_CPPCHECK_BUILD_DIR="$(wslpath -w "$CPPCHECK_BUILD_DIR")"
  WIN_SRC_DIR="$(wslpath -w "src")"

  set +e
  cppcheck.exe \
    "--project=$WIN_COMPILE_DB" \
    "--cppcheck-build-dir=$WIN_CPPCHECK_BUILD_DIR" \
    -j "$JOBS" \
    --enable=all \
    --std=c++20 \
    "--file-filter=$WIN_SRC_DIR\\*" \
    --suppress=missingIncludeSystem \
    "--suppress=*:*\\_deps\\*" \
    --suppress=unmatchedSuppression \
    --inline-suppr \
    --check-level=exhaustive \
    -q 2>&1 | tee -a "$OUT"
  set -e
fi

echo "" | tee -a "$OUT"

# --- Phase 3: lizard complexity ---
echo "=== Phase 3: lizard complexity ===" | tee -a "$OUT"
lizard ./src -C 15 -L 75 -a 5 -w 2>&1 | tee -a "$OUT" || true

echo "" | tee -a "$OUT"
echo "=== Done. Results saved to $OUT ===" | tee -a "$OUT"
