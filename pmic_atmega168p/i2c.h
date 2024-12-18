#pragma once

// pin SDA: PC4
// pin SCL: PC5

#define I2C_INIT_PORTB_MASK    0
#define I2C_INIT_PORTC_MASK    0
//#define I2C_INIT_PORTC_MASK    (_BV(4) | _BV(5))
#define I2C_INIT_PORTD_MASK    0

#define I2C_INIT_DDRB_MASK     0
#define I2C_INIT_DDRC_MASK     0
#define I2C_INIT_DDRD_MASK     0

void i2c_slave_init(void);
void i2c_slave_regs_write(uint8_t reg, uint8_t val);
uint8_t i2c_slave_regs_read(uint8_t reg);
