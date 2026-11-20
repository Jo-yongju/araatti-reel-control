#include <stdio.h>
#include <string.h>

#include "ffb_protocol.h"
#include "ffb_safety.h"
#include "ffb_control.h"
#include "motor_if.h"
#include "encoder_math.h"


static int g_pass_count = 0;
static int g_fail_count = 0;


static int FloatNear(float a, float b, float tolerance)
{
    float diff = a - b;

    if (diff < 0.0f)
    {
        diff = -diff;
    }

    return (diff <= tolerance);
}


#define CHECK(name, condition)                                      \
    do                                                              \
    {                                                               \
        if (condition)                                              \
        {                                                           \
            printf("[PASS] %s\n", name);                            \
            g_pass_count++;                                         \
        }                                                           \
        else                                                        \
        {                                                           \
            printf("[FAIL] %s  (%s:%d)\n", name, __FILE__, __LINE__); \
            g_fail_count++;                                         \
        }                                                           \
    } while (0)


static void Test_Protocol(void)
{
    FFB_Command cmd;
    bool result;

    printf("\n=== Protocol Parser ===\n");

    result = FFB_Protocol_ParseLine("R,000", 5U, &cmd);

    CHECK("R,000 valid",
          result == true);

    CHECK("R,000 type",
          cmd.type == FFB_CMD_RESISTANCE);

    CHECK("R,000 value",
          cmd.resistance_percent == 0U);


    result = FFB_Protocol_ParseLine("R,050\n", 6U, &cmd);

    CHECK("R,050 LF valid",
          result == true);

    CHECK("R,050 value",
          cmd.resistance_percent == 50U);


    result = FFB_Protocol_ParseLine("R,100\r\n", 7U, &cmd);

    CHECK("R,100 CRLF valid",
          result == true);

    CHECK("R,100 value",
          cmd.resistance_percent == 100U);


    result = FFB_Protocol_ParseLine("STOP\n", 5U, &cmd);

    CHECK("STOP valid",
          result == true);

    CHECK("STOP type",
          cmd.type == FFB_CMD_STOP);


    CHECK("R,101 rejected",
          FFB_Protocol_ParseLine("R,101", 5U, &cmd) == false);

    CHECK("R,ABC rejected",
          FFB_Protocol_ParseLine("R,ABC", 5U, &cmd) == false);

    CHECK("R,-10 rejected",
          FFB_Protocol_ParseLine("R,-10", 5U, &cmd) == false);

    CHECK("R,50 rejected",
          FFB_Protocol_ParseLine("R,50", 4U, &cmd) == false);

    CHECK("R,0050 rejected",
          FFB_Protocol_ParseLine("R,0050", 6U, &cmd) == false);

    CHECK("START rejected",
          FFB_Protocol_ParseLine("START", 5U, &cmd) == false);
}


static void Test_Telemetry(void)
{
    char buffer[FFB_TELEMETRY_BUFFER_SIZE];
    char small_buffer[4];
    size_t length;

    printf("\n=== Telemetry Formatter ===\n");

    length = FFB_Protocol_FormatTelemetry(
        buffer,
        sizeof(buffer),
        12540,
        320,
        0U);

    CHECK("Telemetry generated",
          length > 0U);

    CHECK("Telemetry contents",
          strcmp(buffer, "E,12540,320,0\n") == 0);

    CHECK("Telemetry length",
          length == strlen("E,12540,320,0\n"));


    length = FFB_Protocol_FormatTelemetry(
        buffer,
        sizeof(buffer),
        -123,
        -45,
        7U);

    CHECK("Signed telemetry contents",
          strcmp(buffer, "E,-123,-45,7\n") == 0);


    length = FFB_Protocol_FormatTelemetry(
        small_buffer,
        sizeof(small_buffer),
        12540,
        320,
        0U);

    CHECK("Small telemetry buffer rejected",
          length == 0U);
}


static void Test_Safety(void)
{
    FFB_Safety safety;
    FFB_Command cmd;
    bool parsed;

    printf("\n=== Safety ===\n");

    FFB_Safety_Init(&safety, 1000U);

    CHECK("Startup force zero",
          FFB_Safety_ShouldForceZero(&safety) == true);

    CHECK("Startup READY",
          FFB_Safety_GetState(&safety) == FFB_SAFETY_READY);


    FFB_Safety_OnValidResistance(
        &safety,
        50U,
        1000U);

    CHECK("Valid command releases force zero",
          FFB_Safety_ShouldForceZero(&safety) == false);

    CHECK("Resistance 50 -> ACTIVE",
          FFB_Safety_GetState(&safety) == FFB_SAFETY_ACTIVE);


    FFB_Safety_Update(&safety, 1299U);

    CHECK("299 ms no timeout",
          FFB_Safety_IsTimedOut(&safety) == false);

    CHECK("299 ms output allowed",
          FFB_Safety_ShouldForceZero(&safety) == false);


    FFB_Safety_Update(&safety, 1300U);

    CHECK("300 ms timeout",
          FFB_Safety_IsTimedOut(&safety) == true);

    CHECK("Timeout force zero",
          FFB_Safety_ShouldForceZero(&safety) == true);


    /*
     * New valid command should recover from communication timeout.
     */
    FFB_Safety_OnValidResistance(
        &safety,
        25U,
        1400U);

    CHECK("Valid command clears timeout",
          FFB_Safety_IsTimedOut(&safety) == false);

    CHECK("Valid command reactivates",
          FFB_Safety_ShouldForceZero(&safety) == false);


    /*
     * STOP must be immediate.
     */
    FFB_Safety_OnStop(&safety);

    CHECK("STOP force zero",
          FFB_Safety_ShouldForceZero(&safety) == true);

    CHECK("STOP -> READY",
          FFB_Safety_GetState(&safety) == FFB_SAFETY_READY);


    /*
     * Invalid packet must NOT refresh timeout.
     *
     * A valid command arrives at 2000 ms.
     * An invalid command appears at 2200 ms.
     * Since invalid packets are never passed to Safety,
     * timeout must still occur at 2300 ms.
     */
    FFB_Safety_Init(&safety, 2000U);

    FFB_Safety_OnValidResistance(
        &safety,
        50U,
        2000U);

    parsed =
        FFB_Protocol_ParseLine(
            "R,ABC",
            5U,
            &cmd);

    CHECK("Invalid packet rejected before Safety",
          parsed == false);

    if (parsed &&
        (cmd.type == FFB_CMD_RESISTANCE))
    {
        FFB_Safety_OnValidResistance(
            &safety,
            cmd.resistance_percent,
            2200U);
    }

    FFB_Safety_Update(&safety, 2300U);

    CHECK("Invalid packet did not refresh timeout",
          FFB_Safety_IsTimedOut(&safety) == true);


    /*
     * Fault is latched.
     */
    FFB_Safety_Init(&safety, 3000U);

    FFB_Safety_OnValidResistance(
        &safety,
        50U,
        3000U);

    FFB_Safety_OnFault(&safety);

    CHECK("Fault latched",
          FFB_Safety_IsFaulted(&safety) == true);

    CHECK("Fault force zero",
          FFB_Safety_ShouldForceZero(&safety) == true);

    CHECK("Fault state",
          FFB_Safety_GetState(&safety) == FFB_SAFETY_FAULT);


    FFB_Safety_OnValidResistance(
        &safety,
        100U,
        3100U);

    CHECK("Normal command cannot clear fault",
          FFB_Safety_IsFaulted(&safety) == true);

    CHECK("Fault remains torque-off",
          FFB_Safety_ShouldForceZero(&safety) == true);
}


static void Test_Control(void)
{
    FFB_Control control;
    bool result;
    float output;

    printf("\n=== FFB Control ===\n");

    /*
     * rise = 2.0 normalized units/sec
     * fall = 4.0 normalized units/sec
     *
     * These values are TEST values only.
     */
    result =
        FFB_Control_Init(
            &control,
            2.0f,
            4.0f);

    CHECK("Control init",
          result == true);

    CHECK("Initial output zero",
          FloatNear(
              FFB_Control_GetOutput(&control),
              0.0f,
              0.0001f));


    result =
        FFB_Control_SetResistance(
            &control,
            100U);

    CHECK("Set 100 percent",
          result == true);

    CHECK("Target = 1.0",
          FloatNear(
              FFB_Control_GetTarget(&control),
              1.0f,
              0.0001f));


    /*
     * 2.0 / sec * 0.1 sec = +0.2
     */
    output =
        FFB_Control_Update(
            &control,
            0.1f,
            false);

    CHECK("Ramp rise 0.1 sec -> 0.2",
          FloatNear(
              output,
              0.2f,
              0.0001f));


    /*
     * Another 0.4 sec:
     * +0.8 -> reaches 1.0
     */
    output =
        FFB_Control_Update(
            &control,
            0.4f,
            false);

    CHECK("Ramp reaches target 1.0",
          FloatNear(
              output,
              1.0f,
              0.0001f));


    FFB_Control_SetResistance(
        &control,
        0U);

    /*
     * 4.0 / sec * 0.1 sec = -0.4
     *
     * 1.0 -> 0.6
     */
    output =
        FFB_Control_Update(
            &control,
            0.1f,
            false);

    CHECK("Normal decrease uses ramp",
          FloatNear(
              output,
              0.6f,
              0.0001f));


    /*
     * Safety path bypasses ramp.
     */
    output =
        FFB_Control_Update(
            &control,
            0.01f,
            true);

    CHECK("Force zero bypasses ramp",
          FloatNear(
              output,
              0.0f,
              0.0001f));


    CHECK("Resistance >100 rejected",
          FFB_Control_SetResistance(
              &control,
              101U) == false);
}


static void Test_MotorIf(void)
{
    printf("\n=== Motor Interface Mock ===\n");

    MotorIf_Init();

    CHECK("MotorIf starts zero",
          FloatNear(
              MotorIf_GetLastNormalizedCommand(),
              0.0f,
              0.0001f));


    MotorIf_SetNormalizedCommand(0.5f);

    CHECK("MotorIf stores 0.5",
          FloatNear(
              MotorIf_GetLastNormalizedCommand(),
              0.5f,
              0.0001f));


    MotorIf_SetNormalizedCommand(2.0f);

    CHECK("MotorIf clamps high",
          FloatNear(
              MotorIf_GetLastNormalizedCommand(),
              1.0f,
              0.0001f));


    MotorIf_SetNormalizedCommand(-1.0f);

    CHECK("MotorIf clamps low",
          FloatNear(
              MotorIf_GetLastNormalizedCommand(),
              0.0f,
              0.0001f));


    MotorIf_SetNormalizedCommand(0.8f);
    MotorIf_Stop();

    CHECK("MotorIf STOP zero",
          FloatNear(
              MotorIf_GetLastNormalizedCommand(),
              0.0f,
              0.0001f));

    CHECK("Pre-hardware mock has no fault",
          MotorIf_HasFault() == false);
}


static void Test_EncoderMath(void)
{
    const uint32_t test_cpr = 4000U;

    printf("\n=== Encoder Math ===\n");

    /*
     * IMPORTANT:
     * 4000 CPR is only a convenient UNIT TEST value.
     * It is NOT an assumption about the real BL4270 encoder.
     */

    CHECK("CPR zero invalid",
          EncoderMath_IsCprValid(0U) == false);

    CHECK("Test CPR valid",
          EncoderMath_IsCprValid(test_cpr) == true);


    CHECK("12000 counts -> motor 3 turns",
          FloatNear(
              EncoderMath_CountToMotorTurns(
                  12000,
                  test_cpr),
              3.0f,
              0.0001f));


    CHECK("Motor 3 turns -> reel 1 turn",
          FloatNear(
              EncoderMath_CountToReelTurns(
                  12000,
                  test_cpr),
              1.0f,
              0.0001f));


    CHECK("Negative direction preserved",
          FloatNear(
              EncoderMath_CountToReelTurns(
                  -12000,
                  test_cpr),
              -1.0f,
              0.0001f));


    /*
     * 4000 counts / 4000 CPR = 1 revolution/sec
     * = 60 RPM
     */
    CHECK("Motor RPM calculation",
          FloatNear(
              EncoderMath_DeltaCountToMotorRpm(
                  4000,
                  test_cpr,
                  1.0f),
              60.0f,
              0.0001f));


    CHECK("3:1 reduction -> reel 20 RPM",
          FloatNear(
              EncoderMath_DeltaCountToReelRpm(
                  4000,
                  test_cpr,
                  1.0f),
              20.0f,
              0.0001f));


    CHECK("Invalid dt returns zero",
          FloatNear(
              EncoderMath_DeltaCountToMotorRpm(
                  4000,
                  test_cpr,
                  0.0f),
              0.0f,
              0.0001f));
}


int main(void)
{
    printf("FFB Reel - Host Unit Test\n");

    Test_Protocol();
    Test_Telemetry();
    Test_Safety();
    Test_Control();
    Test_MotorIf();
    Test_EncoderMath();

    printf("\n=============================\n");
    printf("PASS : %d\n", g_pass_count);
    printf("FAIL : %d\n", g_fail_count);
    printf("=============================\n");

    if (g_fail_count == 0)
    {
        printf("ALL TESTS PASSED\n");
        return 0;
    }

    printf("TEST FAILURE\n");

    return 1;
}
