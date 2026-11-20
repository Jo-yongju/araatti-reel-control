#include "ffb_protocol.h"

#include <stdio.h>
#include <inttypes.h>

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

    return true;
}


size_t FFB_Protocol_FormatTelemetry(char *buffer,
                                    size_t buffer_size,
                                    int64_t encoder_count,
                                    int32_t motor_rpm,
                                    uint32_t fault_code)
{
    int written;

    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return 0U;
    }

    written = snprintf(
        buffer,
        buffer_size,
        "E,%" PRId64 ",%" PRId32 ",%" PRIu32 "\n",
        encoder_count,
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
