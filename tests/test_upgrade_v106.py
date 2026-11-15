from pathlib import Path
from tempfile import TemporaryDirectory
import hashlib,shutil,sys,json
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from upgrade_v106 import run
pkg=Path(__file__).resolve().parents[1]

def setup(root,autobind):
    main=root/'main'
    for sub in ('core','id/can','id/signals','id/decoders','other/diagnostics',
                'other/storage','other/buttons','ui/v0_6','ui/bridge'):
        (main/sub).mkdir(parents=True,exist_ok=True)
    (root/'CMakeLists.txt').write_text('project(karoq_can_hud_v1_0_5)\n')
    (main/'CMakeLists.txt').write_text('''idf_component_register(SRCS
 "id/can/karoq_can_sniffer.c" "id/signals/karoq_mqb_opendbc.c"
 "other/diagnostics/karoq_mqb_openpilot_probe.c" "other/storage/karoq_sd_logger.c"
 INCLUDE_DIRS "ui/v0_6")
''')
    (main/'core/main.c').write_text('''#include "esp_log.h"
void app_main(void){
  esp_err_t mqb_diag_status = karoq_mqb_openpilot_probe_init();
  karoq_can_sniffer_start();
  if(example_lvgl_lock(-1)){
     karoq_hud_ui_init();
     karoq_hud_speed_bridge_attach();
     example_lvgl_unlock();
  }
}
''')
    (main/'id/can/karoq_can_sniffer.c').write_text('''#include "karoq_mqb_openpilot_probe.h"
void can_rx_task(void *arg){
    karoq_speed_ingest(&frame);
    karoq_mqb_openpilot_probe_ingest(&msg);
    karoq_sd_logger_receive(&msg, now_ms);
}
''')
    for rel in ('id/signals/karoq_mqb_opendbc.c','id/signals/karoq_mqb_opendbc.h',
                'other/diagnostics/karoq_mqb_openpilot_probe.c'):
        shutil.copy2(pkg/'main'/rel,main/rel)
    for rel in ('id/decoders/karoq_can_speed.c','id/decoders/karoq_can_acc_probe.c',
                'other/storage/karoq_sd_logger.c','other/buttons/karoq_record_button.c'):
        (main/rel).write_text('original stable source\n')
    if autobind:
        (main/'ui/v0_6/karoq_hud.c').write_text('''#include "lvgl.h"
extern const lv_image_dsc_t assist_off, assist_acc, assist_lane, assist_travel;
void karoq_hud_ui_init(void) {
 lv_obj_t *ui_assist = lv_image_create(lv_screen_active());
 lv_image_set_src(ui_assist,&assist_off);
}
''')
    else:
        (main/'ui/v0_6/karoq_hud.c').write_text('''/* V0.6 vector UI using custom primitives (not images). */
void karoq_hud_ui_init(void) {}
''')
    (main/'ui/v0_6/karoq_hud_assets.c').write_text('private image data unchanged\n')
    (main/'ui/v0_6/icon.bin').write_bytes(b'original iconic data\0\x01')

with TemporaryDirectory() as t:
    root=Path(t)
    for auto in (False,True):
        src=root/('src_auto' if auto else 'src_unknown')
        dst=root/('dst_auto' if auto else 'dst_unknown')
        setup(src,auto)
        hashes={p.relative_to(src).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in src.rglob('*') if p.is_file()}
        run(src,dst,pkg)
        assert all(hashlib.sha256((src/k).read_bytes()).hexdigest()==v for k,v in hashes.items()),'source mutated'
        status=json.loads((dst/'docs/V106_UI_SHA256.json').read_text())
        assert status['auto_bound']==auto
        assert 'karoq_mqb_assist_runtime_ingest(&msg);' in (dst/'main/id/can/karoq_can_sniffer.c').read_text()
        assert (dst/'main/core/main.c').read_text().count('karoq_v06_adas_ui_attach();')==1
        assert 'ESP_ERROR_CHECK(karoq_mqb_assist_runtime_init());' in (dst/'main/core/main.c').read_text()
        if auto:
            code=(dst/'main/ui/v0_6/karoq_hud.c').read_text()
            assert 'karoq_v06_adas_bind_image(ui_assist, &assist_off, &assist_acc, &assist_lane, &assist_travel);' in code
        else:
            assert status['original']==status['final']
        try:
            run(src,dst,pkg)
            raise AssertionError('should not overwrite destination')
        except RuntimeError as e:
            assert '目標目錄已存在' in str(e)
        print('PASS fixture migration V1.0.5 → V1.0.6', 'UI AUTO BOUND' if auto else 'UI fail-closed UNBOUND')
