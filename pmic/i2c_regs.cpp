#include <stdint.h>
#include <avr/io.h>
#include "dev.h"
#include "key.h"
#include "adc.h"
#include "eeprom.h"

typedef enum {
    I2cRegId = 0,
    I2cRegState,
    I2cRegBootMode,
    I2cRegAdcTemp0,
    I2cRegAdcTemp1,
    I2cRegAdcVbg0,
    I2cRegAdcVbg1,
    I2cRegEspTimeoutSec0,
    I2cRegEspTimeoutSec1,
    I2cRegPicoTimeoutSec0,
    I2cRegPicoTimeoutSec1,
    I2cRegEspScheduleSec0,
    I2cRegEspScheduleSec1,
    I2cRegEspScheduleSec2,
    I2cRegEspScheduleSec3,
    I2cRegPicoScheduleSec0,
    I2cRegPicoScheduleSec1,
    I2cRegPicoScheduleSec2,
    I2cRegPicoScheduleSec3,
} i2c_reg_t;

uint32_t buf;

uint8_t i2c_slave_regs_read(uint8_t reg)
{
    switch (reg) {
    case I2cRegId:
        return 0x5d;
    case I2cRegState:
        return dev_pwr_enabled(DevPico) | (dev_pwr_enabled(DevEsp) << 1) |
            (key_state() << 2) | ((!adc_busy()) << 4);
    case I2cRegBootMode:
        return eeprom_read_byte(&eeprom_data->boot_mode);

    case I2cRegAdcTemp0:
        buf = adc_result(AdcChTemp);
        // fall-through
    case I2cRegAdcTemp1:
        return buf >> (8 * (reg - I2cRegAdcTemp0));

    case I2cRegAdcVbg0:
        buf = adc_result(AdcChVcc);
        // fall-through
    case I2cRegAdcVbg1:
        return buf >> (8 * (reg - I2cRegAdcVbg0));

    case I2cRegEspTimeoutSec0:
        buf = eeprom_read_word(&eeprom_data->esp_timeout_sec);
        // fall-through
    case I2cRegEspTimeoutSec1:
        return buf >> (8 * (reg - I2cRegEspTimeoutSec0));

    case I2cRegPicoTimeoutSec0:
        buf = eeprom_read_word(&eeprom_data->pico_timeout_sec);
        // fall-through
    case I2cRegPicoTimeoutSec1:
        return buf >> (8 * (reg - I2cRegPicoTimeoutSec0));
    }
    return 0xdd;
}

void i2c_slave_regs_write(uint8_t reg, uint8_t val)
{
    switch (reg) {
    case I2cRegState:
        dev_pwr_en(DevPico, val & 1);
        dev_pwr_en(DevEsp, val & 2);
        break;
    case I2cRegBootMode:
        eeprom_update_byte(&eeprom_data->boot_mode, val);
        break;

    case I2cRegEspScheduleSec0:
    case I2cRegEspScheduleSec1:
    case I2cRegEspScheduleSec2:
    case I2cRegEspScheduleSec3:
    case I2cRegPicoScheduleSec0:
    case I2cRegPicoScheduleSec1:
    case I2cRegPicoScheduleSec2:
    case I2cRegPicoScheduleSec3:
        buf = (buf >> 8) | ((uint32_t)val << 24);
        if (reg == I2cRegPicoScheduleSec3)
            dev_schedule_sec(DevPico, buf);
        else if (reg == I2cRegEspScheduleSec3)
            dev_schedule_sec(DevEsp, buf);
        break;
    }
}
