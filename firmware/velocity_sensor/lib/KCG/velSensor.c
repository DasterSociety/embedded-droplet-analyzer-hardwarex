/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130) 
** Command: scadecg.exe -config C:/Users/daste/OneDrive/Documents/Embedded Systems/ScadeSuite/VelSensor3/VelSensor/KCG/config.txt
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */

#include "kcg_consts.h"
#include "kcg_sensors.h"
#include "velSensor.h"

/* velSensor/ */
void velSensor(inC_velSensor *inC, outC_velSensor *outC)
{
  /* IfBlock1:then:_L27/ */
  kcg_bool _L27_then_IfBlock1;
  /* IfBlock2:then:_L3/, counter_s2/ */
  kcg_uint8 counter_s2_partial;
  /* IfBlock1: */
  kcg_bool IfBlock1_clock;
  /* IfBlock2: */
  kcg_bool IfBlock2_clock;
  /* _L3/, reset/ */
  kcg_bool _L3;

  _L3 = outC->timerInit & outC->timerStop;
  /* _L4=(digital::RisingEdge#1)/ */
  RisingEdge_digital(fs1, &outC->Context_RisingEdge_1);
  IfBlock1_clock = outC->Context_RisingEdge_1.RE_Output & (!_L3);
  /* _L1=(digital::RisingEdge#2)/ */
  RisingEdge_digital(fs2, &outC->Context_RisingEdge_2);
  IfBlock2_clock = outC->Context_RisingEdge_2.RE_Output & outC->timerInit &
    (!_L3);
  /* IfBlock1: */
  if (IfBlock1_clock) {
    _L27_then_IfBlock1 = !outC->timerInit;
    if (_L27_then_IfBlock1) {
      /* IfBlock1:then:_L2=(isr1#1)/ */
      isr1(&outC->_L2_then_IfBlock1, &outC->_L1_then_IfBlock1);
    }
    outC->startTime = outC->_L2_then_IfBlock1;
    outC->timerInit = outC->_L1_then_IfBlock1;
  }
  else /* IfBlock1:else: */
  if (_L3) {
    outC->timerInit = kcg_false;
  }
  /* IfBlock2: */
  if (IfBlock2_clock) {
    /* IfBlock2:then:_L5=(isr2#1)/ */
    isr2(
      inC->n,
      outC->counter_s2,
      &outC->stopTime,
      &outC->timerStop,
      &counter_s2_partial);
    outC->vel = kcg_lit_float32(-1.0);
    outC->counter_s2 = counter_s2_partial;
  }
  else /* IfBlock2:else: */
  if (_L3) {
    outC->timerStop = kcg_false;
    /* IfBlock2:else:then:_L7=(getVelocity#1)/ */
    getVelocity(outC->stopTime, outC->startTime, &outC->vel, &outC->counter_s2);
  }
  else {
    outC->vel = kcg_lit_float32(-1.0);
  }
}

#ifndef KCG_USER_DEFINED_INIT
void velSensor_init(outC_velSensor *outC)
{
  outC->timerInit = kcg_false;
  outC->timerStop = kcg_false;
  outC->_L1_then_IfBlock1 = kcg_false;
  outC->vel = kcg_lit_float32(0.0);
  /* _L1=(digital::RisingEdge#2)/ */
  RisingEdge_init_digital(&outC->Context_RisingEdge_2);
  /* _L4=(digital::RisingEdge#1)/ */
  RisingEdge_init_digital(&outC->Context_RisingEdge_1);
  outC->counter_s2 = kcg_lit_uint8(0);
  outC->stopTime = kcg_lit_uint32(0);
  outC->startTime = kcg_lit_uint32(0);
  outC->_L2_then_IfBlock1 = kcg_lit_uint32(0);
}
#endif /* KCG_USER_DEFINED_INIT */


#ifndef KCG_NO_EXTERN_CALL_TO_RESET
void velSensor_reset(outC_velSensor *outC)
{
  outC->timerInit = kcg_false;
  outC->timerStop = kcg_false;
  outC->_L1_then_IfBlock1 = kcg_false;
  /* _L1=(digital::RisingEdge#2)/ */
  RisingEdge_reset_digital(&outC->Context_RisingEdge_2);
  /* _L4=(digital::RisingEdge#1)/ */
  RisingEdge_reset_digital(&outC->Context_RisingEdge_1);
  outC->counter_s2 = kcg_lit_uint8(0);
  outC->stopTime = kcg_lit_uint32(0);
  outC->startTime = kcg_lit_uint32(0);
  outC->_L2_then_IfBlock1 = kcg_lit_uint32(0);
}
#endif /* KCG_NO_EXTERN_CALL_TO_RESET */



/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130) 
** velSensor.c
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */

