from pathlib import Path
from tempfile import TemporaryDirectory
import sys,shutil,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from upgrade_v105 import update
from verify_v105 import check

addon=Path(__file__).resolve().parents[1]
with TemporaryDirectory() as t:
    root=Path(t);src=root/'v104';dst=root/'v105'
    for rel in ['core','id/can','id/decoders','ui/v0_6','other/storage','other/buttons','ui/bridge']:
        (src/'main'/rel).mkdir(parents=True,exist_ok=True)
    (src/'CMakeLists.txt').write_text('project(karoq_can_hud_v1_0_4)\n')
    (src/'main/CMakeLists.txt').write_text('''idf_component_register(
 SRCS "id/can/karoq_can_sniffer.c" "other/storage/karoq_sd_logger.c"
 INCLUDE_DIRS "ui/v0_6"
 PRIV_REQUIRES esp_driver_twai)
''')
    (src/'main/core/main.c').write_text('''#include "esp_log.h"
void app_main(void){
 karoq_sd_logger_init();
 karoq_can_sniffer_start();
}
''')
    (src/'main/id/can/karoq_can_sniffer.c').write_text('''#include "karoq_can_sniffer.h"
void can_rx_task(void *arg){
    if (rc == ESP_OK) {
       karoq_sd_logger_receive(&msg,0);
       karoq_speed_ingest(&frame);
    }
}
''')
    for rel in ['id/decoders/karoq_can_speed.c','id/decoders/karoq_can_acc_probe.c',
                'other/storage/karoq_sd_logger.c','other/buttons/karoq_record_button.c']:
        (src/'main'/rel).write_text('/* original user source, do not edit */\n')
    for file in ['karoq_hud.c','karoq_hud_assets.c','custom_icon.bin']:
        (src/'main/ui/v0_6'/file).write_bytes(b'original-V0.6\x00\x01'+file.encode())
    original={p.relative_to(src).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
               for p in src.rglob('*') if p.is_file()}
    update(src,dst,addon)
    check(dst)
    for rel,digest in original.items():
        assert hashlib.sha256((src/rel).read_bytes()).hexdigest()==digest,'source changed'
    assert not (dst/'build').exists()
    try:
        update(src,dst,addon)
        raise AssertionError('expected refusal to overwrite')
    except RuntimeError as e:
        assert '目標目錄已存在' in str(e)
    print('PASS V1.0.5 migration source unchanged, V0.6 UI SHA256 unchanged, no duplicate hooks')
