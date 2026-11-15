# Karoq HUD V1.0.8 — explicit sources, no competing speed/ADAS writers.
# Script Mode compatible (ESP-IDF v5.5.3).
idf_component_register(
    SRCS
        "core/main.c"
        "id/can/karoq_can_sniffer.c"
        "id/decoders/karoq_can_speed.c"
        "id/decoders/karoq_can_acc_probe.c"
        "id/signals/karoq_mqb_opendbc.c"
        "id/signals/karoq_mqb_assist_state.c"
        "other/diagnostics/karoq_mqb_assist_runtime.c"
        "other/diagnostics/karoq_mqb_openpilot_probe.c"
        "other/storage/karoq_sd_logger.c"
        "other/storage/karoq_id_tracker.c"
        "other/buttons/karoq_record_button.c"
        "ui/v0_6/karoq_hud.c"
        "ui/v0_6/karoq_hud_assets.c"
        "ui/v0_6/karoq_hud_speed_digits.c"
        "ui/bridge/karoq_v06_live_model.c"
        "ui/bridge/karoq_v06_live_bridge.c"
        "ui/layout/karoq_reference_assets.c"
        "ui/layout/karoq_reference_layout.c"
        "ui/layout/karoq_nav_clean_assets.c"
        "ui/layout/karoq_acc_dual_palette.c"
        "ui/layout/karoq_acc_green_palette.c"
        "ui/layout/karoq_large_sign_assets.c"
        "id/navigation/karoq_oem_nav_bap.c"
        "id/navigation/karoq_oem_nav_decoder.c"
        "id/signals/karoq_ui_acc_set.c"
        "id/signals/karoq_vze_state.c"
        "id/traffic_sign/karoq_ts_bap.c"
        "id/traffic_sign/karoq_ts_vza_decoder.c"
        "id/can/karoq_can_dispatcher.c"
        "other/traffic_sign/karoq_ts_service.c"
        "other/traffic_sign/karoq_ts_carplay_adapter.c"
        "other/navigation/karoq_nav_service.c"
        "other/navigation/karoq_nav_wire.c"
        "other/navigation/karoq_nav_carplay_adapter.c"
    INCLUDE_DIRS "id/navigation" "ui/layout" "id/signals" "other/navigation" "id/traffic_sign" "other/traffic_sign" "id/can"
        "ui/v0_6"
        "ui/bridge"
        "id/signals"
        "other/diagnostics"
        "id/can"
        "id/decoders"
        "other/storage"
        "other/buttons"
        "core"
        "."
    PRIV_REQUIRES
        esp_driver_twai
        esp_driver_gpio
        esp_driver_sdmmc
        esp_driver_spi
        esp_driver_i2c
        sdmmc
        fatfs
        esp_timer
        i2c_bsp
        lcd_bl_pwm_bsp
        esp_lcd
        lvgl
        espressif__esp_lcd_axs15231b
        espressif__esp_io_expander_tca9554
        espressif__esp_lcd_touch
)

if(NOT CMAKE_SCRIPT_MODE_FILE)
    # Original V0.6 demo timer is compiled out. Do not alter the artwork.
    target_compile_definitions(${COMPONENT_LIB} PRIVATE KAROQ_HUD_DEMO_AUTORUN=0)
endif()
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/id/types")
