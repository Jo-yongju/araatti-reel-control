#include "../Inc/motor_if.h"
#include "../../Inc/mc_api.h"

void MotorIf_Init(void)
{
    /* MCSDK starts in torque mode with 0 A default Iq. No automatic START. */
}

void MotorIf_SetSignedNormalizedCommand(float command)
{
    qd_f_t current = {0};
    if ((FFB_MAX_IQ_A <= 0.0f) ||
        (MC_GetSTMStateMotor1() != RUN) ||
        (MotorIf_GetFaults() != 0U))
    {
        return;
    }
    if (command > 1.0f) command = 1.0f;
    if (command < -1.0f) command = -1.0f;
    current.q = command * FFB_MAX_IQ_A;
    current.d = 0.0f;
    MC_SetCurrentReferenceMotor1_F(current);
}

void MotorIf_Stop(void)
{
    MCI_State_t state = MC_GetSTMStateMotor1();

    /* These are the start/run phases handled by this generated state machine.
     * MC_StopMotor1() accepts a stop request in each phase when fault-free.
     */
    if ((state == OFFSET_CALIB) || (state == CHARGE_BOOT_CAP) ||
        (state == ALIGNMENT) || (state == WAIT_STOP_MOTOR) || (state == RUN))
    {
        if (MotorIf_GetFaults() == 0U)
        {
            if (state == RUN)
            {
                /* Stop processing clears FOC references and disables PWM. */
                MC_ProgramTorqueRampMotor1_F(0.0f, 0U);
            }
            (void)MC_StopMotor1();
        }
    }
}

uint16_t MotorIf_GetFaults(void)
{
    return (uint16_t)(MC_GetCurrentFaultsMotor1() |
                      MC_GetOccurredFaultsMotor1());
}
