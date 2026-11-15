from pathlib import Path
import tempfile,shutil,sys
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/'tools'))
from upgrade_v104 import patch
from verify_v104 import check
addon=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory() as t:
    root=Path(t)
    src=root/'v103';dst=root/'v104'
    (src/'main/core').mkdir(parents=True)
    (src/'main/id/can').mkdir(parents=True)
    (src/'main/ui/v0_6').mkdir(parents=True)
    (src/'main/ui/bridge').mkdir(parents=True)
    for file in ('other/storage/karoq_sd_logger.c','other/storage/karoq_sd_logger.h',
                 'other/storage/karoq_id_tracker.c','other/storage/karoq_id_tracker.h',
                 'other/buttons/karoq_record_button.c','other/buttons/karoq_record_button.h'):
        p=src/'main'/file;p.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(addon/'main'/file,p)
    (src/'main/core/main.c').write_text('void app_main(void){karoq_sd_logger_init(); karoq_can_sniffer_start();}\n')
    (src/'main/id/can/karoq_can_sniffer.c').write_text('void task(void){karoq_sd_logger_receive(&frame,0);}\n')
    (src/'main/ui/v0_6/karoq_hud.c').write_text('/* V0.6 original UI fixture */\n')
    (src/'main/ui/v0_6/karoq_hud_assets.c').write_text('/* V0.6 original assets fixture */\n')
    (src/'main/ui/v0_6/custom_icon.bin').write_bytes(b'0x4b41524f51-unaltered')
    (src/'main/ui/bridge/karoq_hud_speed_bridge.c').write_text('/* speed bridge fixture */\n')
    (src/'CMakeLists.txt').write_text('project(karoq_can_hud_v1_0_3)\n')
    (src/'main/CMakeLists.txt').write_text('''idf_component_register(SRCS
  "other/storage/karoq_sd_logger.c" "other/storage/karoq_id_tracker.c"
  "other/buttons/karoq_record_button.c" "id/can/karoq_can_sniffer.c"
  INCLUDE_DIRS "ui/v0_6"
  PRIV_REQUIRES fatfs esp_driver_sdmmc sdmmc)
''')
    ui_before=(src/'main/ui/v0_6/karoq_hud.c').read_bytes()
    patch(src,dst,addon)
    check(dst)
    assert (src/'main/ui/v0_6/karoq_hud.c').read_bytes()==ui_before
    assert (dst/'main/ui/v0_6/karoq_hud.c').read_bytes()==ui_before
    assert 'karoq_sd_logger_init' in (dst/'main/core/main.c').read_text()
    assert 'karoq_sd_logger_receive' in (dst/'main/id/can/karoq_can_sniffer.c').read_text()
    assert not (dst/'build').exists()
    try:
        patch(src,dst,addon)
    except RuntimeError as e:
        assert 'Destination exists' in str(e)
    else:raise AssertionError('Cannot overwrite existing output')
    print('PASS: independent upgrade, UI bytes preserved, source unchanged, overwrite refused')
