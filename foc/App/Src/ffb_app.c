#include "../Inc/ffb_app.h"
#include "../Inc/ffb_protocol.h"
#include "../Inc/ffb_safety.h"
#include "../Inc/ffb_control.h"
#include "../Inc/encoder_math.h"
#include "../Inc/motor_if.h"
#include "../../Inc/main.h"
#include "../../Inc/mc_api.h"
#include "../../Inc/mc_config.h"
#include "../../Inc/mc_config_common.h"
#include "../../Inc/pmsm_motor_parameters.h"

/* Motor Control retains TIM2. This application only reads CNT. */
#if (M1_ENCODER_PPR != 1000)
#error Review reel encoder constants when MCSDK encoder PPR changes
#endif

#define UART_RING_SIZE (128U)
#define COMMAND_LINE_SIZE (24U)
#define TELEMETRY_PERIOD_MS (50U)

static UART_HandleTypeDef tiny_uart;
static uint8_t rx_byte;
static uint8_t rx_ring[UART_RING_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;
static volatile uint8_t rx_overflow;
static char command_line[COMMAND_LINE_SIZE];
static uint8_t command_length;
static uint8_t command_drop;
static FFB_Safety safety;
static FFB_Control control;
static volatile int64_t encoder_count;
static volatile int32_t telemetry_delta;
static uint32_t previous_counter;
static uint8_t counter_valid;
static MCI_State_t previous_motor_state;
static uint8_t command_active;
static uint32_t previous_telemetry_ms;

static uint32_t SaveIrq(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void RestoreIrq(uint32_t primask)
{
    if (primask == 0U) __enable_irq();
}

static void ProcessCommand(const char *data, size_t length)
{
    FFB_Command cmd;
    uint32_t primask;
    if (!FFB_Protocol_ParseLine(data, length, &cmd)) return;

    primask = SaveIrq();
    if (cmd.type == FFB_CMD_STOP)
    {
        FFB_Safety_OnStop(&safety);
        FFB_Control_Reset(&control);
        command_active = 0U;
        RestoreIrq(primask);
        MotorIf_Stop();
        return;
    }
    if ((cmd.type == FFB_CMD_RESISTANCE) || (cmd.type == FFB_CMD_SIGNED))
    {
        uint16_t magnitude = (cmd.signed_percent < 0) ?
                             (uint16_t)(-cmd.signed_percent) : (uint16_t)cmd.signed_percent;
        if (FFB_Control_SetSignedPercent(&control, cmd.signed_percent))
        {
            FFB_Safety_OnValidResistance(&safety, (uint8_t)magnitude, HAL_GetTick());
            command_active = FFB_Safety_ShouldForceZero(&safety) ? 0U : 1U;
        }
    }
    RestoreIrq(primask);
}

void FFB_App_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    MotorIf_Init();
    FFB_Safety_Init(&safety, HAL_GetTick());
    (void)FFB_Control_Init(&control, 1.0f, 1.0f);
    previous_motor_state = MC_GetSTMStateMotor1();
    previous_telemetry_ms = HAL_GetTick();

    /* PB6/PB7: STM32G431RBTx USART1 AF7. USART2 MCP is untouched. */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &gpio);

    tiny_uart.Instance = USART1;
    tiny_uart.Init.BaudRate = 115200;
    tiny_uart.Init.WordLength = UART_WORDLENGTH_8B;
    tiny_uart.Init.StopBits = UART_STOPBITS_1;
    tiny_uart.Init.Parity = UART_PARITY_NONE;
    tiny_uart.Init.Mode = UART_MODE_TX_RX;
    tiny_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    tiny_uart.Init.OverSampling = UART_OVERSAMPLING_16;
    tiny_uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    tiny_uart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    tiny_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&tiny_uart) != HAL_OK) Error_Handler();
    HAL_NVIC_SetPriority(USART1_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    if (HAL_UART_Receive_IT(&tiny_uart, &rx_byte, 1U) != HAL_OK) Error_Handler();
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&tiny_uart);
}

void FFB_App_UartIrq(void)
{
    HAL_UART_IRQHandler(&tiny_uart);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &tiny_uart)
    {
        uint8_t next = (uint8_t)((rx_head + 1U) % UART_RING_SIZE);
        if (next == rx_tail) rx_overflow = 1U;
        else { rx_ring[rx_head] = rx_byte; rx_head = next; }
        (void)HAL_UART_Receive_IT(&tiny_uart, &rx_byte, 1U);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &tiny_uart)
    {
        rx_overflow = 1U;
        (void)HAL_UART_Receive_IT(&tiny_uart, &rx_byte, 1U);
    }
}

void FFB_App_MediumFrequencyHook(void)
{
    uint32_t raw = TIM2->CNT & 0x7FFFFFFFU;
    MCI_State_t motor_state = MC_GetSTMStateMotor1();
    uint8_t alignment_rebased = ((previous_motor_state == ALIGNMENT) &&
                                 (motor_state == WAIT_STOP_MOTOR)) ? 1U : 0U;
    int32_t delta;
    /* ENC_SetMecAngle rewrites TIM2 CNT on ALIGNMENT -> WAIT_STOP_MOTOR.
     * Rebase only this sample; never write the MCSDK-owned timer ourselves.
     */
    if ((counter_valid != 0U) && (alignment_rebased == 0U))
    {
        delta = (int32_t)raw - (int32_t)previous_counter;
        if (delta > 2000) delta -= 4000;
        else if (delta < -2000) delta += 4000;
        encoder_count += delta;
        telemetry_delta += delta;
    }
    if ((previous_motor_state == STOP) && (motor_state == IDLE))
    {
        /* A completed normal stop invalidates the mechanical/electrical alignment.
         * Reinitialize only the EAC state so the next manual start aligns again.
         */
        EAC_Init(&EncAlignCtrlM1, &SpeednTorqCtrlM1,
                 &VirtualSpeedSensorM1, &ENCODER_M1);
    }

    counter_valid = 1U;
    previous_counter = raw;
    previous_motor_state = motor_state;

    if ((MotorIf_GetFaults() != 0U) || (motor_state == FAULT_NOW) ||
        (motor_state == FAULT_OVER)) FFB_Safety_OnFault(&safety);
    FFB_Safety_Update(&safety, HAL_GetTick());
    if (FFB_Safety_ShouldForceZero(&safety))
    {
        FFB_Control_Reset(&control);
        if (command_active != 0U) MotorIf_Stop();
        command_active = 0U;
    }
    else
    {
        float output = FFB_Control_Update(&control, 0.001f, false);
        MotorIf_SetSignedNormalizedCommand(output);
    }
}

void FFB_App_Poll(void)
{
    uint32_t now;
    uint8_t budget = 64U;
    while ((rx_tail != rx_head) && (budget-- != 0U))
    {
        uint8_t byte = rx_ring[rx_tail];
        rx_tail = (uint8_t)((rx_tail + 1U) % UART_RING_SIZE);
        if (rx_overflow != 0U) { command_drop = 1U; rx_overflow = 0U; }
        if (byte == '\n')
        {
            if ((command_drop == 0U) && (command_length < COMMAND_LINE_SIZE))
            {
                command_line[command_length++] = (char)byte;
                ProcessCommand(command_line, command_length);
            }
            command_drop = 0U;
            command_length = 0U;
        }
        else if (command_drop == 0U)
        {
            if (command_length < COMMAND_LINE_SIZE)
                command_line[command_length++] = (char)byte;
            else command_drop = 1U;
        }
    }

    now = HAL_GetTick();
    if ((uint32_t)(now - previous_telemetry_ms) >= TELEMETRY_PERIOD_MS)
    {
        char line[FFB_TELEMETRY_BUFFER_SIZE];
        int64_t count;
        int32_t delta;
        int32_t reel_rpm;
        size_t length;
        uint32_t primask = SaveIrq();
        count = encoder_count;
        delta = telemetry_delta;
        telemetry_delta = 0;
        RestoreIrq(primask);
        reel_rpm = (int32_t)EncoderMath_MotorRpmToReelRpm(
            MC_GetAverageMecSpeedMotor1_F());
        length = FFB_Protocol_FormatReelTelemetry(line, sizeof(line), count, delta,
                    reel_rpm, (uint32_t)MC_GetSTMStateMotor1(),
                    (uint32_t)MotorIf_GetFaults());
        if (length != 0U)
            (void)HAL_UART_Transmit(&tiny_uart, (uint8_t *)line, (uint16_t)length, 10U);
        previous_telemetry_ms = now;
    }
}
