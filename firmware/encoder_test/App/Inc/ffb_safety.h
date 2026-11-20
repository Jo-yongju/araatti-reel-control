#ifndef FFB_SAFETY_H
#define FFB_SAFETY_H

#include <stdbool.h>
#include <stdint.h>

/*
 * FFB Safety V1
 *
 * - Valid resistance command:
 *      refreshes 300 ms communication timeout
 *
 * - STOP:
 *      immediately requests torque = 0
 *
 * - Timeout:
 *      immediately requests torque = 0
 *
 * - Fault:
 *      immediately requests torque = 0
 *      fault is latched until FFB_Safety_Init() is called again
 *
 * - Invalid packet:
 *      must NOT call FFB_Safety_OnValidResistance()
 *      therefore timeout is NOT refreshed
 */

#define FFB_SAFETY_TIMEOUT_MS    (300U)


typedef enum
{
    FFB_SAFETY_READY = 0,
    FFB_SAFETY_ACTIVE,
    FFB_SAFETY_FAULT

} FFB_SafetyState;


typedef struct
{
    FFB_SafetyState state;

    uint32_t last_valid_command_ms;

    bool valid_command_received;
    bool force_zero;
    bool timed_out;
    bool fault_latched;

} FFB_Safety;


/*
 * Initialize safety state.
 *
 * Startup behavior:
 *   - torque must remain zero
 *   - state = READY
 */
void FFB_Safety_Init(FFB_Safety *safety,
                     uint32_t now_ms);


/*
 * Call ONLY after a valid R,000 ~ R,100 command
 * has successfully passed the protocol parser.
 *
 * resistance_percent:
 *   0 ~ 100
 *
 * A valid command refreshes the communication timeout.
 */
void FFB_Safety_OnValidResistance(FFB_Safety *safety,
                                  uint8_t resistance_percent,
                                  uint32_t now_ms);


/*
 * Immediate STOP.
 *
 * This bypasses any future ramp/slew logic.
 */
void FFB_Safety_OnStop(FFB_Safety *safety);


/*
 * Latch a motor/FOC/driver fault.
 *
 * Once faulted, normal resistance commands cannot
 * reactivate torque.
 */
void FFB_Safety_OnFault(FFB_Safety *safety);


/*
 * Periodic safety update.
 *
 * Call repeatedly with current time in milliseconds.
 *
 * If no valid resistance command has been received
 * for >= 300 ms, torque is forced to zero.
 */
void FFB_Safety_Update(FFB_Safety *safety,
                       uint32_t now_ms);


/*
 * true:
 *   output must immediately become zero
 *
 * false:
 *   normal control may generate torque
 */
bool FFB_Safety_ShouldForceZero(const FFB_Safety *safety);


bool FFB_Safety_IsTimedOut(const FFB_Safety *safety);


bool FFB_Safety_IsFaulted(const FFB_Safety *safety);


FFB_SafetyState FFB_Safety_GetState(const FFB_Safety *safety);


#endif /* FFB_SAFETY_H */
