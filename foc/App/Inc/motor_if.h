#ifndef REEL_MOTOR_IF_H
#define REEL_MOTOR_IF_H

#include <stdint.h>

/* TinyS3 normalized commands are limited below the MCSDK 3.5 A Iq ceiling. */
#define FFB_MAX_IQ_A (3.0f)

void MotorIf_Init(void);
void MotorIf_SetSignedNormalizedCommand(float command);
void MotorIf_Stop(void);
/* Active and non-acknowledged MCSDK faults, combined as one bitmask. */
uint16_t MotorIf_GetFaults(void);

#endif
