/* V1.0.14.5: single owner for speed image placement.
 * This header is shared by the native V0.6 number writer and the reference
 * layout diagnostics. Geometry is constant; no speed smoothing, CAN changes,
 * or second independent LVGL position writer.
 */
#pragma once
#include <stdint.h>
#include "karoq_hud_speed_digits.h"
#define KAROQ_SPEED_SCALE_SHORT 244U
#define KAROQ_SPEED_SCALE_LONG 219U
#define KAROQ_SPEED_CENTER_SHORT_X 535
#define KAROQ_SPEED_CENTER_LONG_X 530
#define KAROQ_SPEED_TOP_SHORT 3
#define KAROQ_SPEED_TOP_LONG 9

typedef struct {
    int start_x;
    int y;
    int scaled_width;
    uint32_t scale;
} karoq_speed_geometry_t;

static inline karoq_speed_geometry_t karoq_speed_geometry(unsigned count)
{
    const bool three = count >= 3U;
    const uint32_t scale = three ? KAROQ_SPEED_SCALE_LONG : KAROQ_SPEED_SCALE_SHORT;
    const int center = three ? KAROQ_SPEED_CENTER_LONG_X : KAROQ_SPEED_CENTER_SHORT_X;
    const int width = (KAROQ_SPEED_DIGIT_W*(int)scale + 128)/256;
    const int y = three ? KAROQ_SPEED_TOP_LONG : KAROQ_SPEED_TOP_SHORT;
    const int x = center - (int)(count*width)/2;
    return (karoq_speed_geometry_t){.start_x=x,.y=y,.scaled_width=width,.scale=scale};
}
