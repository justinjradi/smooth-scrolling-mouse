/*
 * pmw3610.h
 *
 *  Created on: Dec 18, 2025
 *      Author: Justin
 */

#ifndef INC_PMW3610_H_
#define INC_PMW3610_H_

#include "stm32f4xx_hal.h"

// Public defines

// For motion register
#define PMW3610_MOT_REG_VALID		1
#define PMW3610_MOT_REG_INVAL		0

// For motion pin
#define PMW3610_MOT_PIN_SET 		0
#define PMW3610_MOT_PIN_RESET		1

// For nreset pin
#define PMW3610_NRST_SET				1
#define PMW3610_NRST_RESET			0

struct pmw3610_dev
{
	SPI_HandleTypeDef *hspi_p;
	GPIO_TypeDef* NCS_port_p;
	uint16_t NCS_pin;
};

struct pmw3610_values
{
	int16_t delta_x;
	int16_t delta_y;
	uint8_t valid;
};

// Public functions

HAL_StatusTypeDef pmw3610_init(struct pmw3610_dev *dev_p);
HAL_StatusTypeDef pmw3610_get_values(struct pmw3610_dev *dev_p, struct pmw3610_values *vals_p);

#endif /* INC_PMW3610_H_ */
