#include "../Inc/ffb_protocol.h"

#include <stdio.h>
#include <inttypes.h>

/* INT64_MIN needs 20 characters including its sign, plus the terminator. */
#define ENCODER_COUNT_TEXT_SIZE (21U)

static void FFB_Protocol_FormatEncoderCount(int64_t value,
                                           char text[ENCODER_COUNT_TEXT_SIZE])
{
    uint64_t magnitude = (uint64_t)value;
    size_t first = 0U;
    size_t length;
    size_t last;

    if (value < 0)
    {
        text[first++] = '-';
        /* Unsigned subtraction also handles INT64_MIN without signed overflow. */
        magnitude = UINT64_C(0) - magnitude;
    }

    length = first;
    do
    {
        text[length++] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    } while (magnitude != 0U);
    text[length] = '\0';

    last = length - 1U;
    while (first < last)
    {
        char digit = text[first];
        text[first++] = text[last];
        text[last--] = digit;
    }
}

/*
 * Remove optional line ending.
 *
 * Supported:
 *   '\n'
 *   '\r\n'
 */
static size_t FFB_Protocol_RemoveLineEnding(const char *data,
                                             size_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return 0U;
    }

    /*
     * Windows / serial style CRLF
     */
    if ((length >= 2U) &&
        (data[length - 2U] == '\r') &&
        (data[length - 1U] == '\n'))
    {
        return length - 2U;
    }

    /*
     * LF only
     */
    if (data[length - 1U] == '\n')
    {
        return length - 1U;
    }

    return length;
}


static bool FFB_Protocol_IsDigit(char c)
{
    return ((c >= '0') && (c <= '9'));
}


bool FFB_Protocol_ParseLine(const char *data,
                            size_t length,
                            FFB_Command *out_cmd)
{
    size_t command_length;
    uint16_t value;

    /*
     * Basic parameter validation
     */
    if ((data == NULL) || (out_cmd == NULL))
    {
        return false;
    }

    /*
     * Always initialize output to a safe invalid state.
     */
    out_cmd->type = FFB_CMD_INVALID;
    out_cmd->resistance_percent = 0U;
    out_cmd->signed_percent = 0;

    command_length =
        FFB_Protocol_RemoveLineEnding(data, length);

    /*
     * --------------------------------------------------
     * STOP
     * --------------------------------------------------
     *
     * Exact format:
     *   STOP
     */
    if (command_length == 4U)
    {
        if ((data[0] == 'S') &&
            (data[1] == 'T') &&
            (data[2] == 'O') &&
            (data[3] == 'P'))
        {
            out_cmd->type = FFB_CMD_STOP;

            return true;
        }

        return false;
    }

    /*
     * --------------------------------------------------
     * Resistance command
     * --------------------------------------------------
     *
     * Exact format:
     *
     *   R,000
     *   R,001
     *      ...
     *   R,100
     *
     * Exactly 5 characters.
     */
    /* Signed bidirectional command, with explicit sign and three digits. */
    if ((command_length == 6U) && (data[0] == 'F') && (data[1] == ',') &&
        ((data[2] == '+') || (data[2] == '-')) &&
        FFB_Protocol_IsDigit(data[3]) && FFB_Protocol_IsDigit(data[4]) &&
        FFB_Protocol_IsDigit(data[5]))
    {
        value = (uint16_t)((uint16_t)(data[3] - '0') * 100U +
                           (uint16_t)(data[4] - '0') * 10U +
                           (uint16_t)(data[5] - '0'));
        if (value > 100U)
        {
            return false;
        }
        out_cmd->type = FFB_CMD_SIGNED;
        out_cmd->signed_percent = (data[2] == '-') ? -(int16_t)value : (int16_t)value;
        return true;
    }

    if (command_length != 5U)
    {
        return false;
    }

    if ((data[0] != 'R') ||
        (data[1] != ','))
    {
        return false;
    }

    /*
     * All three value characters must be decimal digits.
     */
    if ((!FFB_Protocol_IsDigit(data[2])) ||
        (!FFB_Protocol_IsDigit(data[3])) ||
        (!FFB_Protocol_IsDigit(data[4])))
    {
        return false;
    }

    /*
     * Convert DDD -> integer.
     *
     * Example:
     *   '0' '5' '0'
     *       ->
     *   50
     */
    value =
        ((uint16_t)(data[2] - '0') * 100U) +
        ((uint16_t)(data[3] - '0') * 10U) +
        ((uint16_t)(data[4] - '0'));

    /*
     * Protocol range:
     *   0 ~ 100 only
     */
    if (value > 100U)
    {
        return false;
    }

    out_cmd->type = FFB_CMD_RESISTANCE;
    out_cmd->resistance_percent = (uint8_t)value;
    out_cmd->signed_percent = (int16_t)value;

    return true;
}


size_t FFB_Protocol_FormatTelemetry(char *buffer,
                                    size_t buffer_size,
                                    int64_t encoder_count,
                                    int32_t motor_rpm,
                                    uint32_t fault_code)
{
    int written;
    char encoder_count_text[ENCODER_COUNT_TEXT_SIZE];

    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return 0U;
    }

    FFB_Protocol_FormatEncoderCount(encoder_count, encoder_count_text);
    written = snprintf(
        buffer,
        buffer_size,
        "E,%s,%" PRId32 ",%" PRIu32 "\n",
        encoder_count_text,
        motor_rpm,
        fault_code
    );

    /*
     * snprintf error
     */
    if (written < 0)
    {
        buffer[0] = '\0';
        return 0U;
    }

    /*
     * Output was truncated.
     *
     * snprintf returns the number of characters that
     * WOULD have been written, so >= buffer_size means
     * the supplied buffer was too small.
     */
    if ((size_t)written >= buffer_size)
    {
        buffer[0] = '\0';
        return 0U;
    }

    return (size_t)written;
}


size_t FFB_Protocol_FormatReelTelemetry(char *buffer, size_t buffer_size,
                                        int64_t encoder_count, int32_t encoder_delta,
                                        int32_t reel_rpm, uint32_t motor_state,
                                        uint32_t fault_code)
{
    int written;
    char encoder_count_text[ENCODER_COUNT_TEXT_SIZE];
    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return 0U;
    }
    FFB_Protocol_FormatEncoderCount(encoder_count, encoder_count_text);
    written = snprintf(buffer, buffer_size,
                       "E,%s,%" PRId32 ",%" PRId32 ",%" PRIu32 ",%" PRIu32 "\n",
                       encoder_count_text, encoder_delta, reel_rpm, motor_state, fault_code);
    if ((written < 0) || ((size_t)written >= buffer_size))
    {
        buffer[0] = '\0';
        return 0U;
    }
    return (size_t)written;
}
