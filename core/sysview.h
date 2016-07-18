#ifndef __SYSVIEW_H
#define __SYSVIEW_H

#define SYSVIEW  0 /* define as 1 if you want to use systemview */

#if(SYSVIEW == 1)

#include "segger_sysview.h"

#define ID_GETPKT  0
#define ID_FREEPKT 1

#define traceGET_PKT(ID) SEGGER_SYSVIEW_RecordU32(ID_GETPKT + PKT_POOL_module.EventOffset, ID)
#define traceFREE_PKT(ID) SEGGER_SYSVIEW_RecordU32(ID_FREEPKT + PKT_POOL_module.EventOffset, ID)

#define ID_DOWN_START  0
#define ID_DOWN_STOP   1

#define ID_STORE_START 2
#define ID_STORE_WRITE 3
#define ID_STORE_STOP  4

#define ID_REP_START   5
#define ID_REP_STOP    6

#define ID_LIST_START  7
#define ID_LIST_STOP   8

#define ID_DEL_START  9
#define ID_DEL_STOP   10

#define ID_FORM_START  11
#define ID_FORM_STOP   12

#define ID_MS_ERROR  14

#define trace_MS_DOWN_START() SEGGER_SYSVIEW_RecordVoid(ID_DOWN_START + MS_module.EventOffset)
#define trace_MS_DOWN_STOP() SEGGER_SYSVIEW_RecordVoid(ID_DOWN_STOP + MS_module.EventOffset)
#define trace_MS_STORE_START() SEGGER_SYSVIEW_RecordVoid(ID_STORE_START + MS_module.EventOffset)
#define trace_MS_STORE_WRITE(FILE) SEGGER_SYSVIEW_RecordU32(ID_STORE_WRITE + MS_module.EventOffset, FILE)
#define trace_MS_STORE_STOP() SEGGER_SYSVIEW_RecordVoid(ID_STORE_STOP + MS_module.EventOffset)
#define trace_MS_REP_START() SEGGER_SYSVIEW_RecordVoid(ID_REP_START + MS_module.EventOffset)
#define trace_MS_REP_STOP() SEGGER_SYSVIEW_RecordVoid(ID_REP_STOP + MS_module.EventOffset)
#define trace_MS_LIST_START() SEGGER_SYSVIEW_RecordVoid(ID_LIST_START + MS_module.EventOffset)
#define trace_MS_LIST_STOP() SEGGER_SYSVIEW_RecordVoid(ID_LIST_STOP + MS_module.EventOffset)
#define trace_MS_DEL_START() SEGGER_SYSVIEW_RecordVoid(ID_DEL_START + MS_module.EventOffset)
#define trace_MS_DEL_STOP() SEGGER_SYSVIEW_RecordVoid(ID_DEL_STOP + MS_module.EventOffset)
#define trace_MS_FORM_START() SEGGER_SYSVIEW_RecordVoid(ID_FORM_START + MS_module.EventOffset)
#define trace_MS_FORM_STOP() SEGGER_SYSVIEW_RecordVoid(ID_FORM_STOP + MS_module.EventOffset)
#define trace_MS_STORE_ERROR(RES, LINE) SEGGER_SYSVIEW_RecordU32x2(ID_MS_ERROR + ASSERTION_module.EventOffset, RES, LINE)

#define ID_IMPORT  0
#define ID_EXPORT  1

#define traceIMPORT(ID) SEGGER_SYSVIEW_RecordU32(ID_IMPORT + COMMS_module.EventOffset, ID)
#define traceEXPORT(ID) SEGGER_SYSVIEW_RecordU32(ID_EXPORT + COMMS_module.EventOffset, ID)

#define ID_ASSERTION  0

#define traceASSERTION(FID, LINE) SEGGER_SYSVIEW_RecordU32x2(ID_ASSERTION + ASSERTION_module.EventOffset, FID, LINE)

extern SEGGER_SYSVIEW_MODULE PKT_POOL_module;
extern SEGGER_SYSVIEW_MODULE MS_module;
extern SEGGER_SYSVIEW_MODULE COMMS_module;
extern SEGGER_SYSVIEW_MODULE ASSERTION_module;

void sysview_init();

#else

#define traceGET_PKT(ID)   ;
#define traceFREE_PKT(ID)  ;

#define trace_MS_DOWN_START()  ;
#define trace_MS_DOWN_STOP()   ;
#define trace_MS_STORE_START() ;
#define trace_MS_STORE_WRITE(FILE) ;
#define trace_MS_STORE_STOP()  ;
#define trace_MS_REP_START()   ;
#define trace_MS_REP_STOP()    ;
#define trace_MS_LIST_START()  ;
#define trace_MS_LIST_STOP()   ;
#define trace_MS_DEL_START()   ;
#define trace_MS_DEL_STOP()    ;
#define trace_MS_FORM_START()  ;
#define trace_MS_FORM_STOP()   ;
#define trace_MS_STORE_ERROR(RES, LINE)  ;

#define traceIMPORT(ID) ;
#define traceEXPORT(ID) ;

#define traceASSERTION(FID, LINE) ;


#endif

#endif
