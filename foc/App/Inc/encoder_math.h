#ifndef ENCODER_MATH_H
#define ENCODER_MATH_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Mechanical structure is fixed:
 *
 * Reel pulley  : 60T
 * Motor pulley : 20T
 *
 * Therefore:
 *
 *   motor 3 revolutions = reel 1 revolution
 */
#define ENCODER_MATH_REDUCTION_RATIO    (3.0f)


/*
 * Validate empirically determined encoder CPR.
 *
 * IMPORTANT:
 * BL4270E1K specification says "1000 Lines",
 * but this code does NOT assume that this means
 * 1000, 2000, or 4000 counts/revolution.
 *
 * The actual count change for one motor shaft revolution
 * will be measured after hardware arrives.
 */
bool EncoderMath_IsCprValid(uint32_t counts_per_motor_rev);


/*
 * Convert signed cumulative encoder count
 * to motor shaft revolutions.
 *
 * Positive/negative direction is preserved.
 */
float EncoderMath_CountToMotorTurns(int64_t cumulative_count,
                                    uint32_t counts_per_motor_rev);


/*
 * Convert signed cumulative encoder count
 * directly to reel shaft revolutions.
 *
 * reel_turns = motor_turns / 3
 */
float EncoderMath_CountToReelTurns(int64_t cumulative_count,
                                   uint32_t counts_per_motor_rev);


/*
 * Calculate motor RPM from encoder count change.
 *
 * delta_count:
 *   encoder count difference during dt_sec
 *
 * dt_sec:
 *   measurement interval in seconds
 */
float EncoderMath_DeltaCountToMotorRpm(int32_t delta_count,
                                       uint32_t counts_per_motor_rev,
                                       float dt_sec);


/*
 * Convert motor RPM to reel RPM.
 *
 * reel_rpm = motor_rpm / 3
 */
float EncoderMath_MotorRpmToReelRpm(float motor_rpm);


/*
 * Convenience function:
 *
 * encoder delta count -> reel RPM
 */
float EncoderMath_DeltaCountToReelRpm(int32_t delta_count,
                                      uint32_t counts_per_motor_rev,
                                      float dt_sec);


#endif /* ENCODER_MATH_H */
