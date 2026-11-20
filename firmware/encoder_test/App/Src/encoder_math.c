#include "encoder_math.h"


bool EncoderMath_IsCprValid(uint32_t counts_per_motor_rev)
{
    return (counts_per_motor_rev > 0U);
}


float EncoderMath_CountToMotorTurns(int64_t cumulative_count,
                                    uint32_t counts_per_motor_rev)
{
    if (!EncoderMath_IsCprValid(counts_per_motor_rev))
    {
        return 0.0f;
    }

    return (float)cumulative_count /
           (float)counts_per_motor_rev;
}


float EncoderMath_CountToReelTurns(int64_t cumulative_count,
                                   uint32_t counts_per_motor_rev)
{
    float motor_turns;

    motor_turns =
        EncoderMath_CountToMotorTurns(
            cumulative_count,
            counts_per_motor_rev);

    return motor_turns /
           ENCODER_MATH_REDUCTION_RATIO;
}


float EncoderMath_DeltaCountToMotorRpm(int32_t delta_count,
                                       uint32_t counts_per_motor_rev,
                                       float dt_sec)
{
    float motor_turns;

    if (!EncoderMath_IsCprValid(counts_per_motor_rev))
    {
        return 0.0f;
    }

    if (dt_sec <= 0.0f)
    {
        return 0.0f;
    }

    motor_turns =
        (float)delta_count /
        (float)counts_per_motor_rev;

    /*
     * revolutions / second -> revolutions / minute
     */
    return (motor_turns / dt_sec) * 60.0f;
}


float EncoderMath_MotorRpmToReelRpm(float motor_rpm)
{
    return motor_rpm /
           ENCODER_MATH_REDUCTION_RATIO;
}


float EncoderMath_DeltaCountToReelRpm(int32_t delta_count,
                                      uint32_t counts_per_motor_rev,
                                      float dt_sec)
{
    float motor_rpm;

    motor_rpm =
        EncoderMath_DeltaCountToMotorRpm(
            delta_count,
            counts_per_motor_rev,
            dt_sec);

    return EncoderMath_MotorRpmToReelRpm(
        motor_rpm);
}
