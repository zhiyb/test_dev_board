#include <stdint.h>
#include "dev.h"
#include "key.h"

typedef enum {
    I2cRegId = 0,
    I2cRegPwrKey,
} i2c_reg_t;

static uint8_t regs[256];

uint8_t i2c_slave_regs_read(uint8_t reg)
{
    switch (reg) {
    case I2cRegId:
        return 0x5d;
    case I2cRegPwrKey:
        return dev_pwr_state() | (key_state() << 4);
    }
    return 0xdd;
}

void i2c_slave_regs_write(uint8_t reg, uint8_t val)
{
    switch (reg) {
    case I2cRegPwrKey:
        dev_pwr_en(val);
        break;
    }
}
