#pragma once
#include <stdint.h>
typedef struct { uint32_t identifier; uint8_t extd,rtr,data_length_code,data[8];} twai_message_t;
