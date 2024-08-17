#include <stdint.h>
#include <avr/io.h>
#include "dev.h"
#include "key.h"
#include "adc.h"

typedef enum {
    I2cRegId = 0,
    I2cRegState,
    I2cRegAdcTempL,
    I2cRegAdcTempH,
    I2cRegAdcVbgL,
    I2cRegAdcVbgH,
} i2c_reg_t;

uint8_t i2c_slave_regs_read(uint8_t reg)
{
    switch (reg) {
    case I2cRegId:
        return 0x5d;
    case I2cRegState:
        return dev_pwr_state() | (key_state() << 2) | ((!adc_busy()) << 4);
    case I2cRegAdcTempL:
        return adc_result(AdcChTemp) & 0xff;
    case I2cRegAdcTempH:
        return (adc_result(AdcChTemp) >> 8) & 0xff;
    case I2cRegAdcVbgL:
        return adc_result(AdcChVcc) & 0xff;
    case I2cRegAdcVbgH:
        return (adc_result(AdcChVcc) >> 8) & 0xff;
    }
    return 0xdd;
}

void i2c_slave_regs_write(uint8_t reg, uint8_t val)
{
    switch (reg) {
    case I2cRegState:
        dev_pwr_en(val);
        if (val & _BV(4))
            adc_start();
        break;
    }
}
