#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
executable="$(mktemp /tmp/karoq_v104_tracker_XXXXXX)"
trap 'rm -f "$executable"' EXIT
cc -std=c11 -Wall -Wextra -Werror \
   -I"$root/main/other/storage" \
   "$root/main/other/storage/karoq_id_tracker.c" \
   "$root/tests/test_id_tracker.c" -o "$executable"
"$executable"
python3 "$root/tests/test_compare.py"
python3 "$root/tests/test_upgrade.py"
# C syntax-level test with minimal ESP-IDF API stubs (NOT a true ESP-IDF build).
cc -std=c11 -Wall -Wextra -Werror -fsyntax-only \
   -I"$root/tests/idf_stubs" \
   -I"$root/main/other/storage" -I"$root/main/other/buttons" \
   "$root/main/other/storage/karoq_sd_logger.c" \
   "$root/main/other/storage/karoq_id_tracker.c" \
   "$root/main/other/buttons/karoq_record_button.c"
echo 'PASS: V1.0.4 logger/button syntax under ESP-IDF API stubs'
