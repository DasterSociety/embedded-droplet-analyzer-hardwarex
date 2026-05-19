/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130)
** Command: scadecg.exe -config C:/Users/daste/OneDrive/Documents/Embedded Systems/ScadeSuite/VelSensor3/VelSensor/KCG/config.txt
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */

#include "kcg_consts.h"
#include "kcg_sensors.h"
#include "isr2.h"

/* isr2/ */
void isr2(
    /* n/ */
    kcg_uint8 n,
    /* _L5/, count_i/ */
    kcg_uint8 count_i,
    /* stopTime/ */
    kcg_uint32 *stopTime,
    /* _L1/, timerStop/ */
    kcg_bool *timerStop,
    /* _L3/, count_o/ */
    kcg_uint8 *count_o)
{
  *count_o = kcg_lit_uint8(1) + count_i;
  *timerStop = count_i == n - kcg_lit_uint8(1);
  *stopTime = Time;
}

/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130)
** isr2.c
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */
