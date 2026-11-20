#include "ffb_control.h"

#include <stddef.h>


static float FFB_Control_Clamp01(float value)
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


bool FFB_Control_Init(FFB_Control *control,
                      float rise_rate_per_sec,
                      float fall_rate_per_sec)
{
    if (control == NULL)
    {
        return false;
    }

    if ((rise_rate_per_sec <= 0.0f) ||
        (fall_rate_per_sec <= 0.0f))
    {
        return false;
    }

    control->target_normalized = 0.0f;
    control->output_normalized = 0.0f;

    control->rise_rate_per_sec = rise_rate_per_sec;
    control->fall_rate_per_sec = fall_rate_per_sec;

    return true;
}


bool FFB_Control_SetResistance(FFB_Control *control,
                               uint8_t resistance_percent)
{
    if (control == NULL)
    {
        return false;
    }

    if (resistance_percent > 100U)
    {
        return false;
    }

    control->target_normalized =
        (float)resistance_percent / 100.0f;

    return true;
}


float FFB_Control_Update(FFB_Control *control,
                         float dt_sec,
                         bool force_zero)
{
    float step;

    if (control == NULL)
    {
        return 0.0f;
    }

    /*
     * STOP / timeout / fault:
     *
     * Safety has priority over ramp.
     * Output immediately becomes zero.
     */
    if (force_zero)
    {
        control->output_normalized = 0.0f;

        return 0.0f;
    }

    /*
     * Invalid or zero elapsed time:
     * do not advance ramp.
     */
    if (dt_sec <= 0.0f)
    {
        return control->output_normalized;
    }

    /*
     * Ramp upward.
     */
    if (control->output_normalized <
        control->target_normalized)
    {
        step =
            control->rise_rate_per_sec * dt_sec;

        control->output_normalized += step;

        if (control->output_normalized >
            control->target_normalized)
        {
            control->output_normalized =
                control->target_normalized;
        }
    }

    /*
     * Ramp downward.
     */
    else if (control->output_normalized >
             control->target_normalized)
    {
        step =
            control->fall_rate_per_sec * dt_sec;

        control->output_normalized -= step;

        if (control->output_normalized <
            control->target_normalized)
        {
            control->output_normalized =
                control->target_normalized;
        }
    }

    control->output_normalized =
        FFB_Control_Clamp01(
            control->output_normalized);

    return control->output_normalized;
}


void FFB_Control_Reset(FFB_Control *control)
{
    if (control == NULL)
    {
        return;
    }

    control->target_normalized = 0.0f;
    control->output_normalized = 0.0f;
}


float FFB_Control_GetTarget(const FFB_Control *control)
{
    if (control == NULL)
    {
        return 0.0f;
    }

    return control->target_normalized;
}


float FFB_Control_GetOutput(const FFB_Control *control)
{
    if (control == NULL)
    {
        return 0.0f;
    }

    return control->output_normalized;
}
