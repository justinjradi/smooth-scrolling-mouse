/*
 * pmw3610.c
 *
 *  Created on: Dec 18, 2025
 *      Author: Justin
 */

#include "stm32f4xx_hal.h"
#include "pmw3610.h"
#include "delay.h"

// PRIVATE DEFINES

#define SPI_DEFAULT_TIMEOUT			100
#define NCS_SELECT							0
#define NCS_DESELECT						1

// PRIVATE FUNCTIONS

HAL_StatusTypeDef read_reg(struct pmw3610_dev *dev_p, uint8_t address, uint8_t *data_p,
		uint8_t size)
{
	if ((dev_p == NULL) || (dev_p->NCS_port_p == NULL) || (dev_p->hspi_p == NULL))
	{
		return HAL_ERROR;
	}
	if (data_p == NULL)
	{
		return HAL_ERROR;
	}
	HAL_StatusTypeDef result = HAL_ERROR;
	// Select chip
	HAL_GPIO_WritePin(dev_p->NCS_port_p, dev_p->NCS_pin, NCS_SELECT);
	delay_us(150);
	// Transmit first byte to indicate address and operation type
	uint8_t first_byte[1] = {address & 0b01111111};	// Set MSB to 0 to indicate read operation
	result = HAL_SPI_Transmit(dev_p->hspi_p, first_byte, 1, SPI_DEFAULT_TIMEOUT);
	if (result != HAL_OK)
	{
		return result;
	}
	// Receive second byte with data
	result = HAL_SPI_Receive(dev_p->hspi_p, data_p, size, SPI_DEFAULT_TIMEOUT);
	// Deselect chip and return
	HAL_GPIO_WritePin(dev_p->NCS_port_p, dev_p->NCS_pin, NCS_DESELECT);
	delay_us(150);
	return result;
}

HAL_StatusTypeDef write_reg_helper(struct pmw3610_dev *dev_p, uint8_t address, uint8_t value)
{
	if ((dev_p == NULL) || (dev_p->NCS_port_p == NULL) || (dev_p->hspi_p == NULL))
	{
		return HAL_ERROR;
	}
	HAL_StatusTypeDef result = HAL_ERROR;
	// Select chip
	HAL_GPIO_WritePin(dev_p->NCS_port_p, dev_p->NCS_pin, NCS_SELECT);
	delay_us(150);
	// Transmit first byte to indicate address and operation type
	uint8_t first_byte[1] = {address | 0b10000000};		// Set MSB to 1 to indicate write operation
	result = HAL_SPI_Transmit(dev_p->hspi_p, first_byte, 1, SPI_DEFAULT_TIMEOUT);
	if (result != HAL_OK)
	{
		return result;
	}
	// Transmit second byte with data
	uint8_t data_p[1] = {value};
	result = HAL_SPI_Transmit(dev_p->hspi_p, data_p, 1, SPI_DEFAULT_TIMEOUT);
	// Deselect chip and return
	HAL_GPIO_WritePin(dev_p->NCS_port_p, dev_p->NCS_pin, NCS_DESELECT);
	delay_us(150);
	return result;
}

HAL_StatusTypeDef write_reg(struct pmw3610_dev *dev_p, uint8_t address, uint8_t value)
{
	if (dev_p == NULL)
	{
		return HAL_ERROR;
	}
	HAL_StatusTypeDef result = HAL_ERROR;
	// Turn on SPI clock and wait 300 µs as required by datasheet
	const uint8_t CLK_ON_CMD = 0xBA;
	const uint8_t CLK_CTRL_REG = 0x41;
	result = write_reg_helper(dev_p, CLK_CTRL_REG, CLK_ON_CMD);
	if (result != HAL_OK)
	{
		return result;
	}
	delay_us(300);
	// Execute actual write
	result = write_reg_helper(dev_p, address, value);
	if (result != HAL_OK)
	{
		return result;
	}
	// Turn off SPI clock and as required by datasheet
	const uint8_t CLK_OFF_CMD = 0xB5;
	result = write_reg_helper(dev_p, CLK_CTRL_REG, CLK_OFF_CMD);
	return result;
}

// PUBLIC FUNCTIONS

// Assumes SPI and GPIO peripherals are already initialized; SPI CLK frequency ~= 100 kHz
HAL_StatusTypeDef pmw3610_init(struct pmw3610_dev *dev_p)
{
	if ((dev_p == NULL) || (dev_p->NCS_port_p == NULL))
	{
		return HAL_ERROR;
	}
	HAL_GPIO_WritePin(dev_p->NCS_port_p, dev_p->NCS_pin, NCS_DESELECT);
	delay_ms(100);
	// Power-up from shutdown
	const uint8_t PWR_UP_REG = 0x3A;
	const uint8_t PWR_UP_CMD = 0x96;
	write_reg(dev_p, PWR_UP_REG, PWR_UP_CMD);
	delay_ms(100);
	// Clear observation1 register as specified by datasheet
	const uint8_t OBS1_REG = 0x2D;
	const uint8_t OBS1_CLEAR_CMD = 0x00;
	write_reg(dev_p, OBS1_REG, OBS1_CLEAR_CMD);
	delay_ms(100);
	// Check observation1 register as specified by datasheet
	uint8_t obs1_value = 0x00;	// dummy value to be overwritten
	read_reg(dev_p, OBS1_REG, &obs1_value, 1);
	if ((obs1_value & 0x0F) != 0x0F)
	{
		return HAL_ERROR;		// Bits 3-0 should be '1'
	}
	// Check product ID
	const uint8_t PRODUCT_ID_REG = 0x00;
	const uint8_t EXPECTED_PRODUCT_ID = 0x3E;
	uint8_t actual_product_ID = 0x00;	// dummy value to be overwritten
	read_reg(dev_p, PRODUCT_ID_REG, &actual_product_ID, 1);
	if (actual_product_ID != EXPECTED_PRODUCT_ID)
	{
		return HAL_ERROR;
	}
	// Read registers 0x01, 0x02, 0x03, and 0x04 as specified by datasheet
	uint8_t data_p[1] = {0x00};
	read_reg(dev_p, 0x01, data_p, 1);
	read_reg(dev_p, 0x02, data_p, 1);
	read_reg(dev_p, 0x04, data_p, 1);
	read_reg(dev_p, 0x05, data_p, 1);
	// Write to registers 0x11, 0x1B, 0x1C, and 0x1D with required values
	const uint8_t PERFORMANCE_REG = 0x11;
	const uint8_t PERFORMANCE_INIT_VAL = 0x0D;
	write_reg(dev_p, PERFORMANCE_REG, PERFORMANCE_INIT_VAL);
	//
	const uint8_t RUN_DOWNSHIFT_REG = 0x1B;
	const uint8_t RUN_DOWNSHIFT_INIT_VAL = 0x04;
	write_reg(dev_p, RUN_DOWNSHIFT_REG, RUN_DOWNSHIFT_INIT_VAL);
	//
	const uint8_t REST1_RATE_REG = 0x1C;
	const uint8_t REST1_RATE_INIT_VAL = 0x04;
	write_reg(dev_p, REST1_RATE_REG, REST1_RATE_INIT_VAL);
	//
	const uint8_t REST1_DOWNSHIFT_REG = 0x1D;
	const uint8_t REST1_DOWNSHIFT_INIT_VAL = 0x0F;
	write_reg(dev_p, REST1_DOWNSHIFT_REG, REST1_DOWNSHIFT_INIT_VAL);
	return HAL_OK;
}

// Get values using burst mode
HAL_StatusTypeDef pmw3610_get_values(struct pmw3610_dev *dev_p, struct pmw3610_values *vals_p)
{
	if ((dev_p == NULL) || (vals_p == NULL))
	{
		return HAL_ERROR;
	}
	HAL_StatusTypeDef result = HAL_ERROR;
	vals_p->valid = PMW3610_MOT_REG_INVAL;
	uint8_t burst_read_vals[4] = {0};
	const uint8_t BURST_READ_REG = 0x12;
	// Read from burst read register
	result = read_reg(dev_p, BURST_READ_REG, burst_read_vals, 4);
	if (result != HAL_OK)
	{
		return HAL_ERROR;
	}
	// First 4 bits are MOTION, DELTA_X_L, DELTA_Y_L, and DELTA_XY_H
	uint8_t motion_reg = burst_read_vals[0];
	if (!(motion_reg & 0b10000000))
	{
		// No motion has occured; set valid to 0 and return
		vals_p->valid = PMW3610_MOT_REG_INVAL;
		return HAL_OK;
	}
	else
	{
		vals_p->valid = PMW3610_MOT_REG_VALID;
	}
	uint8_t dx_low_byte = burst_read_vals[1];
	uint8_t dy_low_byte = burst_read_vals[2];
	uint8_t dxy_high_nibbles = burst_read_vals[3];
	/*
	 * 	Data Conversion
	 *
	 * 	Sensor outputs 12-bit signed integers x and y where:
	 *  DELTA_X_L is the low byte of x and bits 7-4 of DELTA_XY_H form bits 8-11 of x,
	 *  DELTA_Y_L is the low byte of y and bits 0-3 of DELTA_XY_H form bits 8-11 of y
	 */
	// 1. Shift high nibbles of x and y to bits 8-11
	uint16_t dx_high_nibble = (uint16_t)(0xF0 & dxy_high_nibbles) << 4;
	uint16_t dy_high_nibble = (uint16_t)(0x0F & dxy_high_nibbles) << 8;
	// 2. Combine with low bytes to form 12-bit representation
	uint16_t dx_12b = dx_high_nibble | (uint16_t)dx_low_byte;
	uint16_t dy_12b = dy_high_nibble | (uint16_t)dy_low_byte;
	// 3. Get bit 11, corresponding to the signed bit of the 12-bit value
	uint16_t dx_sign_bit = (dx_12b & 0b0000100000000000) >> 11;
	uint16_t dy_sign_bit = (dy_12b & 0b0000100000000000) >> 11;
	// 4. Perform sign extension by copying to bits 12, 13, 14, and 15
	uint16_t dx_sign_extended;
	uint16_t dy_sign_extended;
	dx_sign_extended = dx_12b | (dx_sign_bit << 12) | (dx_sign_bit << 13) |
			(dx_sign_bit << 14) | (dx_sign_bit << 15);
	dy_sign_extended = dy_12b | (dy_sign_bit << 12) | (dy_sign_bit << 13) |
			(dy_sign_bit << 14) | (dy_sign_bit << 15);
	// 5. Typecast and store now that conversion is complete
	vals_p->delta_x = (int16_t)dx_sign_extended;
	vals_p->delta_y = (int16_t)dy_sign_extended;
	return HAL_OK;
}
