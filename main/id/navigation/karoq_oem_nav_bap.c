#include "karoq_oem_nav_bap.h"
#include <string.h>

void karoq_oem_bap_init(karoq_oem_bap_t *ctx)
{
    if (ctx) memset(ctx,0,sizeof(*ctx));
}

static bool finish(karoq_oem_bap_t *ctx,const uint8_t *p,uint16_t n,
                   karoq_oem_bap_message_t *out)
{
    if (n<2U || n> KAROQ_OEM_NAV_BAP_APP_MAX+2U) {
        ctx->bad_length++;return false;
    }
    const uint16_t head=((uint16_t)p[0]<<8)|p[1];
    const uint8_t lsg=(uint8_t)((head>>6)&0x3fU);
    if (lsg!=0x32U) return false;
    out->opcode=(uint8_t)((head>>12)&0x7U);
    out->lsg=lsg;
    out->function_id=(uint8_t)(head&0x3fU);
    out->payload_len=n-2U;
    if (out->payload_len) memcpy(out->payload,p+2,out->payload_len);
    ctx->completed++;
    return true;
}

bool karoq_oem_bap_feed(karoq_oem_bap_t *ctx,uint32_t can_id,
                        const uint8_t *data,uint8_t dlc,uint32_t now_ms,
                        karoq_oem_bap_message_t *out)
{
    if (!ctx || !data || !out || dlc==0U || dlc>8U) return false;
    uint8_t id_index;
    if (can_id==0x17333210UL) id_index=0U;
    else if (can_id==0x17333211UL) id_index=1U;
    else return false;

    const uint8_t h=data[0];
    if ((h&0x80U)==0U) {
        return finish(ctx,data,dlc,out);
    }
    const uint8_t channel=(h>>4)&3U;
    karoq_oem_bap_stream_t *s=&ctx->stream[id_index][channel];
    if ((h&0xC0U)==0x80U) { /* first frame: 12-bit APP length */
        s->active=false;
        if (dlc<4U) {ctx->bad_length++;return false;}
        const uint16_t app_len=(uint16_t)(((uint16_t)(h&0x0FU)<<8)|data[1]);
        if (app_len>KAROQ_OEM_NAV_BAP_APP_MAX) {
            ctx->bad_length++;return false;
        }
        s->expected=app_len+2U;
        s->count=dlc-2U;
        if (s->count>s->expected) {ctx->bad_length++;return false;}
        memcpy(s->bytes,data+2,s->count);
        s->next_seq=0U;
        s->last_ms=now_ms;
        s->active=true;
        if (s->count==s->expected) {
            s->active=false;
            return finish(ctx,s->bytes,s->count,out);
        }
        return false;
    }
    if (!s->active) return false;
    if ((uint32_t)(now_ms-s->last_ms)>KAROQ_OEM_NAV_BAP_TIMEOUT_MS) {
        s->active=false;ctx->expired++;return false;
    }
    if ((h&0x0FU)!=s->next_seq) {
        s->active=false;ctx->bad_sequence++;return false;
    }
    if ((uint16_t)(s->count+dlc-1U)>s->expected) {
        s->active=false;ctx->bad_length++;return false;
    }
    if (dlc>1U) memcpy(s->bytes+s->count,data+1,dlc-1U);
    s->count+=(uint16_t)(dlc-1U);
    s->next_seq=(uint8_t)((s->next_seq+1U)&0x0FU);
    s->last_ms=now_ms;
    if (s->count!=s->expected) return false;
    s->active=false;
    return finish(ctx,s->bytes,s->count,out);
}
