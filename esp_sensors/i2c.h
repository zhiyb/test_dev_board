#pragma once

#include <cstdint>

void i2c_init(void);
void i2c_shutdown(void);

uint8_t i2c_write_multi(uint8_t addr, uint8_t reg, const uint8_t *buf, uint8_t len);
uint8_t i2c_read_multi(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);

static inline bool i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    return i2c_write_multi(addr, reg, &val, 1) == 1;
}

static uint8_t i2c_read_reg(uint8_t addr, uint8_t reg)
{
    uint8_t v = 0;
    i2c_read_multi(addr, reg, &v, 1);
    return v;
}
