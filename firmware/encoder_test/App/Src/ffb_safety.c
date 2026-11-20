#include "ffb_safety.h"
#include <stddef.h>

void FFB_Safety_Init(FFB_Safety *safety,
                     uint32_t now_ms)
{
    if (safety == NULL)
    {
        return;
    }

    safety->state = FFB_SAFETY_READY;

    safety->last_valid_command_ms = now_ms;

    safety->valid_command_received = false;

    /*
     * Startup must always be torque-off.
     */
    safety->force_zero = true;

    safety->timed_out = false;
    safety->fault_latched = false;
}


void FFB_Safety_OnValidResistance(FFB_Safety *safety,
                                  uint8_t resistance_percent,
                                  uint32_t now_ms)
{
    if (safety == NULL)
    {
        return;
    }

    /*
     * Defensive range check.
     *
     * Protocol parser should already reject > 100,
     * but Safety does not blindly trust external input.
     */
    if (resistance_percent > 100U)
    {
        return;
    }

    /*
     * Fault is latched.
     *
     * A normal command must NEVER reactivate torque
     * while a fault is active.
     */
    if (safety->fault_latched)
    {
        return;
    }

    safety->last_valid_command_ms = now_ms;
    safety->valid_command_received = true;

    safety->timed_out = false;
    safety->force_zero = false;

    if (resistance_percent == 0U)
    {
        safety->state = FFB_SAFETY_READY;
    }
    else
    {
        safety->state = FFB_SAFETY_ACTIVE;
    }
}


void FFB_Safety_OnStop(FFB_Safety *safety)
{
    if (safety == NULL)
    {
        return;
    }

    /*
     * STOP must immediately force torque to zero.
     */
    safety->force_zero = true;

    /*
     * STOP is not a fault.
     */
    if (!safety->fault_latched)
    {
        safety->state = FFB_SAFETY_READY;
    }
}


void FFB_Safety_OnFault(FFB_Safety *safety)
{
    if (safety == NULL)
    {
        return;
    }

    safety->fault_latched = true;

    safety->force_zero = true;
    safety->state = FFB_SAFETY_FAULT;
}


void FFB_Safety_Update(FFB_Safety *safety,
                       uint32_t now_ms)
{
    uint32_t elapsed_ms;

    if (safety == NULL)
    {
        return;
    }

    /*
     * Fault always has highest priority.
     */
    if (safety->fault_latched)
    {
        safety->force_zero = true;
        safety->state = FFB_SAFETY_FAULT;
        return;
    }

    /*
     * No valid command has ever arrived.
     *
     * Keep startup torque at zero.
     */
    if (!safety->valid_command_received)
    {
        safety->force_zero = true;
        return;
    }

    /*
     * Unsigned subtraction intentionally handles
     * uint32_t tick wrap-around correctly.
     */
    elapsed_ms =
        now_ms - safety->last_valid_command_ms;

    if (elapsed_ms >= FFB_SAFETY_TIMEOUT_MS)
    {
        safety->timed_out = true;

        /*
         * Timeout bypasses ramp.
         * Torque must immediately become zero.
         */
        safety->force_zero = true;
        safety->state = FFB_SAFETY_READY;
    }
}


bool FFB_Safety_ShouldForceZero(const FFB_Safety *safety)
{
    if (safety == NULL)
    {
        return true;
    }

    return safety->force_zero;
}


bool FFB_Safety_IsTimedOut(const FFB_Safety *safety)
{
    if (safety == NULL)
    {
        return false;
    }

    return safety->timed_out;
}


bool FFB_Safety_IsFaulted(const FFB_Safety *safety)
{
    if (safety == NULL)
    {
        return true;
    }

    return safety->fault_latched;
}


FFB_SafetyState FFB_Safety_GetState(const FFB_Safety *safety)
{
    if (safety == NULL)
    {
        return FFB_SAFETY_FAULT;
    }

    return safety->state;
}
