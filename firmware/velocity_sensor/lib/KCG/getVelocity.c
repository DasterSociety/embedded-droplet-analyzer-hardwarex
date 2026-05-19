/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130) 
** Command: scadecg.exe -config C:/Users/daste/OneDrive/Documents/Embedded Systems/ScadeSuite/VelSensor3/VelSensor/KCG/config.txt
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */

#include "kcg_consts.h"
#include "kcg_sensors.h"
#include "getVelocity.h"

/* getVelocity/ */
void getVelocity(
  /* _L1/, stopTime/ */
  kcg_uint32 stopTime,
  /* _L7/, startTime/ */
  kcg_uint32 startTime,
  /* _L9/, velocity/ */
  kcg_float32 *velocity,
  /* _L6/, reset_c2/ */
  kcg_uint8 *reset_c2)
{
  *velocity = d / (/* _L2= */(kcg_float32) (stopTime - startTime) /
      kcg_lit_float32(1000000.00));
  *reset_c2 = kcg_lit_uint8(0);
}



/* $ ANSYS SCADE Suite (R) Code Generator version Student 2022 R1 (build 20211130) 
** getVelocity.c
** Generation date: 2025-09-05T16:14:17
*************************************************************$ */

