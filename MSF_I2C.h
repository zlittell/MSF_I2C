#ifndef MSF_I2C_H_
#define MSF_I2C_H_

#include <stdbool.h>

void init_i2c(void);



bool i2c_start(uint8_t);
bool i2c_write(uint8_t);
void i2c_stop(void);


#endif /* MSF_I2C_H_ */