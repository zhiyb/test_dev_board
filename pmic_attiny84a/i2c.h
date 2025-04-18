#pragma once

// pin SDA: PA6
// pin SCL: PA4

#define I2C_DDR     DDRA
#define I2C_PORT    PORTA
#define I2C_PIN     PINA
#define I2C_SDA     _BV(6)
#define I2C_SCL     _BV(4)

#define I2C_INIT_PORTA_MASK    0
#define I2C_INIT_PORTB_MASK    0

#define I2C_INIT_DDRA_MASK     0
#define I2C_INIT_DDRB_MASK     0

void i2c_slave_init(void);
void i2c_slave_regs_write(uint8_t reg, uint8_t val);
uint8_t i2c_slave_regs_read(uint8_t reg);

void i2c_master_init(void);
void i2c_master_init_resync(void);
bool i2c_master_write(uint8_t addr, const uint8_t *src, uint8_t len);
bool i2c_master_read(uint8_t addr, uint8_t *dst, uint8_t len);

void i2c_deinit(void);
bool i2c_enabled(void);
