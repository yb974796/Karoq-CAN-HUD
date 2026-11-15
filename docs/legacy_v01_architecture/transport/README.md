Transport adapters are intentionally not implemented in this portable architecture pack.
Planned adapters:
- waveshare_v2_board: use original Waveshare V2 ESP-IDF LVGL v9.3 board demo without altering pins
- twai_rx: TWAI read-only controller + TJA1051T/3 S=VIO (never transmit to vehicle)
- sd_logger: append raw CAN frames and timestamps + overflow metadata
- bap_transport: reassemble 29-bit frames (not the same as plain DBC signals)
- ios_ble_gatt: accept only explicitly defined navigation messages from a companion app
- wifi_config: on-device settings portal; auth required, do not expose vehicle data openly
