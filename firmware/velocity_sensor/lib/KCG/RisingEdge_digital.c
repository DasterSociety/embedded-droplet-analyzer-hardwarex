/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130) 
** Command: scadecg.exe -config C:/Users/daste/OneDrive/Documents/Embedded Systems/ScadeSuite/VelSensor3/VelSensor/KCG/config.txt
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */

#include "kcg_consts.h"
#include "kcg_sensors.h"
#include "RisingEdge_digital.h"

/* digital::RisingEdge/ */
void RisingEdge_digital(
  /* RE_Input/ */
  kcg_bool RE_Input,
  outC_RisingEdge_digital *outC)
{
  outC->RE_Output = (!outC->init) & ((!outC->_L1) & RE_Input);
  outC->init = kcg_false;
  outC->_L1 = RE_Input;
}

#ifndef KCG_USER_DEFINED_INIT
void RisingEdge_init_digital(outC_RisingEdge_digital *outC)
{
  outC->_L1 = kcg_false;
  outC->RE_Output = kcg_true;
  outC->init = kcg_true;
}
#endif /* KCG_USER_DEFINED_INIT */


#ifndef KCG_NO_EXTERN_CALL_TO_RESET
void RisingEdge_reset_digital(outC_RisingEdge_digital *outC)
{
  outC->_L1 = kcg_false;
  outC->init = kcg_true;
}
#endif /* KCG_NO_EXTERN_CALL_TO_RESET */



/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130) 
** RisingEdge_digital.c
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */

