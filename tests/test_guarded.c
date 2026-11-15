#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "lvgl.h"
#include "karoq_can_speed.h"
#include "karoq_hud_speed_bridge.h"
#include "karoq_hud_speed_guard.h"
struct _lv_obj_t {bool is_label; char text[128];lv_area_t a; lv_obj_t *children[5];unsigned count;};
struct lv_timer_t { void (*cb)(lv_timer_t*);};
const lv_obj_class_t lv_label_class={1};
static struct lv_timer_t timer;
static lv_obj_t screen, speed, sign, nav, unit;
static karoq_speed_snapshot_t snap;
static uint32_t ticks=2000;
extern void simulate_original_hud_demo(lv_obj_t *speed, lv_obj_t *nav);
static void init(void) {
 speed=(lv_obj_t){.is_label=true,.a={486,45,610,114}};strcpy(speed.text,"88");
 sign=(lv_obj_t){.is_label=true,.a={28,60,84,145}};strcpy(sign.text,"60");
 nav=(lv_obj_t){.is_label=true,.a={247,84,397,124}};strcpy(nav.text,"170");
 unit=(lv_obj_t){.is_label=true,.a={510,123,610,145}};strcpy(unit.text,"km/h");
 screen.count=4;screen.children[0]=&sign;screen.children[1]=&nav;screen.children[2]=&speed;screen.children[3]=&unit;
 snap=(karoq_speed_snapshot_t){.valid=true,.kmh=64.7f,.source=KAROQ_SPEED_ESP_0FD};
}
bool lv_obj_check_type(const lv_obj_t *o,const lv_obj_class_t *c){return o->is_label && c==&lv_label_class;}
const char *lv_label_get_text(const lv_obj_t *o){return o->text;}
void lv_label_set_text(lv_obj_t *o,const char *s){snprintf(o->text,sizeof o->text,"%s",s?s:"");}
void lv_label_set_text_static(lv_obj_t *o,const char *s){lv_label_set_text(o,s);}
void lv_label_set_text_fmt(lv_obj_t *o,const char *fmt,...){va_list args;va_start(args,fmt);vsnprintf(o->text,sizeof o->text,fmt,args);va_end(args);}
void lv_obj_get_coords(const lv_obj_t *o,lv_area_t *a){*a=o->a;}
uint32_t lv_obj_get_child_count(const lv_obj_t *o){return o->count;}
lv_obj_t *lv_obj_get_child(const lv_obj_t *o,int32_t i){return i>=0 && (unsigned)i<o->count?o->children[i]:NULL;}
lv_obj_t *lv_screen_active(void){return &screen;}
void lv_obj_update_layout(lv_obj_t *o){(void)o;}
bool lv_obj_is_valid(const lv_obj_t *o){return o==&speed || o==&sign || o==&nav || o==&unit;}
uint32_t lv_tick_get(void){return ticks;}
lv_timer_t *lv_timer_create(void(*cb)(lv_timer_t*),uint32_t per,void *p){(void)per;(void)p;timer.cb=cb;return &timer;}
void lv_timer_ready(lv_timer_t *t){t->cb(t);}
karoq_speed_snapshot_t karoq_speed_get(void){return snap;}
int main(void){
 init();
 /* Old HUD must be free to create initial "88" before bind. */
 simulate_original_hud_demo(&speed,&nav);
 assert(strcmp(speed.text,"88")==0);
 karoq_hud_speed_bridge_attach();
 assert(strcmp(speed.text,"65")==0);
 simulate_original_hud_demo(&speed,&nav);
 assert(strcmp(speed.text,"65")==0);
 assert(strcmp(nav.text,"NEXT TURN 7")==0);
 assert(karoq_hud_speed_guard_blocked_total()==3);
 snap.kmh=5.1f;ticks=2200;lv_timer_ready(&timer);
 assert(strcmp(speed.text,"5")==0);
 simulate_original_hud_demo(&speed,&nav);
 assert(strcmp(speed.text,"5")==0);
 snap.valid=false;ticks=2300;lv_timer_ready(&timer);
 assert(strcmp(speed.text,"--")==0);
 puts("PASS V1.0.2 transitional guard: ignored 3 original-demo label writes, other UI untouched, 0x0FD speed and stale mode OK");
 return 0;
}
