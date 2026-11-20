#include "motor_if.h"


/*
 * Pre-hardware mock state.
 *
 * This is NOT actual motor current.
 */
static float s_last_normalized_command = 0.0f;


static float MotorIf_Clamp01(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }

    if (value > 1.0f)
    {
        return 1.0f;
    }

    return value;
}


void MotorIf_Init(void)
{
    s_last_normalized_command = 0.0f;
}


void MotorIf_SetNormalizedCommand(float command)
{
    /*
     * Pre-hardware implementation:
     * only remember the requested normalized command.
     *
     * Later this function becomes roughly:
     *
     * normalized command
     *      ↓
     * resistance-to-Iq mapping
     *      ↓
     * IQ_MAX_SAFE clamp
     *      ↓
     * actual MCSDK API
     */

    s_last_normalized_command =
        MotorIf_Clamp01(command);
}


void MotorIf_Stop(void)
{
    /*
     * Safety path.
     *
     * Later this must result in immediate
     * q-axis current / torque command = 0.
     */

    s_last_normalized_command = 0.0f;
}


bool MotorIf_HasFault(void)
{
    /*
     * No real motor backend exists yet.
     *
     * Do NOT invent hardware fault state.
     */
    return false;
}


float MotorIf_GetLastNormalizedCommand(void)
{
    return s_last_normalized_command;
}
