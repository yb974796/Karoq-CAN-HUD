#include "karoq_mqb_opendbc.h"
#include <string.h>

#define FIELD(name, start, bits, factor, ofs) {name,start,bits,factor,ofs}

static const karoq_mqb_field_def_t k_esp21[] = {
    FIELD("ESP_v_Signal",32,16,0.01f,0),
    FIELD("ESP_QBit_v_Signal",55,1,1,0),
    FIELD("ESP_Haltebestaetigung",53,1,1,0),
    FIELD("ESP_Eingriff",61,1,1,0),
};
static const karoq_mqb_field_def_t k_esp19[] = {
    FIELD("ESP_HL_Radgeschw_02",0,16,0.0075f,0),
    FIELD("ESP_HR_Radgeschw_02",16,16,0.0075f,0),
    FIELD("ESP_VL_Radgeschw_02",32,16,0.0075f,0),
    FIELD("ESP_VR_Radgeschw_02",48,16,0.0075f,0),
};
static const karoq_mqb_field_def_t k_kombi[] = {
    FIELD("KBI_angez_Geschw",48,10,0.32f,0),
    FIELD("KBI_Anzeigestatus_ACC",12,1,1,0),
};
static const karoq_mqb_field_def_t k_tsk[] = {
    FIELD("TSK_Status",24,3,1,0),
    FIELD("TSK_Limiter_ausgewaehlt",31,1,1,0),
    FIELD("TSK_Hauptschalter_GRA_ACC",42,2,1,0),
};
static const karoq_mqb_field_def_t k_acc02[] = {
    FIELD("ACC_Wunschgeschw_02",12,10,0.32f,0),
    FIELD("ACC_Status_Prim_Anz",22,2,1,0),
    FIELD("ACC_Gesetzte_Zeitluecke",37,3,1,0),
    FIELD("ACC_Status_Anzeige",61,3,1,0),
};
static const karoq_mqb_field_def_t k_acc06[] = {
    FIELD("ACC_Status_ACC",60,3,1,0),
    FIELD("ACC_Typ",58,2,1,0),
};
static const karoq_mqb_field_def_t k_gra[] = {
    FIELD("GRA_Hauptschalter",12,1,1,0),
    FIELD("GRA_Tip_Setzen",16,1,1,0),
    FIELD("GRA_Tip_Wiederaufnahme",19,1,1,0),
    FIELD("GRA_Verstellung_Zeitluecke",20,2,1,0),
};
static const karoq_mqb_field_def_t k_ldw[] = {
    FIELD("LDW_Warnung_links",56,1,1,0),
    FIELD("LDW_Warnung_rechts",57,1,1,0),
    FIELD("LDW_Lernmodus_links",38,2,1,0),
    FIELD("LDW_Lernmodus_rechts",36,2,1,0),
    FIELD("LDW_Status_LED_gelb",61,1,1,0),
    FIELD("LDW_Status_LED_gruen",62,1,1,0),
};
static const karoq_mqb_field_def_t k_eps[] = {
    FIELD("EPS_HCA_Status",32,4,1,0),
    FIELD("EPS_AWA_Status",36,4,1,0),
};
static const karoq_mqb_field_def_t k_hca[] = {
    FIELD("HCA_01_Status_HCA",32,4,1,0),
    FIELD("HCA_01_Sendestatus",30,1,1,0),
};
static const karoq_mqb_field_def_t k_swa[] = {
    FIELD("SWA_Infostufe_SWA_li",26,1,1,0),
    FIELD("SWA_Warnung_SWA_li",27,1,1,0),
    FIELD("SWA_Infostufe_SWA_re",42,1,1,0),
    FIELD("SWA_Warnung_SWA_re",43,1,1,0),
};
static const karoq_mqb_field_def_t k_blink[] = {
    FIELD("Comfort_Signal_Left",23,1,1,0),
    FIELD("Comfort_Signal_Right",24,1,1,0),
};
// Hint describes opendbc use only. Actual HUD tap CAN bus must be verified.
#define MSG(id, label, net, data) { id,label,net,8,(uint8_t)(sizeof(data)/sizeof(data[0])),data }
static const karoq_mqb_msg_def_t messages[] = {
    MSG(0x0FD,"ESP_21","MQB/PT", k_esp21),
    MSG(0x0B2,"ESP_19","MQB/PT", k_esp19),
    MSG(0x30B,"Kombi_01","MQB/PT",k_kombi),
    MSG(0x120,"TSK_06","MQB/PT",k_tsk),
    MSG(0x30C,"ACC_02","MQB/EXT",k_acc02),
    MSG(0x122,"ACC_06","MQB/EXT",k_acc06),
    MSG(0x12B,"GRA_ACC_01","MQB/PT",k_gra),
    MSG(0x397,"LDW_02","MQB/CAM",k_ldw),
    MSG(0x09F,"LH_EPS_03","MQB/PT",k_eps),
    MSG(0x126,"HCA_01","MQB/CAM",k_hca),
    MSG(0x30F,"SWA_01","MQB/EXT",k_swa),
    MSG(0x366,"Blinkmodi_02","MQB/PT",k_blink),
};

bool karoq_mqb_extract_le(const uint8_t data[8], unsigned start_bit,
                           unsigned bit_length, uint32_t *out)
{
    if (!data || !out || !bit_length || bit_length>32 || start_bit>=64 ||
        start_bit+bit_length>64) return false;
    uint32_t raw=0;
    for (unsigned i=0;i<bit_length;i++) {
        unsigned p=start_bit+i;
        raw|=((uint32_t)((data[p/8]>>(p%8))&1U))<<i;
    }
    *out=raw;
    return true;
}
size_t karoq_mqb_count(void) {return sizeof(messages)/sizeof(messages[0]);}
const karoq_mqb_msg_def_t *karoq_mqb_by_index(size_t i) {
    return i<karoq_mqb_count()?&messages[i]:NULL;
}
const karoq_mqb_msg_def_t *karoq_mqb_find(uint32_t id) {
    for (size_t i=0;i<karoq_mqb_count();i++) if(messages[i].can_id==id)return &messages[i];
    return NULL;
}
bool karoq_mqb_decode(uint32_t can_id, bool extended, bool rtr, uint8_t dlc,
                      const uint8_t data[8], karoq_mqb_decoded_t *out)
{
    if (!out || !data || extended || rtr || dlc<8) return false;
    const karoq_mqb_msg_def_t *def=karoq_mqb_find(can_id);
    if (!def || def->num_fields>KAROQ_MQB_MAX_FIELDS) return false;
    memset(out,0,sizeof(*out));
    out->def=def;
    for (unsigned j=0;j<def->num_fields;j++) {
        const karoq_mqb_field_def_t *f=&def->fields[j];
        if(!karoq_mqb_extract_le(data,f->start_bit,f->bit_length,&out->raw[j]))return false;
        out->physical[j]=(float)out->raw[j]*f->scale+f->offset;
    }
    return true;
}
