
//-----------------------------------------------------------------------------
// Hardware I2C single master routines for PIC18
//   with hardware Master mode.
//
// i2c_init	      - initialize I2C
// i2c_start	  - issue Start condition
// i2c_repStart   - issue Repeated Start condition
// i2c_write	  - write char - returns ACK
// i2c_read(x)	  - receive char - x=0, Issue a don't acknowledge (NACK)- x=1,
//                  Issue an acknowledge (ACK)
// i2c_stop 	  - issue Stop condition
//
//-----------------------------------------------------------------------------
#include <xc.h>
#include "main.h"
#include "i2crxtx.h"
#include "mcc_generated_files/mcc.h"

//----------------------------------------------------------------------------//
// Local Function Prototypes
//----------------------------------------------------------------------------//
//static void reset_i2c(void);
//static void i2c_stop(void);
//static unsigned char i2c_waitForIdle(void);
//static unsigned char i2c_start(void);
//static I2C_RESULT i2c_read(unsigned char);
//static unsigned char i2c_write(unsigned char);
//static unsigned char Board_Check(unsigned char);


//-----------------------------------------------------------------------------
void i2c_init()
{
  SCL_TRIS = 1;             // set SCL pin as input
  SDA_TRIS = 1;             // set SDA pin as input
  SSPCON1 = 0b00101000;     //SSPEN and I2C master mode, clock = FOSC/(4 * (SSPxADD + 1))
  SSPCON2 = 0;

  SSPADD  = 150;
  
  SSPSTAT = 0b10000000;     //Disable slew rate. disable SMBus inputs

  //CKE     = 1;             // Data transmitted on falling edge of SCK
  //SMP     = 1;             // disable slew rate control
  SSPIF   = 0;             // clear SSPIF interrupt flag
  BCLIF   = 0;             // clear bus collision flag
}
//-----------------------------------------------------------------------------
// Function Name:  i2c_waitForIdle()
//-----------------------------------------------------------------------------
// Changes:
//   SSP2CON2 wasn't shifted before adding RW2.  VERY small chance of losing data.
//   Function now returns 1 for idle and 0 for not idle
//-----------------------------------------------------------------------------
unsigned char i2c_waitForIdle(void)
{
  unsigned long  i2c_wait;
  unsigned char i2c_idle_status;
  static unsigned char i2c_fail = 0;

  i2c_wait = 0;
  i2c_idle_status = (((SSPCON2 & 0x1F)<<1) + RW);

  while(i2c_idle_status && (i2c_wait < 50000))
    {
      i2c_idle_status = (((SSPCON2 & 0x1F)<<1) + RW);
      i2c_wait++;
      CLRWDT();
    }

  if(i2c_wait == 50000)     // Idle timeout
    {
      i2c_fail++;
      if(i2c_fail >= 3)
       {
         i2c_fail = 0;
         reset_i2c();
       }
      return 0;
    }
  else
    {
      i2c_fail = 0;
      return 1;
    }
}

//-----------------------------------------------------------------------------
void reset_i2c(void)  // Force the I2C to reset.  Temporary fix until real problem is solved!
{
  unsigned int reset_i;

  SSPEN = 0;
  SSPCON2 = 0;
  SDA_TRIS = 0;
  SCL_TRIS = 0;

  SDA = 0;
  SCL = 0;
  reset_i = 5000;
  while(reset_i--){NOP();}

  SDA = 1;
  __delay_us(75);

  reset_i = 9;
  while(reset_i--)
    {
      SCL = 0;
      __delay_us(75);
      SCL = 1;
      __delay_us(75);
    }
  SCL = 0;
  __delay_us(75);

  SCL_TRIS = 1;             // set SCL pin as input
  SDA_TRIS = 1;             // set SDA pin as input
  SSPEN = 1;

  SEN = 1;
  SSPBUF = 0x55;
  PEN = 1;
}
//-----------------------------------------------------------------------------
// Function name:  i2c_start
//-----------------------------------------------------------------------------
//  Returns 1 if start initiated.
//  Returns 0 is bus is not idle.
//-----------------------------------------------------------------------------
unsigned char i2c_start(void)
{
  if(i2c_waitForIdle() && !START)       // Bus Idle, Start not received last
    {
      SEN = 1;                          // Initiate START conditon.
      __delay_us(10);
      return 1;
    }
  else
    {return 0;}
}
//-----------------------------------------------------------------------------
// Function name:  i2c_repStart
//-----------------------------------------------------------------------------
//  Returns 1 if start initiated.
//  Returns 0 is bus is not idle.
//-----------------------------------------------------------------------------
unsigned char i2c_repStart(void)
{
  if(i2c_waitForIdle())       // Bus Idle, Start not received last
    {
      RSEN = 1;                          // Initiate RESTART conditon.
      __delay_us(10);
      return 1;
    }
  else{return 0;}
}
//-----------------------------------------------------------------------------
unsigned char i2c_write(unsigned char i2cWriteData)
{
  unsigned char temp_chk = 0;

  if(i2c_waitForIdle())           // Wait for the idle condition
    {
      __delay_us(10);                // wait 10 uS
      SSPBUF = i2cWriteData;     // Load SSPBUF with i2cWriteData (the value to be transmitted)
      temp_chk = i2c_waitForIdle(); // Wait for the idle condition
      return (!ACKSTAT);         // ACKSTAT returns '0' if transmission is acknowledged
    }
  else
    {return 0;}
}
//-----------------------------------------------------------------------------
I2C_RESULT i2c_read(unsigned char ack)
{
  I2C_RESULT temp_read;

  temp_read.data = 0;
  temp_read.tx_chk = 0;

  temp_read.tx_chk = i2c_waitForIdle();   // Wait for the idle condition

  __delay_us(10);                  // wait 10 uS

  RCEN = 1;                    // Enable receive mode

  if(i2c_waitForIdle())         // Wait for the idle condition
    {temp_read.data = SSPBUF;} // Read SSPBUF and put it in i2cReadData

  temp_read.tx_chk = i2c_waitForIdle();   // Wait for the idle condition

  if(ack)                       // if ack=1 (from i2c_read(ack))
    {ACKDT = 0;}               // then transmit an Acknowledge
  else
    {ACKDT = 1;}               // otherwise transmit a Not Acknowledge

  ACKEN = 1;                   // send acknowledge sequence

  return(temp_read);            // return the value read from SSPBUF

}
//-----------------------------------------------------------------------------
void i2c_stop(void)
{
  if(i2c_waitForIdle())	    // Wait for the idle condition
    {
      __delay_us(10);          // wait 10 uS
      PEN = 1;             // Initiate STOP condition
    }
}
//-----------------------------------------------------------------------------
I2C_RESULT get_i2c_data(unsigned char i2caddr)
{
  unsigned char output = 0;
  I2C_RESULT temp_get;

  temp_get.data = 0;
  temp_get.tx_chk = 0;

  if(i2c_start())           // Bus idle and start initiated
    {
      if(i2c_write(i2caddr + 1u))
        {
          temp_get = i2c_read(0);
        }
      else                  // No Ack.  Retry
        {
          i2c_stop();       // Stop bus and wait
          __delay_us(75);

          if(i2c_start())   // Bus idle and start initiated
            {
              if(i2c_write(i2caddr + 1u))
                {
                  temp_get = i2c_read(0);
                }
            }
        }

      i2c_stop();
    }

  return temp_get;            // data
}
//-----------------------------------------------------------------------------
I2C_RESULT get_i2c_data_pointer(unsigned char i2caddr, unsigned char address) 
{
    I2C_RESULT temp_get;
    unsigned char retry = 10;

    temp_get.data = 0;
    temp_get.tx_chk = 0;

    while (retry) 
    {
        if (i2c_start()) 
        {
            if (i2c_write(i2caddr)) 
            {
                if (i2c_write(address)) 
                {
                    i2c_repStart();
                    if (i2c_write(i2caddr + 1u)) 
                    {
                            temp_get = i2c_read(0);
                    }
                }
            }
            i2c_stop();
        } 
        else 
        {
            reset_i2c();
        }

        if (!temp_get.tx_chk) 
        {
            retry--;
        } 
        else 
        {
            retry = 0;
        }
    }

    return temp_get;
}
//-----------------------------------------------------------------------------
unsigned char send_i2c_data(unsigned char i2caddr, unsigned char registeraddr, unsigned char data)
{
    unsigned char acked = 0;
    unsigned char retry = 10;

    while (retry)
    {
        if (i2c_start())
        {
            if (i2c_write(i2caddr))
            {
                if (i2c_write(registeraddr))
                {
                    acked = i2c_write(data);
                }
            }

            i2c_stop();
        }
        else {reset_i2c();}

        if(!acked) {retry--;}
        else {retry=0;}
    }

    return acked;
}
//-----------------------------------------------------------------------------
I2C_RESULT_2BYTE get_i2c_data_2byte(unsigned char i2caddr)
{
    I2C_RESULT temp_get;
    I2C_RESULT_2BYTE full_get;
    unsigned char retry = 10;

    temp_get.data = 0;
    temp_get.tx_chk = 0;

    full_get.data0 = 0;
    full_get.data1 = 0;
    full_get.tx_chk = 0;

    while (retry)
    {
        if (i2c_start()) {
            if (i2c_write(i2caddr + 1u)) {
                temp_get = i2c_read(1);
                if (temp_get.tx_chk) {
                    full_get.data0 = temp_get.data;
                    temp_get = i2c_read(0);
                    if (temp_get.tx_chk) {
                        full_get.data1 = temp_get.data;
                        full_get.tx_chk = 1;
                    }
                }
            }
            i2c_stop();
        } else {
            reset_i2c();
        }

        if (!full_get.tx_chk) {
            retry--;
        } else {
            retry = 0;
        }
    }

    return full_get;
}
//-----------------------------------------------------------------------------
I2C_RESULT_2BYTE get_i2c_data_2byte_pointer(unsigned char i2caddr, unsigned char address) 
{
    I2C_RESULT temp_get;
    I2C_RESULT_2BYTE full_get;
    unsigned char retry = 10;

    temp_get.data = 0;
    temp_get.tx_chk = 0;

    full_get.data0 = 0;
    full_get.data1 = 0;
    full_get.tx_chk = 0;

    while (retry) 
    {
        if (i2c_start()) 
        {
            if (i2c_write(i2caddr)) 
            {
                if (i2c_write(address)) 
                {
                    i2c_repStart();
                    if (i2c_write(i2caddr + 1u)) 
                    {
                        temp_get = i2c_read(1);
                        if (temp_get.tx_chk) 
                        {
                            full_get.data0 = temp_get.data;
                            temp_get = i2c_read(0);
                            if (temp_get.tx_chk) 
                            {
                                full_get.data1 = temp_get.data;
                                full_get.tx_chk = 1;
                            }
                        }
                    }
                }
            }
            i2c_stop();
        } 
        else 
        {
            reset_i2c();
        }

        if (!full_get.tx_chk) 
        {
            retry--;
        } 
        else 
        {
            retry = 0;
        }
    }

    return full_get;
}

