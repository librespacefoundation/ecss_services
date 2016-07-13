#ifndef __SYSVIEW_H
#define __SYSVIEW_H

#define SYSVIEW  0 /* define as 1 if you want to use systemview */

#if(SYSVIEW == 1)

#include "segger_sysview.h"

#define ID_GETPKT  0
#define ID_FREEPKT 1

#define traceGET_PKT(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + PKT_POOL_module.EventOffset, ID)
#define traceFREE_PKT(ID) SEGGER_SYSVIEW_RecordU32(ID_FREEPKT + PKT_POOL_module.EventOffset, ID)

// #define trace_mass_storage_delete_su_scr(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)
// #define trace_mass_storage_hard_delete(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)
// #define trace_mass_storage_downlinkFile(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)
// #define trace_mass_storage_storeFile(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)
// #define trace_mass_storage_list(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)
// #define trace_mass_storage_su_load_api(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)
// #define trace_mass_storage_FORMAT(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)
// #define trace_mass_storage_delete_api(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)
// #define trace_get_fs_stat(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)
// #define trace_get_new_fileId(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + MS_module.EventOffset, ID)

#define ID_DOWN_START  0
#define ID_DOWN_STOP   1

#define ID_STORE_START 2
#define ID_STORE_STOP  3

#define ID_REP_START   4
#define ID_REP_STOP    5

#define ID_LIST_START  6
#define ID_LIST_STOP   7

#define ID_DEL_START  8
#define ID_DEL_STOP   9

#define ID_FORM_START  10
#define ID_FORM_STOP   11

#define trace_MS_DOWN_START() SEGGER_SYSVIEW_RecordVoid(ID_DOWN_START + MS_module.EventOffset)
#define trace_MS_DOWN_STOP() SEGGER_SYSVIEW_RecordVoid(ID_DOWN_STOP + MS_module.EventOffset)
#define trace_MS_STORE_START() SEGGER_SYSVIEW_RecordVoid(ID_STORE_START + MS_module.EventOffset)
#define trace_MS_STORE_STOP() SEGGER_SYSVIEW_RecordVoid(ID_STORE_STOP + MS_module.EventOffset)
#define trace_MS_REP_START() SEGGER_SYSVIEW_RecordVoid(ID_REP_START + MS_module.EventOffset)
#define trace_MS_REP_STOP() SEGGER_SYSVIEW_RecordVoid(ID_REP_STOP + MS_module.EventOffset)
#define trace_MS_LIST_START() SEGGER_SYSVIEW_RecordVoid(ID_LIST_START + MS_module.EventOffset)
#define trace_MS_LIST_STOP() SEGGER_SYSVIEW_RecordVoid(ID_LIST_STOP + MS_module.EventOffset)
#define trace_MS_DEL_START() SEGGER_SYSVIEW_RecordVoid(ID_DEL_START + MS_module.EventOffset)
#define trace_MS_DEL_STOP() SEGGER_SYSVIEW_RecordVoid(ID_DEL_STOP + MS_module.EventOffset)
#define trace_MS_FORM_START() SEGGER_SYSVIEW_RecordVoid(ID_FORM_START + MS_module.EventOffset)
#define trace_MS_FORM_STOP() SEGGER_SYSVIEW_RecordVoid(ID_FORM_STOP + MS_module.EventOffset)

#define ID_IMPORT  0
#define ID_EXPORT  1

#define traceIMPORT(ID) SEGGER_SYSVIEW_RecordU32(ID_IMPORT + COMMS_module.EventOffset, ID)
#define traceEXPORT(ID) SEGGER_SYSVIEW_RecordU32(ID_EXPORT + COMMS_module.EventOffset, ID)

extern SEGGER_SYSVIEW_MODULE PKT_POOL_module;
extern SEGGER_SYSVIEW_MODULE MS_module
extern SEGGER_SYSVIEW_MODULE COMMS_module;

void sysview_init();

#else

#define traceGET_PKT(ID)   ;
#define traceFREE_PKT(ID)  ;

#endif

#endif
