#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
CC="${CC:-gcc}"
"$CC" -std=c11 -Wall -Wextra -Werror -Imain/id/signals \
 main/id/signals/karoq_mqb_opendbc.c tests/test_opendbc_mqb.c -lm -o /tmp/karoq_mqb_v105_10run
/tmp/karoq_mqb_v105_10run
"$CC" -std=c11 -Wall -Wextra -Werror -Itests/stubs \
 -Imain/id/signals -Imain/other/diagnostics -fsyntax-only \
 main/other/diagnostics/karoq_mqb_openpilot_probe.c
python tests/test_upgrade_v105.py
python tools/analyze_mqb_can.py tests/sample_new_ids.log > /tmp/karoq_v105_observed.txt
cat /tmp/karoq_v105_observed.txt
