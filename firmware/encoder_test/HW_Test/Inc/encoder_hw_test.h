#ifndef ENCODER_HW_TEST_H
#define ENCODER_HW_TEST_H

#include "stm32g4xx_hal.h"
#include <stdint.h>

/* CubeIDE Live Expressions에서 볼 측정값 */
extern volatile uint16_t encoder_raw_count;
extern volatile int32_t  encoder_total_count;
extern volatile int16_t  encoder_delta;
extern volatile int8_t   encoder_direction;

/* TIM Encoder 시작 */
HAL_StatusTypeDef EncoderHwTest_Init(TIM_HandleTypeDef *htim);

/* 주기적으로 호출해서 count 갱신 */
void EncoderHwTest_Update(void);

/* 측정 시작점을 0으로 재설정 */
void EncoderHwTest_Reset(void);

#endif
