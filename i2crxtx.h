//-----------------------------------------------------------------------------
//   i2c_init  - initialize I2C functions
//   i2c_start - issue Start condition
//   i2c_repStart- issue Repeated Start condition
//   i2c_stop  - issue Stop condition
//   i2c_read(x) - receive unsigned char - x=0, don't acknowledge - x=1, acknowledge
//   i2c_write - write unsigned char - returns ACK
//  
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------//
// Type Definitions
//----------------------------------------------------------------------------//
typedef struct
  {
    unsigned char tx_chk;
    unsigned char data;
  }I2C_RESULT;
  
  typedef struct
  {
    unsigned char tx_chk;
    unsigned char data0;
    unsigned char data1;
  }I2C_RESULT_2BYTE;
  
//----------------------------------------------------------------------------//
// Function Prototypes
//----------------------------------------------------------------------------//
void i2c_init(void);
I2C_RESULT get_i2c_data(unsigned char);
I2C_RESULT get_i2c_data_pointer(unsigned char, unsigned char);
I2C_RESULT_2BYTE get_i2c_data_2byte(unsigned char);
I2C_RESULT_2BYTE get_i2c_data_2byte_pointer(unsigned char, unsigned char);
unsigned char send_i2c_data(unsigned char, unsigned char, unsigned char);
unsigned char i2c_trans_ret(unsigned char,unsigned char);
unsigned char i2c_trans_pos(unsigned char,unsigned char);
//void i2c_transmit(unsigned char,unsigned char);
//void i2c_transmit2(unsigned char,unsigned char,unsigned char);
//unsigned char i2c_transmit2_ret(unsigned char,unsigned char,unsigned char);
//unsigned char i2c_transmit5(unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char);
//void RS485_wait(void);

//Function prototypes needed to be exposed for using external device libraries
unsigned char i2c_start(void);
void i2c_stop(void);
unsigned char i2c_write(unsigned char);
I2C_RESULT i2c_read(unsigned char);
unsigned char i2c_waitForIdle(void);
void reset_i2c(void);

//----------------------------------------------------------------------------//
// Variables
//----------------------------------------------------------------------------//

