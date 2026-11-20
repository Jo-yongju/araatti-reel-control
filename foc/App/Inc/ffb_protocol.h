#ifndef FFB_PROTOCOL_H
#define FFB_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/*
 * FFB Protocol V1
 *
 * Valid commands:
 *
 *   R,000
 *   ...
 *   R,100
 *
 *   STOP
 *
 * '\n' 또는 "\r\n"이 뒤에 붙어 있어도 허용한다.
 *
 * Examples:
 *   "R,050"
 *   "R,050\n"
 *   "R,050\r\n"
 *   "STOP"
 *   "STOP\n"
 */

typedef enum
{
    FFB_CMD_INVALID = 0,
    FFB_CMD_RESISTANCE,
    FFB_CMD_SIGNED,
    FFB_CMD_STOP

} FFB_CommandType;

typedef struct
{
    FFB_CommandType type;

    /*
     * Valid only when type == FFB_CMD_RESISTANCE
     *
     * 0   -> Resistance 0.00
     * 50  -> Resistance 0.50
     * 100 -> Resistance 1.00
     */
    uint8_t resistance_percent;
    int16_t signed_percent; /* -100 to +100; R,DDD remains positive */

} FFB_Command;

/*
 * Parse one complete protocol line.
 *
 * Parameters:
 *   data    : received data buffer
 *   length  : number of valid bytes in data
 *             (do not include terminating '\0')
 *   out_cmd : parsed command output
 *
 * Return:
 *   true  -> valid command
 *   false -> invalid command
 */
bool FFB_Protocol_ParseLine(const char *data,
                            size_t length,
                            FFB_Command *out_cmd);



/*
 * Recommended minimum buffer size for telemetry.
 */
#define FFB_TELEMETRY_BUFFER_SIZE    (96U)


/*
 * Format telemetry message.
 *
 * Output format:
 *
 *   E,<encoder_count>,<motor_rpm>,<fault_code>\n
 *
 * Example:
 *
 *   E,12540,320,0\n
 *
 * Returns:
 *   number of characters written, excluding '\0'
 *
 *   0 -> invalid argument or buffer too small
 */
size_t FFB_Protocol_FormatTelemetry(char *buffer,
                                    size_t buffer_size,
                                    int64_t encoder_count,
                                    int32_t motor_rpm,
                                    uint32_t fault_code);



/* Signed command: F,+000 through F,+100 or F,-000 through F,-100.
 * A complete UART command is terminated by LF (optional preceding CR).
 */
size_t FFB_Protocol_FormatReelTelemetry(char *buffer, size_t buffer_size,
                                        int64_t encoder_count, int32_t encoder_delta,
                                        int32_t reel_rpm, uint32_t motor_state,
                                        uint32_t fault_code);

#endif /* FFB_PROTOCOL_H */
