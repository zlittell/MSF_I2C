#ifndef MSF_I2C_H_
#define MSF_I2C_H_

void init_i2c(void);
uint8_t i2c_send(uint8_t, uint8_t*, uint8_t);
uint8_t i2c_read(uint8_t, uint8_t*, uint8_t);

#endif /* MSF_I2C_H_ */