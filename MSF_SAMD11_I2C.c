#include <sam.h>
#include "MSF_I2C.h"

/*
*	This library uses the none shifted 7 bit address as an input
*	then shifts and ORs read/write bit.
*/

/*
	create a function to check for SB and MB intflags
	this function should timeout correctly
*/

//init
void init_i2c(void)
{
	//PM
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0;
	
	//GCLK
	GCLK->CLKCTRL.reg =
	(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_SERCOM0_CORE | GCLK_CLKCTRL_GEN_GCLK0);
	
	//Master Mode with 400KHz speed
	SERCOM0->I2CM.CTRLA.reg = (SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_SPEED(0));
	
	//Set baud rate, Should be 400KHz at 8MHz GCLK
	SERCOM0->I2CM.BAUD.bit.BAUD = 5;
	SERCOM0->I2CM.BAUD.bit.BAUDLOW = 5;
	
	//Enable Smart Mode
	SERCOM0->I2CM.CTRLB.reg = (SERCOM_I2CM_CTRLB_SMEN);
	
	//IO lines
	PORT->Group[0].WRCONFIG.reg = 
		(PORT_WRCONFIG_WRPINCFG | PORT_WRCONFIG_WRPMUX | PORT_WRCONFIG_PMUX(0x2) | PORT_WRCONFIG_PMUXEN | PORT_PA14 | PORT_PA15);
	
	//Enable I2C
	while(SERCOM0->I2CM.SYNCBUSY.bit.ENABLE);
	SERCOM0->I2CM.CTRLA.bit.ENABLE = 1;
	
	//Force Idle Bus State
	while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
	SERCOM0->I2CM.STATUS.bit.BUSSTATE = 0x1;
	
	//Enable interrupts for master on bus and slave on bus
	//SERCOM0->I2CM.INTENSET.reg = SERCOM_I2CM_INTENSET_MB | SERCOM_I2CM_INTENSET_SB;
}

uint8_t i2c_send(uint8_t i2caddr, uint8_t *data, uint8_t size)
{
	// Set bus to ACK received data
	SERCOM0->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
	
	//Load write address
	while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
	SERCOM0->I2CM.ADDR.reg = ((i2caddr << 1) | 0);
	
	while(!(SERCOM0->I2CM.INTFLAG.bit.MB));
	SERCOM0->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB;	
	
	uint8_t result = 0;
	for (int i = 0; i < size; i++)
	{
		while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
		SERCOM0->I2CM.DATA.reg = (uint8_t)*data;
		while(!(SERCOM0->I2CM.INTFLAG.bit.MB));
		SERCOM0->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB;
		data++;
		result++;
	}
	while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
	SERCOM0->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(3);
	
	return result;
}

void simpleTest()
{
	uint8_t data[10] = {0x80, 0x01, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xAA};
	// Set bus to ACK received data
	SERCOM0->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
	
	//Load write address
	while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
	SERCOM0->I2CM.ADDR.reg = ((0x60 << 1) | 0);
	
	while(!(SERCOM0->I2CM.INTFLAG.bit.MB));
	SERCOM0->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB;	
	
	for (int i = 0; i < 10; i++)
	{
		while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
		SERCOM0->I2CM.DATA.reg = data[i];
		while(!(SERCOM0->I2CM.INTFLAG.bit.MB));
		SERCOM0->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB;
	}
	
	while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
	SERCOM0->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(3);
}

uint8_t i2c_read(uint8_t i2caddr, uint8_t *data, uint8_t size)
{
	uint8_t result = 0;
	SERCOM0->I2CM.ADDR.reg = ((i2caddr << 1) | 1);
	
	//Wait for byte to arrive from peripheral
	while(!(SERCOM0->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_SB));
	
	//If NACK'd put in stop state and fail out
	if (SERCOM0->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK)
	{
		SERCOM0->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(3);
		return result;
	}
	
	//Set controller to ACK reads
	SERCOM0->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
	
	for (uint8_t i = 0; i < (size - 1); i++)
	{
		*data = SERCOM0->I2CM.DATA.reg;
		data++;
		result++;
		while(!(SERCOM0->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_SB));
	}
	
	if (size)
	{
		//NACK the last byte read to end request, idle bus.
		SERCOM0->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_ACKACT;
		SERCOM0->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(3);
		*data = SERCOM0->I2CM.DATA.reg;
		result++;
	}
	
	return result;
}

//WaitForIdle
//read byte
//reset
//repeated start
//read with I2C_RESULT
//get_i2c_data_pointer
//send_i2c_data
//get_i2c_data_2byte
//get_i2c_data_2byte_pointer

//send data with length
//receive data with length also allow pointers or have another function
