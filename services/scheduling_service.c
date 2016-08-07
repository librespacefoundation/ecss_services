/* 
 * File:   scheduling_service.c
 * Author: 
 *
 * Created on March 8, 2016, 9:05 PM
 * This is the implementation of scheduling service as is
 * documented at pages 99-118 of ECSS-E-70-41A document.
 * Service Type 11 
 * (some restrictions apply)
 */

#undef __FILE_ID__
#define __FILE_ID__ 6

#include "scheduling_service.h"

extern void wdg_reset_SCH();
extern SAT_returnState mass_storage_schedule_load_api(MS_sid sid, uint32_t sch_number, uint8_t *buf);
extern SAT_returnState mass_storage_storeFile(MS_sid sid, uint32_t file, uint8_t *buf, uint16_t *size);
extern SAT_returnState route_pkt(tc_tm_pkt *pkt);
extern uint32_t HAL_GetTick(void);
SAT_returnState handle_sch_reporting(uint8_t *tc_tm_data);

struct time_keeping obc_time;

SAT_returnState copy_inner_tc(const uint8_t *buf, tc_tm_pkt *pkt, const uint16_t size);
SC_pkt* find_schedule_pos();
Scheduling_service_state sc_s_state;
Schedule_pkt_pool schedule_mem_pool;

/**
 * Initializes the scheduling service.
 * 
 * @return the execution state.
 */
SAT_returnState scheduling_service_init(){
    
    /* Initialize schedules memory.
     * Assign proper memory allocation to inner TC of ScheduleTC for its data payload.
     */
    for(uint8_t s=0;s<SC_MAX_STORED_SCHEDULES;s++){
        schedule_mem_pool.sc_mem_array[s].tc_pck.data 
                         = schedule_mem_pool.innerd_tc_data[s];
        
        /* Marks every schedule as invalid, so its position
         * can be taken by a request to the Schedule packet pool.
         */
        schedule_mem_pool.sc_mem_array[s].pos_taken = false;
    }
        
    sc_s_state.nmbr_of_ld_sched = 0;
    sc_s_state.schedule_arr_full = false;
    
    /*Enable scheduling release for every APID*/
    for(uint8_t s=0;s<LAST_APP_ID;s++){
        sc_s_state.scheduling_apids_enabled[s] = true;
    }
    
    /* Load Schedules from storage.
     * 
     */
    
    return SATR_OK;
}

/**
 *  Loads the schedules from persistent storage. 
 * @return the execution state.
 */
SAT_returnState scheduling_service_load_schedules(){

//    SC_pkt *temp_pkt;
    uint8_t sche_tc_buffer[30];
    memset(sche_tc_buffer,0x00,30);
    for(uint8_t s=0;s<SC_MAX_STORED_SCHEDULES;s++){
//        temp_pkt = find_schedule_pos();
        mass_storage_schedule_load_api(SCHS, s, sche_tc_buffer);
        
        /*save the tc's data length in the first 2 bytes*/
        cnv8_16LE(sche_tc_buffer, &schedule_mem_pool.sc_mem_array[s].tc_pck.len);
        
        /*start loading the sch packet*/
        schedule_mem_pool.sc_mem_array[s].app_id = (TC_TM_app_id)sche_tc_buffer[2];
        cnv8_16LE(&sche_tc_buffer[3], &schedule_mem_pool.sc_mem_array[s].seq_count);
        schedule_mem_pool.sc_mem_array[s].enabled = sche_tc_buffer[5];
        schedule_mem_pool.sc_mem_array[s].sub_schedule_id = sche_tc_buffer[6];
        schedule_mem_pool.sc_mem_array[s].num_of_sch_tc = sche_tc_buffer[7];
        schedule_mem_pool.sc_mem_array[s].intrlck_set_id = sche_tc_buffer[8];
        schedule_mem_pool.sc_mem_array[s].intrlck_ass_id = sche_tc_buffer[9];
        schedule_mem_pool.sc_mem_array[s].assmnt_type = sche_tc_buffer[10];
        schedule_mem_pool.sc_mem_array[s].sch_evt = (SC_event_time_type) sche_tc_buffer[11];
        cnv8_32(&sche_tc_buffer[12], &schedule_mem_pool.sc_mem_array[s].release_time);
        cnv8_16LE(&sche_tc_buffer[16], &schedule_mem_pool.sc_mem_array[s].timeout);
        
        /*TC parsing begins here*/
        schedule_mem_pool.sc_mem_array[s].tc_pck.app_id = (TC_TM_app_id) sche_tc_buffer[18];
        schedule_mem_pool.sc_mem_array[s].tc_pck.type = sche_tc_buffer[19];
        schedule_mem_pool.sc_mem_array[s].tc_pck.seq_flags = sche_tc_buffer[20];
        cnv8_16LE(&sche_tc_buffer[21], &schedule_mem_pool.sc_mem_array[s].tc_pck.seq_count);
        cnv8_16LE(&sche_tc_buffer[23], &schedule_mem_pool.sc_mem_array[s].tc_pck.len);
        schedule_mem_pool.sc_mem_array[s].tc_pck.ack = sche_tc_buffer[25];
        schedule_mem_pool.sc_mem_array[s].tc_pck.ser_type = sche_tc_buffer[26];
        schedule_mem_pool.sc_mem_array[s].tc_pck.ser_subtype = sche_tc_buffer[27];
        schedule_mem_pool.sc_mem_array[s].tc_pck.dest_id = (TC_TM_app_id) sche_tc_buffer[28];
        
        /*copy tc payload data*/
        uint16_t i = 0;
        for(;i<schedule_mem_pool.sc_mem_array[s].tc_pck.len;i++){
            schedule_mem_pool.sc_mem_array[s].tc_pck.data[i] = sche_tc_buffer[29+i];
        }
        schedule_mem_pool.sc_mem_array[s].tc_pck.verification_state = (SAT_returnState) sche_tc_buffer[29+i];
        schedule_mem_pool.sc_mem_array[s].pos_taken = true;

        memset(sche_tc_buffer,0x00,30);
    }
    
    return SATR_OK;
}

/**
 * Saves current active schedules on permanent storage.
 * @return the execution state.
 */
SAT_returnState scheduling_service_save_schedules(){
    
    uint8_t sche_tc_buffer[30]; //TODO redefine this
    
    /*convert the Schedule packet from Schedule_pkt_pool format to an array of linear bytes*/        
    for(uint8_t s=0;s<SC_MAX_STORED_SCHEDULES;s++){
        
        uint16_t file_size=0;                
        /*save the tc's data length in the first 2 bytes*/
        cnv16_8(schedule_mem_pool.sc_mem_array[s].tc_pck.len, sche_tc_buffer);
        
        /*start saving sch packet*/
        sche_tc_buffer[2] = (uint8_t)schedule_mem_pool.sc_mem_array[s].app_id;
        cnv16_8(schedule_mem_pool.sc_mem_array[s].seq_count, &sche_tc_buffer[3]);
        sche_tc_buffer[5] = schedule_mem_pool.sc_mem_array[s].enabled;
        sche_tc_buffer[6] = schedule_mem_pool.sc_mem_array[s].sub_schedule_id;
        sche_tc_buffer[7] = schedule_mem_pool.sc_mem_array[s].num_of_sch_tc;
        sche_tc_buffer[8] = schedule_mem_pool.sc_mem_array[s].intrlck_set_id;
        sche_tc_buffer[9] = schedule_mem_pool.sc_mem_array[s].intrlck_ass_id;
        sche_tc_buffer[10] = schedule_mem_pool.sc_mem_array[s].assmnt_type;
        sche_tc_buffer[11] = (uint8_t)schedule_mem_pool.sc_mem_array[s].sch_evt;
        cnv32_8(schedule_mem_pool.sc_mem_array[s].release_time,&sche_tc_buffer[12]);
        cnv16_8(schedule_mem_pool.sc_mem_array[s].timeout,&sche_tc_buffer[16]);
        
        /*TC parsing begins here*/
        sche_tc_buffer[18] = (uint8_t)schedule_mem_pool.sc_mem_array[s].tc_pck.app_id;
        sche_tc_buffer[19] = schedule_mem_pool.sc_mem_array[s].tc_pck.type;
        sche_tc_buffer[20] = schedule_mem_pool.sc_mem_array[s].tc_pck.seq_flags;
        cnv16_8(schedule_mem_pool.sc_mem_array[s].tc_pck.seq_count,&sche_tc_buffer[21]);
        cnv16_8(schedule_mem_pool.sc_mem_array[s].tc_pck.len,&sche_tc_buffer[23]);
        sche_tc_buffer[25] = schedule_mem_pool.sc_mem_array[s].tc_pck.ack;
        sche_tc_buffer[26] = schedule_mem_pool.sc_mem_array[s].tc_pck.ser_type;
        sche_tc_buffer[27] = schedule_mem_pool.sc_mem_array[s].tc_pck.ser_subtype;
        sche_tc_buffer[28] = (uint8_t)schedule_mem_pool.sc_mem_array[s].tc_pck.dest_id;
        
        /*copy tc payload data*/
        uint16_t i = 0;
        for(;i<schedule_mem_pool.sc_mem_array[s].tc_pck.len;i++){
            sche_tc_buffer[29+i] = schedule_mem_pool.sc_mem_array[s].tc_pck.data[i];
        }
        sche_tc_buffer[29+i] = (uint8_t)schedule_mem_pool.sc_mem_array[s].tc_pck.verification_state;
        file_size = 30+i;
        mass_storage_storeFile(SCHS,s,sche_tc_buffer,&file_size);
        //TODO messet the sche_tc_buffer
        
    }
    
    return SATR_OK;
}

/* Cross schedules array, 
 * in every pass check if the specific schedule 
 * if enabled,
 *  if it is then check if its relative or absolute and check the time.
 *  if time >= release time, then execute it. (?? what if time has passed?)
 * else !enabled
 *  if time>= release time, then mark it as !valid
 */
void cross_schedules() {
    
    for (uint8_t i = 0; i < SC_MAX_STORED_SCHEDULES; i++) {
        if (schedule_mem_pool.sc_mem_array[i].pos_taken == true &&
                sc_s_state.scheduling_apids_enabled[(schedule_mem_pool.sc_mem_array[i].app_id) - 1] == true){

            switch( schedule_mem_pool.sc_mem_array[i].sch_evt){
                case 0: /*ABSOLUTE*/
                    uint32_t boot_secs = HAL_GetTick();
                    if( schedule_mem_pool.sc_mem_array[i].release_time <= (boot_secs / 1000)){
                        route_pkt(&(schedule_mem_pool.sc_mem_array[i].tc_pck));
                        schedule_mem_pool.sc_mem_array[i].pos_taken = false;
                        sc_s_state.nmbr_of_ld_sched--;
                        sc_s_state.schedule_arr_full = false;
                    }
                    break;
                case 4: /*QB50EPOCH*/
                    uint32_t qb_temp_secs = 0;
                    get_time_QB50(&qb_temp_secs);
                    if( schedule_mem_pool.sc_mem_array[i].release_time <= qb_temp_secs) {
                        route_pkt(&(schedule_mem_pool.sc_mem_array[i].tc_pck));
                        schedule_mem_pool.sc_mem_array[i].pos_taken = false;
                        sc_s_state.nmbr_of_ld_sched--;
                        sc_s_state.schedule_arr_full = false;
                    }
                    break;                    
                case 5: /*REPETITIVE*/
                    //uint32_t boot_secs = HAL_GetTick();
                    if( schedule_mem_pool.sc_mem_array[i].release_time <= (boot_secs / 1000)){
                        route_pkt(&(schedule_mem_pool.sc_mem_array[i].tc_pck));
                        schedule_mem_pool.sc_mem_array[i].release_time = 0;

//                        schedule_mem_pool.sc_mem_array[i].pos_taken = false;
//                        sc_s_state.nmbr_of_ld_sched--;
//                        sc_s_state.schedule_arr_full = false;
                    }
                    break;
             }
        }
    }
    wdg_reset_SCH();
}

/**
 * Serves requests to Scheduling service, unique entry point to the Service.
 * @param spacket
 * @return 
 */
SAT_returnState scheduling_app( tc_tm_pkt *tc_tm_packet){
    
    /*TODO: add assertions*/
    SAT_returnState insertion_state = SATR_ERROR;
    SC_pkt *sc_packet;
    
    if(!C_ASSERT(tc_tm_packet != NULL) == true) { return SATR_ERROR; }
    
    switch( tc_tm_packet->ser_subtype){
        case 1: /*Enable / Disable release TCs*/
            enable_disable_schedule_apid_release( tc_tm_packet->ser_subtype, tc_tm_packet->data[3] );
            break;
        case 2:
            enable_disable_schedule_apid_release( tc_tm_packet->ser_subtype, tc_tm_packet->data[3] );
            break;
        case 3: /*Reset TCs Schedule*/
            operations_scheduling_reset_schedule_api();
            break;
        case 4: /*Insert TC*/
            if( (sc_packet = find_schedule_pos()) == NULL){
                return SATR_SCHEDULE_FULL;
            }
            else{
                    insertion_state = parse_sch_packet(sc_packet, tc_tm_packet);
                    if (insertion_state == SATR_OK) {
                        /*Place the packet into the scheduling array*/
                        sc_s_state.nmbr_of_ld_sched++;
                        if (sc_s_state.nmbr_of_ld_sched == SC_MAX_STORED_SCHEDULES) {
                            /*schedule array has become full*/
                            sc_s_state.schedule_arr_full = true;
                        }
                    }
                }
            break;
        case 5: /*Delete specific TC from schedule, selection criteria is destined APID and Seq.Count*/
            scheduling_remove_schedule_api( tc_tm_packet->data[1], tc_tm_packet->data[2]);
            break;
        case 6: /*Delete TCs over a time period*/
                /*unimplemented*/
            return SATR_ERROR;
            break;
        case 7: 
            ;
            break;
        case 8: 
            ;
            break;
        case 15: /*Time shift all TCs*/
            time_shift_all_tcs(tc_tm_packet->data);
            break;
        case 16: /*Scheduling reporting*/
//            handle_sch_reporting(tc_tm_packet->data);
            if(!C_ASSERT(tc_tm_packet->data[0] == REPORT_SIMPLE || tc_tm_packet->data[0] == REPORT_FULL) == true ) { return SATR_ERROR; }
            if (tc_tm_packet->data[0] == REPORT_SIMPLE ){
                uint8_t stop_here = 0;
            }
            else if (tc_tm_packet->data[0]== REPORT_FULL){
                uint8_t stop_here = 0;
            }
            break;
        case 24: /*Load TCs from permanent storage*/
            scheduling_service_load_schedules();
            break;
        case 25: /*Save TCs to permanent storage*/
            scheduling_service_save_schedules();
            break;
    }
        
    return SATR_OK;
}

SAT_returnState scheduling_service_crt_pkt_TM(tc_tm_pkt *pkt, uint8_t sid, TC_TM_app_id dest_app_id){

    if(!C_ASSERT(dest_app_id < LAST_APP_ID) == true)  { return SATR_ERROR; }
    
    crt_pkt(pkt, SYSTEM_APP_ID, TM, TC_ACK_NO, TC_TIME_MANAGEMENT_SERVICE, sid, dest_app_id);
    pkt->len = 0;

    return SATR_OK;
}

/**
 * Handles internal reporting functions of scheduling service.
 * @param tc_tm_data, the  data of the report request to distinguish
 * between different report types.
 */
SAT_returnState handle_sch_reporting(uint8_t *tc_tm_data){
    
    
    
    
}

/**
 * Time shifts forward/backward all currently active schedules.
 * For repetitive schedules add /substract the repetition time (restrictions apply).
 * For one time schedules add / substract the execution time (QB50).
 * @param time_v
 * @return the execution state.
 */
SAT_returnState time_shift_all_tcs(uint8_t *time_v){
    
    int32_t time_diff = 0;
    
    time_diff = ( time_diff | time_v[0]) << 8;
    time_diff = ( time_diff | time_v[1]) << 8;
    time_diff = ( time_diff | time_v[2]) << 8;
    time_diff = ( time_diff | time_v[3]);
    
    for ( uint8_t i=0; i< SC_MAX_STORED_SCHEDULES;i++){
        
        if( schedule_mem_pool.sc_mem_array[i].sch_evt == ABSOLUTE){
            
        }else 
        if( schedule_mem_pool.sc_mem_array[i].sch_evt == QB50EPC){
//            schedule_mem_pool.sc_mem_array[i].release_time =
                    
        }
        
    }
    
    return SATR_OK;
}

/**
 * Enable / Disable the APID releases.
 * If the release for a specific APID is enabled (true) then the Sch_pkt(s) destined
 * for this APID are normally scheduled and executed.
 * False otherwise.
 * @param subtype
 * @param apid
 * @return the execution state.
 */
SAT_returnState enable_disable_schedule_apid_release( uint8_t subtype, uint8_t apid  ){
    
    if(!C_ASSERT( subtype == 1 || subtype == 2) == true) { return SATR_ERROR; }
    if(!C_ASSERT( apid < LAST_APP_ID) == true)           { return SATR_ERROR; }
    
    if( subtype == 1 ){
        sc_s_state.scheduling_apids_enabled[apid-1] = true; }
    else{
        sc_s_state.scheduling_apids_enabled[apid-1] = false; }
    
    return SATR_OK;
}

/**
 * Reset the schedule memory pool.
 * Marks every schedule position as invalid and eligible for allocation to a new
 * request. Also, release to every APID will be enabled.
 * @return the execution state.
 */
SAT_returnState operations_scheduling_reset_schedule_api(){
    
    uint8_t g = 0;
    sc_s_state.nmbr_of_ld_sched = 0;
    sc_s_state.schedule_arr_full = false;
    
    /*mark every pos as !valid, = available*/
    for( ;g<SC_MAX_STORED_SCHEDULES;g++ ){
        schedule_mem_pool.sc_mem_array[g].pos_taken = false;
    }
    /*enable release for all apids*/
    for( g=0;g<LAST_APP_ID;g++ ){
        sc_s_state.scheduling_apids_enabled[g] = true;
    }
    //TODO: reload schedules from storage?
    return SATR_OK;
}

/**
 * Extracts the inner TC packet from the Sch_pkt structure
 * @param buf is the source of the data.
 * @param pkt the inner tc_tm_pkt to be created.
 * @param size
 * @return the execution state.
 */
SAT_returnState copy_inner_tc(const uint8_t *buf, tc_tm_pkt *pkt, const uint16_t size) {

    uint8_t tmp_crc[2];
    uint8_t ver, dfield_hdr, ccsds_sec_hdr, tc_pus;
    if(!C_ASSERT(buf != NULL && pkt != NULL && pkt->data != NULL) == true)  { return SATR_ERROR; }
    if(!C_ASSERT(size < MAX_PKT_SIZE) == true)                              { return SATR_ERROR; }

    tmp_crc[0] = buf[size - 1];
    checkSum(buf, size-2, &tmp_crc[1]); /* -2 for excluding the checksum bytes*/
    ver = buf[0] >> 5;
    pkt->type = (buf[0] >> 4) & 0x01;
    dfield_hdr = (buf[0] >> 3) & 0x01;

    pkt->app_id = (TC_TM_app_id)buf[1];

    pkt->seq_flags = buf[2] >> 6;

    cnv8_16((uint8_t*)&buf[2], &pkt->seq_count);
    pkt->seq_count &= 0x3FFF;

    cnv8_16((uint8_t*)&buf[4], &pkt->len);

    ccsds_sec_hdr = buf[6] >> 7;

    tc_pus = buf[6] >> 4;

    pkt->ack = 0x04 & buf[6];

    pkt->ser_type = buf[7];
    pkt->ser_subtype = buf[8];
    pkt->dest_id = (TC_TM_app_id)buf[9];

    pkt->verification_state = SATR_PKT_INIT;

    if(!C_ASSERT(pkt->app_id < LAST_APP_ID) == true) {
        pkt->verification_state = SATR_PKT_ILLEGAL_APPID;
        return SATR_PKT_ILLEGAL_APPID; 
    }

    if(!C_ASSERT(pkt->len == size - ECSS_HEADER_SIZE - 1) == true) {
        pkt->verification_state = SATR_PKT_INV_LEN;
        return SATR_PKT_INV_LEN; 
    }
    pkt->len = pkt->len - ECSS_DATA_HEADER_SIZE - ECSS_CRC_SIZE + 1;

    if(!C_ASSERT(tmp_crc[0] == tmp_crc[1]) == true) {
        pkt->verification_state = SATR_PKT_INC_CRC;
        return SATR_PKT_INC_CRC; 
    }

    if(!C_ASSERT(services_verification_TC_TM[pkt->ser_type][pkt->ser_subtype][pkt->type] == 1) == true) { 
        pkt->verification_state = SATR_PKT_ILLEGAL_PKT_TP;
        return SATR_PKT_ILLEGAL_PKT_TP; 
    }

    if(!C_ASSERT(ver == ECSS_VER_NUMBER) == true) {
        pkt->verification_state = SATR_ERROR;
        return SATR_ERROR; 
    }

    if(!C_ASSERT(tc_pus == ECSS_PUS_VER) == true) {
        pkt->verification_state = SATR_ERROR;
        return SATR_ERROR;
    }

    if(!C_ASSERT(ccsds_sec_hdr == ECSS_SEC_HDR_FIELD_FLG) == true) {
        pkt->verification_state = SATR_ERROR;
        return SATR_ERROR;
    }

    if(!C_ASSERT(pkt->type == TC || pkt->type == TM) == true) {
        pkt->verification_state = SATR_ERROR;
        return SATR_ERROR;
    }

    if(!C_ASSERT(dfield_hdr == ECSS_DATA_FIELD_HDR_FLG) == true) {
        pkt->verification_state = SATR_ERROR;
        return SATR_ERROR;
    }

    if(!C_ASSERT(pkt->ack == TC_ACK_NO || pkt->ack == TC_ACK_ACC) == true) {
        pkt->verification_state = SATR_ERROR;
        return SATR_ERROR;
    }

    if(!C_ASSERT(pkt->seq_flags == TC_TM_SEQ_SPACKET) == true) {
        pkt->verification_state = SATR_ERROR;
        return SATR_ERROR;
    }

    /*assertion for data size depanding on pkt type*/
    //if(!C_ASSERT(pkt->len == pkt_size[app_id][type][subtype][generic] == true) {
    //    pkt->verification_state = SATR_ERROR;
    //    return SATR_ERROR;
    //}    

    for(int i = 0; i < pkt->len; i++) {
        pkt->data[i] = buf[ECSS_DATA_OFFSET+i];
    }

    return SATR_OK;
}

/**
 * Inserts a given Schedule_pck on the schedule array
 * Service Subtype 4
 * @param posit, position of schedule to set.
 * @param theSchpck, the SC_pkt to insert in the schedule.
 * @return the execution state.
 */
SAT_returnState scheduling_insert_api( uint8_t posit, SC_pkt theSchpck){
    
    schedule_mem_pool.sc_mem_array[posit].app_id = theSchpck.app_id;
    schedule_mem_pool.sc_mem_array[posit].assmnt_type = theSchpck.assmnt_type;
    schedule_mem_pool.sc_mem_array[posit].enabled = theSchpck.enabled;
    schedule_mem_pool.sc_mem_array[posit].intrlck_set_id = theSchpck.intrlck_set_id;
    schedule_mem_pool.sc_mem_array[posit].intrlck_ass_id = theSchpck.intrlck_ass_id;
    schedule_mem_pool.sc_mem_array[posit].num_of_sch_tc = theSchpck.num_of_sch_tc;
    schedule_mem_pool.sc_mem_array[posit].release_time = theSchpck.release_time;
    schedule_mem_pool.sc_mem_array[posit].sch_evt = theSchpck.sch_evt;
    schedule_mem_pool.sc_mem_array[posit].seq_count = theSchpck.seq_count;
    schedule_mem_pool.sc_mem_array[posit].sub_schedule_id = theSchpck.sub_schedule_id;
    schedule_mem_pool.sc_mem_array[posit].timeout = theSchpck.timeout;
    schedule_mem_pool.sc_mem_array[posit].pos_taken = theSchpck.pos_taken;
    schedule_mem_pool.sc_mem_array[posit].timeout = theSchpck.timeout;
    
    schedule_mem_pool.sc_mem_array[posit].tc_pck.ack = theSchpck.tc_pck.ack;
    schedule_mem_pool.sc_mem_array[posit].tc_pck.app_id = theSchpck.tc_pck.app_id;
    uint8_t i=0;
    for( ;i<theSchpck.tc_pck.len;i++){
        schedule_mem_pool.sc_mem_array[posit].tc_pck.data[i] = theSchpck.tc_pck.data[i];
    }
    schedule_mem_pool.sc_mem_array[posit].tc_pck.dest_id = theSchpck.tc_pck.dest_id;
    schedule_mem_pool.sc_mem_array[posit].tc_pck.len = theSchpck.tc_pck.len;
    schedule_mem_pool.sc_mem_array[posit].tc_pck.seq_count = theSchpck.tc_pck.seq_count;
    schedule_mem_pool.sc_mem_array[posit].tc_pck.seq_flags = theSchpck.tc_pck.seq_flags;
    schedule_mem_pool.sc_mem_array[posit].tc_pck.ser_subtype = theSchpck.tc_pck.ser_subtype;
    schedule_mem_pool.sc_mem_array[posit].tc_pck.ser_type = theSchpck.tc_pck.ser_type;
    schedule_mem_pool.sc_mem_array[posit].tc_pck.verification_state = theSchpck.tc_pck.verification_state;
    
    /*check if schedule array is already full*/
        
//    if ( !C_ASSERT(sc_s_state.schedule_arr_full) == true ){  
//        /*TODO: Here to create a telemetry/log saying "I'm full"*/
//        return SATR_SCHEDULE_FULL;
//    }
    
//    uint8_t pos = find_schedule_pos(scheduling_mem_array);
//    if ( !C_ASSERT(pos != SC_MAX_STORED_SCHEDULES) == true){
//        return SATR_SCHEDULE_FULL;
//    }
    
    /*Check sub-schedule id*/
//    if ( C_ASSERT(theSchpck->sub_schedule_id !=1) == true ){
//        return SATR_SSCH_ID_INVALID;
//    }
//    /*Check number of tc in schpck id*/
//    if ( C_ASSERT(theSchpck->num_of_sch_tc !=1) == true ){
//        return SATR_NMR_OF_TC_INVALID;
//    }
//    /*Check interlock set id*/
//    if ( C_ASSERT(theSchpck->intrlck_set_id !=0) == true){
//        return SATR_INTRL_ID_INVALID;
//    }
//    /*Check interlock assessment id*/
//    if ( C_ASSERT(theSchpck->intrlck_ass_id !=1) == true ){
//        return SATR_ASS_INTRL_ID_INVALID;
//    }
//    /*Check release time type id*/
//    if ( (!C_ASSERT(theSchpck->sch_evt != ABSOLUTE) == true) ||
//         (!C_ASSERT(theSchpck->sch_evt != QB50EPC) == true) ){
//        return SATR_RLS_TIMET_ID_INVALID;
//    }
    /*Check time value*/
//    if (   ){
//        return TIME_SPEC_INVALID;
//    }
    /*Check execution time out*/
//    if (  ){
//       return INTRL_LOGIC_ERROR; 
//    }
    
    return SATR_OK;
}

/**
 * 
 * @return the execution state.
 */
SAT_returnState scheduling_state_api(){

    return (scheduling_enabled ? SATR_OK : SATR_SCHEDULE_DISABLED);
}

/**
 * Removes a given Schedule_pck from the schedule array
 * Service Subtype 5.
 * Selection Criteria is destined APID and Sequence Count.
 * @param apid
 * @param seqc
 * @return the execution state.
 */
SAT_returnState scheduling_remove_schedule_api( /*SC_pkt* sch_mem_pool,  
                                                SC_pkt* theSchpck, */ uint8_t apid, uint16_t seqc ){
    
    for(uint8_t i=0;i<SC_MAX_STORED_SCHEDULES;i++){
            if ( /*schedule_mem_pool.sc_mem_array[i].valid == true &&*/
                schedule_mem_pool.sc_mem_array[i].seq_count == seqc &&   
                schedule_mem_pool.sc_mem_array[i].app_id == apid ){
                    
                schedule_mem_pool.sc_mem_array[i].pos_taken = false;
                sc_s_state.nmbr_of_ld_sched--;
                sc_s_state.schedule_arr_full = false;
            }
        }
    return SATR_OK;
} 

/**
 * 
 * @param sch_mem_pool
 * @return the execution state.
 */
SAT_returnState scheduling_reset_schedule_api(SC_pkt* sch_mem_pool){
    uint8_t pos = 0;
    while( pos<SC_MAX_STORED_SCHEDULES ){
        sch_mem_pool[pos++].pos_taken = false;
    }
    return SATR_OK;
}

/**
 * Time shifts all Schedule_pcks on the Schedule.
 * int32_t secs parameter can be positive or negative seconds value.
 * If positive the seconds are added to the Schedule's TC time, 
 * if negative the seconds are substracted from the Schedule's TC time value. 
 * Service Subtype 15.
 * @param sch_mem_pool
 * @param secs
 * @return the execution state.
 */
SAT_returnState scheduling_time_shift_all_schedules_api(SC_pkt* sch_mem_pool, int32_t secs ){
    
    uint8_t pos = 0;
    while( pos<SC_MAX_STORED_SCHEDULES ){
        if (sch_mem_pool[pos].sch_evt == ABSOLUTE ){
            /*convert the secs to utc and add them or remove them from the time field.*/
            
            /*TODO: timing api*/
        }
        else
        if(sch_mem_pool[pos].sch_evt == QB50EPC ){
            /*add them or remove them from the time field. Error if */
            
            /*TODO: timing api*/
        }
    }
    return SATR_OK;
}

/** 
 * Time shifts selected Schedule_pcks on the Schedule 
 * Service Subtype 7
 * @param sch_mem_pool
 * @param apid
 * @param seqcount
 * @return the execution state.
 */
SAT_returnState time_shift_sel_schedule(SC_pkt* sch_mem_pool, uint8_t apid, uint16_t seqcount ){
    
    uint8_t pos = 0;
    while( pos<SC_MAX_STORED_SCHEDULES ){
//        if( sch_mem_pool[pos].app_id == apid && 
//            sch_mem_pool[pos].seq_count == seqcount)
//        {
//            /*this is the schedule to be timeshifted. shiftit*/
//            if (sch_mem_pool[pos].sch_evt == ABSOLUTE ){
//            /*convert the secs to utc and add them or remove them from the time field.*/
//            
//        }
//            else
//            if(sch_mem_pool[pos].sch_evt == QB50EPC ){
//                /*add them or remove them from the time field. Error if */
//            }
//        }
        pos++;
    }
    return SATR_OK;
}

/* Find a 'free' (non-valid schedule) position in the Schedule_pck array to
 * insert the Scheduling packet, and return its address.
 */
SC_pkt* find_schedule_pos(/*SC_pkt* sche_mem_pool*/) {

    for (uint8_t i = 0; i < SC_MAX_STORED_SCHEDULES; i++) {
        if (!schedule_mem_pool.sc_mem_array[i].pos_taken) {
            return &(schedule_mem_pool.sc_mem_array[i]);
        }
    }
    return NULL;
}

/* Reports summary info of all telecommands from the Schedule 
 * * Service Subtype 
 *
OBC_returnStateTypedef ( Schedule_pck theSchpck ){
    
    return R_OK;
}*/

/* 
 * * Service Subtype 10
 *
OBC_returnStateTypedef report_( Schedule_pck theSchpck ){
    
    return R_OK;
}*/

/**
 * Reports summary info of all telecommands from the Schedule 
 * Service Subtype 17
 * @param theSchpck
 * @return the execution state.
 */
SAT_returnState report_summary_all( SC_pkt theSchpck ){
    
    return SATR_OK;
}

/**
 * Time shifts selected telecommands over a time period on the Schedule 
 * Service Subtype 8 
 * @param theSchpck
 * @return the execution state.
 */
SAT_returnState time_shift_sel_scheduleOTP( SC_pkt* theSchpck ){
    
    return SATR_OK;
}

/**
 * Reports detailed info about every telecommand the Schedule 
 * Service Subtype 16 
 * @param theSchpck
 * @return the execution state.
 */
SAT_returnState report_detailed( SC_pkt theSchpck ){
    
    return SATR_OK;
}

/**
 *  Reports summary info of a subset of telecommands from the Schedule 
 * Service Subtype 12
 * @param theSchpck
 * @return the execution state.
 */
SAT_returnState report_summary_subset( SC_pkt theSchpck ){
    
    return SATR_OK;
}

/**
 * Reports detailed info about a subset of telecommands from the Schedule 
 * Service Subtype 9
 * @param theSchpck
 * @return the execution state.
 */
SAT_returnState report_detailed_subset( SC_pkt theSchpck ){
    
    return SATR_OK;
}

/**
 * 
 * @param sc_pkt
 * @param tc_pkt
 * @return the execution state.
 */
SAT_returnState parse_sch_packet(SC_pkt *sc_pkt, tc_tm_pkt *tc_pkt) {

    /*extract the packet and route accordingly*/
    uint32_t time = 0;
    uint16_t exec_timeout = 0;
    uint8_t offset = 14;

    /*extract the scheduling packet from the data pointer*/
    (*sc_pkt).sub_schedule_id = tc_pkt->data[0];
    if (!C_ASSERT((*sc_pkt).sub_schedule_id == 1) == true) {
        return SATR_SSCH_ID_INVALID;
    }

    (*sc_pkt).num_of_sch_tc = tc_pkt->data[1];
    if (!C_ASSERT((*sc_pkt).num_of_sch_tc == 1) == true) {

        return SATR_NMR_OF_TC_INVALID;
    }

    (*sc_pkt).intrlck_set_id = tc_pkt->data[2];
    if (!C_ASSERT((*sc_pkt).intrlck_set_id == 0) == true) {

        return SATR_INTRL_ID_INVALID;
    }

    (*sc_pkt).intrlck_ass_id = tc_pkt->data[3];
    if (!C_ASSERT((*sc_pkt).intrlck_ass_id == 0) == true) {

        return SATR_ASS_INTRL_ID_INVALID;
    }

    (*sc_pkt).assmnt_type = tc_pkt->data[4];
    if (!C_ASSERT((*sc_pkt).assmnt_type == 1) == true) {

        return SATR_ASS_TYPE_ID_INVALID;
    }

    (*sc_pkt).sch_evt = (SC_event_time_type) tc_pkt->data[5];
    if (!C_ASSERT((*sc_pkt).sch_evt < LAST_EVENTTIME) == true) {

        return SATR_RLS_TIMET_ID_INVALID;
    }
    
    /*7,8,9,10th bytes are the time fields, combine them to a uint32_t*/
    time = (time | tc_pkt->data[9]) << 8;
    time = (time | tc_pkt->data[8]) << 8;
    time = (time | tc_pkt->data[7]) << 8;
    time = (time | tc_pkt->data[6]);

    /*read execution time out fields*/
    exec_timeout = (exec_timeout | tc_pkt->data[13]) << 8;
    exec_timeout = (exec_timeout | tc_pkt->data[12]) << 8;
    exec_timeout = (exec_timeout | tc_pkt->data[11]) << 8;
    exec_timeout = (exec_timeout | tc_pkt->data[10]);

    /*extract data from internal TC packet ( app_id )*/
    (*sc_pkt).app_id = (TC_TM_app_id)tc_pkt->data[offset + 1];
    if (!C_ASSERT((*sc_pkt).app_id < LAST_APP_ID) == true) {
        return SATR_PKT_ILLEGAL_APPID;
    }
    (*sc_pkt).seq_count = (*sc_pkt).seq_count | (tc_pkt->data[offset + 2] >> 2);
    (*sc_pkt).seq_count << 8;
    (*sc_pkt).seq_count = (*sc_pkt).seq_count | (tc_pkt->data[offset + 3]);
    (*sc_pkt).release_time = time;
    (*sc_pkt).timeout = exec_timeout;
    (*sc_pkt).pos_taken = true;
    (*sc_pkt).enabled = true;

    /*copy the internal TC packet for future use*/
    /*  tc_pkt is a TC containing 14 bytes of data related to scheduling service.
     *  After those 14 bytes, a 'whole_inner_tc' packet starts.
     *  
     *  The 'whole_inner_tc' offset in the tc_pkt's data payload is: 15 (16th byte).
     *  
     *  The length of the 'whole_inner_tc' is tc_pkt->data - 14 bytes
     *  
     *  Within the 'whole_inner_tc' the length of the 'inner' goes for:
     *  16+16+16+32+(tc_pkt->len - 11)+16 bytes.
     */
    return copy_inner_tc( &(tc_pkt->data[14]), &((*sc_pkt).tc_pck), (uint16_t) tc_pkt->len - 14);

}
