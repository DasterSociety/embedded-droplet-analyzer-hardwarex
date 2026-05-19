/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130)
** Command: scadecg.exe -config C:/Users/daste/OneDrive/Documents/Embedded Systems/ScadeSuite/VelSensor3/VelSensor/KCG/config.txt
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */
#ifndef _velSensor_H_
#define _velSensor_H_

#include "kcg_types.h"
#include "isr1.h"
#include "isr2.h"
#include "getVelocity.h"
#include "RisingEdge_digital.h"
#include "isr1.c"
#include "isr2.c"
#include "getVelocity.c"
#include "RisingEdge_digital.c"

/* ========================  input structure  ====================== */
typedef struct
{
  kcg_uint8 /* n/ */ n;
} inC_velSensor;

/* =====================  no output structure  ====================== */

/* ========================  context type  ========================= */
typedef struct
{
  /* ---------------------------  outputs  --------------------------- */
  kcg_float32 /* vel/ */ vel;
  /* -----------------------  no local probes  ----------------------- */
  /* ----------------------- local memories  ------------------------- */
  kcg_uint32 /* IfBlock1:then:_L2/ */ _L2_then_IfBlock1;
  kcg_bool /* IfBlock1:then:_L1/ */ _L1_then_IfBlock1;
  kcg_bool /* timerStop/ */ timerStop;
  kcg_uint8 /* counter_s2/ */ counter_s2;
  kcg_uint32 /* stopTime/ */ stopTime;
  kcg_uint32 /* startTime/ */ startTime;
  kcg_bool /* timerInit/ */ timerInit;
  /* ---------------------  sub nodes' contexts  --------------------- */
  outC_RisingEdge_digital /* _L1=(digital::RisingEdge#2)/ */ Context_RisingEdge_2;
  outC_RisingEdge_digital /* _L4=(digital::RisingEdge#1)/ */ Context_RisingEdge_1;
  /* ----------------- no clocks of observable data ------------------ */
} outC_velSensor;

/* ===========  node initialization and cycle functions  =========== */
/* velSensor/ */
extern void velSensor(inC_velSensor *inC, outC_velSensor *outC);

#ifndef KCG_NO_EXTERN_CALL_TO_RESET
extern void velSensor_reset(outC_velSensor *outC);
#endif /* KCG_NO_EXTERN_CALL_TO_RESET */

#ifndef KCG_USER_DEFINED_INIT
extern void velSensor_init(outC_velSensor *outC);
#endif /* KCG_USER_DEFINED_INIT */

#endif /* _velSensor_H_ */
/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130)
** velSensor.h
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */
