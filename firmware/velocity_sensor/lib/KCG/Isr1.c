/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130) 
** Command: scadecg.exe -config C:/Users/daste/OneDrive/Documents/Embedded Systems/ScadeSuite/VelSensor3/VelSensor/KCG/config.txt
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */

#include "kcg_consts.h"
#include "kcg_sensors.h"
#include "isr1.h"

/* isr1/ */
void isr1(
  /* startTime/ */
  kcg_uint32 *startTime,
  /* _L1/, timerRunning/ */
  kcg_bool *timerRunning)
{
  *timerRunning = kcg_true;
  *startTime = Time;
}



/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130) 
** isr1.c
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */

