#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cc="${CC:-gcc}"
"$cc" -std=c11 -Wall -Wextra -Werror -Imain/id/signals \
 main/id/signals/karoq_mqb_opendbc.c main/id/signals/karoq_mqb_assist_state.c \
 tests/test_adas_link.c -o /tmp/karoq_v106_adas
/tmp/karoq_v106_adas
"$cc" -std=c11 -Wall -Wextra -Werror -Itests/stubs -Imain/id/signals \
 -Imain/other/diagnostics -fsyntax-only main/other/diagnostics/karoq_mqb_assist_runtime.c
"$cc" -std=c11 -Wall -Wextra -Werror -Itests/stubs -Imain/id/signals \
 -Imain/other/diagnostics -Imain/ui/bridge -fsyntax-only main/ui/bridge/karoq_v06_adas_ui_bridge.c
printf 'PASS mock ESP-IDF/runtime and LVGL9 bridge syntax\n'
python tests/test_upgrade_v106.py
