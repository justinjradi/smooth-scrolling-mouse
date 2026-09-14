/*
 * delay.h
 *
 *  Created on: Dec 19, 2025
 *  		Based off code by Khaled Magdy
 */

#ifndef INC_DELAY_H_
#define INC_DELAY_H_

#include "stm32f4xx_hal.h"

uint32_t delay_init(void);

// This Function Provides Delay In Microseconds Using DWT
void delay_us(volatile uint32_t au32_microseconds);

// This Function Provides Delay In Milliseconds Using DWT
void delay_ms(volatile uint32_t au32_milliseconds);

#endif /* INC_DELAY_H_ */
