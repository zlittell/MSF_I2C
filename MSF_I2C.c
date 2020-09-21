#include <sam.h>
#include "MSF_I2C.h"

/*
*	This library uses the none shifted 7 bit address as an input
*	then shifts and ORs read/write bit.
*/

//init
void init_i2c(void)
{
	//IO lines
	
	//I2C_SDA - PA14 PAD0
	PORT->Group[0].PMUX[7].reg = PORT_PMUX_PMUXE_C | PORT_PMUX_PMUXO_C;
	PORT->Group[0].PINCFG[14].reg = PORT_PINCFG_PMUXEN;
	PORT->Group[0].PINCFG[15].reg = PORT_PINCFG_PMUXEN;
	
	//PM
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0;
	
	//GCLK
	GCLK->CLKCTRL.reg = 
		(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_SERCOM0_CORE | GCLK_CLKCTRL_GEN_GCLK0);
		
	SERCOM0->I2CM.CTRLB.reg = (SERCOM_I2CM_CTRLB_SMEN);
	while(SERCOM0->I2CM.SYNCBUSY.reg);
	SERCOM0->I2CM.BAUD.reg = 5;	//Should be 400KHz at 8MHz GCLK
	while(SERCOM0->I2CM.SYNCBUSY.reg);
	SERCOM0->I2CM.CTRLA.reg = (SERCOM_I2CM_CTRLA_SDAHOLD(3) | SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_ENABLE);
	while(SERCOM0->I2CM.SYNCBUSY.reg);
	SERCOM0->I2CM.STATUS.reg |= SERCOM_I2CM_STATUS_BUSSTATE(1);
	while(SERCOM0->I2CM.SYNCBUSY.reg);
}

bool i2c_start(uint8_t address)
{
	SERCOM0->I2CM.ADDR.reg = ((address << 1) | 0);
	
	while(!(SERCOM0->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_MB)); //wait for send
	
	SERCOM0->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB;
	
	if (SERCOM0->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK)
	{
		//NACK failed. initiate stop
		SERCOM0->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(3);
		return false;
	}
	
	return true;
}

bool i2c_write(uint8_t dataToSend)
{
	SERCOM0->I2CM.DATA.reg = dataToSend;
	
	while(!(SERCOM0->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_MB));	//wait for send
	
	SERCOM0->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB;
	
	if(SERCOM0->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK)
	{
		SERCOM0->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(3);
		return false;
	}
	
	return true;
}

void i2c_stop(void)
{
	SERCOM0->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(3);
}
//WaitForIdle
//write byte
//read byte
//reset
//start
//repeated start
//read with I2C_RESULT
// stop
//get_i2c_data_pointer
//send_i2c_data
//get_i2c_data_2byte
//get_i2c_data_2byte_pointer

//send data with length
//receive data with length also allow pointers or have another function
