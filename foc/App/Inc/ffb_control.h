#ifndef FFB_CONTROL_H
#define FFB_CONTROL_H

#include <stdbool.h>
#include <stdint.h>


typedef struct
{
    /*
     * Requested resistance:
     * 0.0f ~ 1.0f
     */
    float target_normalized;

    /*
     * Current ramped output:
     * 0.0f ~ 1.0f
     */
    float output_normalized;

    /*
     * Slew rates in normalized units / second.
     *
     * Example:
     * 1.0f means 0 -> 1 takes 1 second.
     *
     * Final values are tuning parameters,
     * not motor electrical parameters.
     */
    float rise_rate_per_sec;
    float fall_rate_per_sec;

} FFB_Control;


/*
 * Initialize control state.
 *
 * rise_rate_per_sec / fall_rate_per_sec
 * must be > 0.
 */
bool FFB_Control_Init(FFB_Control *control,
                      float rise_rate_per_sec,
                      float fall_rate_per_sec);


/*
 * Set target from protocol resistance percentage.
 *
 * Valid range:
 * 0 ~ 100
 *
 * Returns false when input is invalid.
 */
bool FFB_Control_SetSignedPercent(FFB_Control *control, int16_t signed_percent);

bool FFB_Control_SetResistance(FFB_Control *control,
                               uint8_t resistance_percent);


/*
 * Periodic update.
 *
 * dt_sec:
 *   elapsed time since previous update, in seconds.
 *
 * force_zero:
 *   true when Safety requires immediate torque-off
 *   due to STOP / timeout / fault.
 *
 * Returns:
 *   normalized output 0.0f ~ 1.0f
 */
float FFB_Control_Update(FFB_Control *control,
                         float dt_sec,
                         bool force_zero);


/*
 * Immediate reset to zero.
 *
 * Used for initialization or explicit emergency handling.
 */
void FFB_Control_Reset(FFB_Control *control);


float FFB_Control_GetTarget(const FFB_Control *control);

float FFB_Control_GetOutput(const FFB_Control *control);


#endif /* FFB_CONTROL_H */
