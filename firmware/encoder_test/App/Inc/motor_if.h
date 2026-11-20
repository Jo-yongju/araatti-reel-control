#ifndef MOTOR_IF_H
#define MOTOR_IF_H

#include <stdbool.h>

/*
 * Motor Interface
 *
 * Application layer <-> Motor Control backend boundary.
 *
 * Current pre-hardware version:
 *   - does NOT control real hardware
 *   - does NOT call MCSDK
 *   - stores normalized command only
 *
 * Later:
 *   normalized command
 *       -> safe Iq mapping
 *       -> MCSDK q-axis current command
 */


/*
 * Initialize motor interface.
 *
 * Current mock implementation:
 *   command = 0
 */
void MotorIf_Init(void);


/*
 * Apply normalized resistance/torque command.
 *
 * Valid conceptual range:
 *   0.0f ~ 1.0f
 *
 * Values outside the range are defensively clamped.
 *
 * IMPORTANT:
 *   1.0f does NOT mean 1 A.
 *   1.0f does NOT mean motor rated current.
 *
 * Physical Iq mapping will be added after
 * real motor/MCSDK bring-up and safe-current testing.
 */
void MotorIf_SetNormalizedCommand(float command);


/*
 * Immediate torque-off request.
 *
 * STOP / timeout / fault path should use this.
 *
 * Later implementation must bypass normal command ramp
 * and command zero motor torque/current immediately.
 */
void MotorIf_Stop(void);


/*
 * Motor/backend fault state.
 *
 * Current pre-hardware stub always returns false.
 * Later this will be connected to actual MCSDK /
 * inverter / motor fault information.
 */
bool MotorIf_HasFault(void);


/*
 * Current mock command getter.
 *
 * Useful for pre-hardware tests.
 * Later this may remain as diagnostic information.
 */
float MotorIf_GetLastNormalizedCommand(void);


#endif /* MOTOR_IF_H */