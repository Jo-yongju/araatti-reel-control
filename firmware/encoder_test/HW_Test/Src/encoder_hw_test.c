#include "encoder_hw_test.h"

volatile uint16_t encoder_raw_count   = 0;
volatile int32_t  encoder_total_count = 0;
volatile int16_t  encoder_delta       = 0;
volatile int8_t   encoder_direction   = 0;

static TIM_HandleTypeDef *encoder_htim = NULL;
static uint16_t encoder_prev_count = 0;

HAL_StatusTypeDef EncoderHwTest_Init(TIM_HandleTypeDef *htim)
{
    if (htim == NULL)
    {
        return HAL_ERROR;
    }

    encoder_htim = htim;

    if (HAL_TIM_Encoder_Start(encoder_htim, TIM_CHANNEL_ALL) != HAL_OK)
    {
        return HAL_ERROR;
    }

    EncoderHwTest_Reset();

    return HAL_OK;
}

void EncoderHwTest_Reset(void)
{
    if (encoder_htim == NULL)
    {
        return;
    }

    __HAL_TIM_SET_COUNTER(encoder_htim, 0);

    encoder_prev_count  = 0;
    encoder_raw_count   = 0;
    encoder_total_count = 0;
    encoder_delta       = 0;
    encoder_direction   = 0;
}

void EncoderHwTest_Update(void)
{
    uint16_t now;

    if (encoder_htim == NULL)
    {
        return;
    }

    now = (uint16_t)__HAL_TIM_GET_COUNTER(encoder_htim);

    /*
     * TIM4는 16-bit counter.
     * 손으로 축을 돌리는 이번 시험에서는
     * 두 Update 사이 변화량이 +/-32767을 넘지 않으므로
     * 65535 -> 0 / 0 -> 65535 wrap-around도 처리된다.
     */
    encoder_delta = (int16_t)(now - encoder_prev_count);

    encoder_prev_count = now;
    encoder_raw_count = now;

    encoder_total_count += encoder_delta;

    if (encoder_delta > 0)
    {
        encoder_direction = 1;
    }
    else if (encoder_delta < 0)
    {
        encoder_direction = -1;
    }
    else
    {
        encoder_direction = 0;
    }
}
