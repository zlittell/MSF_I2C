#include <sam.h>
#include "i2c.h"

/*
	TODO Create a functions to check forever whiles and time out accordingly.
	This would allow us to gracefully fail some of this stuff
	Would be much easier with RTOS and threadsleeping...
*/

/**
	@brief Init I2C
	@details Function to initialize I2C bus on device.
*/
void i2c_init(void)
{
	// PM Enable SERCOM0 Clock
    PM_REGS->PM_APBCMASK |= PM_APBCMASK_SERCOM0(1);
	
	// Set SERCOM0 Core to GCLK0
    GCLK_REGS->GCLK_CLKCTRL = (GCLK_CLKCTRL_CLKEN(1) | GCLK_CLKCTRL_ID_SERCOM0_CORE | GCLK_CLKCTRL_GEN_GCLK0);
	
	// Controller Mode with 400KHz speed
    SERCOM0_REGS->I2CM.SERCOM_CTRLA = (SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_SPEED_STANDARD_AND_FAST_MODE);
	
	// Set baud rate
    // 48MHz clock @ BAUD 55 (w/ 15nS Trise) = 397,614.31Hz
    SERCOM0_REGS->I2CM.SERCOM_BAUD = SERCOM_I2CM_BAUD_BAUD(55);
	
	// Enable Smart Mode
    SERCOM0_REGS->I2CM.SERCOM_CTRLB = SERCOM_I2CM_CTRLB_SMEN(1);
	
	// Enable I2C
    while(SERCOM0_REGS->I2CM.SERCOM_SYNCBUSY&SERCOM_I2CM_SYNCBUSY_ENABLE(1));
    SERCOM0_REGS->I2CM.SERCOM_CTRLA|=SERCOM_I2CM_CTRLA_ENABLE(1);
	
	//Force Idle Bus State
	while(SERCOM0_REGS->I2CM.SERCOM_SYNCBUSY&SERCOM_I2CM_SYNCBUSY_SYSOP(1));
    SERCOM0_REGS->I2CM.SERCOM_STATUS |= SERCOM_I2CM_STATUS_BUSSTATE(0x1);
	
	//Enable interrupts for master on bus and slave on bus
	//SERCOM0->I2CM.INTENSET.reg = SERCOM_I2CM_INTENSET_MB | SERCOM_I2CM_INTENSET_SB;
}

/**
	@brief I2C Send
	@details Send array of bytes to I2C device
	@param[in] i2caddr 7 bit I2C address
	@param[in] data Data to write to I2C bus
	@param[in] len Length of the array to write
	@returns Number of bytes written
*/
uint8_t i2c_send(uint8_t i2caddr, uint8_t *data, uint8_t len)
{
	// Set bus to ACK received data
    SERCOM0_REGS->I2CM.SERCOM_CTRLB &= ~SERCOM_I2CM_CTRLB_ACKACT(1);
	
	// Send write address
    while(SERCOM0_REGS->I2CM.SERCOM_SYNCBUSY&SERCOM_I2CM_SYNCBUSY_SYSOP(1));
    SERCOM0_REGS->I2CM.SERCOM_ADDR = ((i2caddr << 1) | 0);
	
    // Wait for controller to transmit byte before moving on
    while(!(SERCOM0_REGS->I2CM.SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_MB(1)));
    SERCOM0_REGS->I2CM.SERCOM_INTFLAG = SERCOM_I2CM_INTFLAG_MB(1);  // Clear Flag
	
    // Send data
	uint8_t result = 0;
	for (int i = 0; i < len; i++)
	{
		while(SERCOM0_REGS->I2CM.SERCOM_SYNCBUSY&SERCOM_I2CM_SYNCBUSY_SYSOP(1));
        SERCOM0_REGS->I2CM.SERCOM_DATA = (uint8_t)*data;    // Send current byte of data
		while(!(SERCOM0_REGS->I2CM.SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_MB(1)));    // Wait for controller transmit
		SERCOM0_REGS->I2CM.SERCOM_INTFLAG = SERCOM_I2CM_INTFLAG_MB(1);  // Clear Flag
		data++;
		result++;
	}

	while(SERCOM0_REGS->I2CM.SERCOM_SYNCBUSY&SERCOM_I2CM_SYNCBUSY_SYSOP(1));
    SERCOM0_REGS->I2CM.SERCOM_CTRLB |= SERCOM_I2CM_CTRLB_CMD(3);
	
	return result;
}

/**
	@brief I2C Read
	@details Reads data from I2C device
	@param[in] i2caddr 7 bit I2C address
	@param[out] data Array to store read data
	@param[in] len Number of bytes to read from i2c device
	@returns Number of bytes read
*/
uint8_t i2c_read(uint8_t i2caddr, uint8_t *data, uint8_t len)
{
	uint8_t result = 0;

	// Load read address into register
	SERCOM0_REGS->I2CM.SERCOM_ADDR = ((i2caddr << 1) | 1);
	
	// Wait for peripheral response
	while(!(SERCOM0_REGS->I2CM.SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_SB(1)));
	// No need to manually clear, with smart mode enabled this will clear when we read the data or write to CTRLB CMD
	
	//If NACK'd put in stop state and fail out
	if (SERCOM0_REGS->I2CM.SERCOM_STATUS & SERCOM_I2CM_STATUS_RXNACK(1))
	{
		SERCOM0_REGS->I2CM.SERCOM_CTRLB |= SERCOM_I2CM_CTRLB_CMD(3);
		return result;
	}
	
	// Set controller to ACK after each read of the DATA register
	SERCOM0_REGS->I2CM.SERCOM_CTRLB &= ~SERCOM_I2CM_CTRLB_ACKACT(1);
	
	// Read Data of LEN, except last byte
	for (uint8_t i = 0; i < (len - 1); i++)
	{
		// Store read data and increment pointer
		*data = SERCOM0_REGS->I2CM.SERCOM_DATA;
		data++;
		result++;

		// Wait for response from peripheral
		while(!(SERCOM0_REGS->I2CM.SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_SB(1)));
	}
	
	// Have controller NACK the last byte read to signal end of request
	if (len)
	{
		SERCOM0_REGS->I2CM.SERCOM_CTRLB |= SERCOM_I2CM_CTRLB_ACKACT(1);
		*data = SERCOM0_REGS->I2CM.SERCOM_DATA;
		result++;
	}
	
	return result;
}
