#ifndef REEL_FFB_APP_H
#define REEL_FFB_APP_H

#include <stdint.h>

#define REEL_ENCODER_COUNTS_PER_MOTOR_REV (4000U)
#define REEL_ENCODER_COUNTS_PER_REEL_REV  (12000U)

void FFB_App_Init(void);
void FFB_App_Poll(void);
void FFB_App_MediumFrequencyHook(void);
void FFB_App_UartIrq(void);

#endif
