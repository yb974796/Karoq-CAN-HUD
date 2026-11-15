#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct _lv_obj_t lv_obj_t;
typedef struct _lv_timer_t lv_timer_t;
int lv_obj_is_valid(lv_obj_t *o);
void lv_image_set_src(lv_obj_t *image,const void *src);
lv_timer_t *lv_timer_create(void (*)(lv_timer_t*),uint32_t,void*);
void lv_timer_ready(lv_timer_t *timer);
