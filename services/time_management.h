#ifndef TIME_MANAGEMENT_H
#define TIME_MANAGEMENT_H

#include <stdint.h>
#include "services.h"

#define MAX_YEAR 21

struct time_keeping {
    uint32_t epoch;
    uint32_t elapsed;
    struct time_utc utc;

};

extern void HAL_sys_setTime(uint8_t hours, uint8_t mins, uint8_t sec);
extern void HAL_sys_getTime(uint8_t *hours, uint8_t *mins, uint8_t *sec);

extern void HAL_sys_setDate(uint8_t mon, uint8_t date, uint8_t year);
extern void HAL_sys_getDate(uint8_t *mon, uint8_t *date, uint8_t *year);

//extern SAT_returnState crt_pkt(tc_tm_pkt *pkt, TC_TM_app_id app_id, uint8_t type, uint8_t ack, uint8_t ser_type, uint8_t ser_subtype, TC_TM_app_id dest_id);
extern SAT_returnState crt_pkt(tc_tm_pkt *pkt, TC_TM_app_id app_id, uint8_t type, uint8_t ack, uint8_t ser_type, uint8_t ser_subtype, TC_TM_app_id dest_id);
extern uint32_t HAL_sys_GetTick();

//ToDo
//  Set assertions everywhere

/*calculate uint size, and perform calculations*/
extern const uint32_t UTC_QB50_YM[MAX_YEAR][13];

extern const uint32_t UTC_QB50_D[32];

extern const uint32_t UTC_QB50_H[25];

void cnv_UTC_QB50(struct time_utc utc, uint32_t *qb);

void set_time_QB50(uint32_t qb);

void set_time_UTC(struct time_utc utc);

void get_time_QB50(uint32_t *qb);

void get_time_UTC(struct time_utc *utc);

uint32_t get_time_ELAPSED();

uint32_t time_cmp_elapsed(uint32_t t1, uint32_t t2);

SAT_returnState time_managment_app( tc_tm_pkt *pck );

SAT_returnState time_management_report_in_utc(tc_tm_pkt **pkt, TC_TM_app_id dest_id);

SAT_returnState time_management_report_in_qb50(tc_tm_pkt **pkt, TC_TM_app_id dest_id);
        
#endif
