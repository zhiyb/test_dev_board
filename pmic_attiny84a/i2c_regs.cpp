#include <stdint.h>
#include <avr/io.h>
#include "dev.h"
#include "key.h"
#include "adc.h"
#include "eeprom.h"
#include "sht.h"
#include "wdt.h"

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
    I2cRegEspScheduleSec0,          // u32
    I2cRegEspScheduleSec1,
    I2cRegEspScheduleSec2,
    I2cRegEspScheduleSec3,
    I2cRegPicoScheduleSec0,         // u32
    I2cRegPicoScheduleSec1,
    I2cRegPicoScheduleSec2,
    I2cRegPicoScheduleSec3,

    I2cRegWdtScratch,
    I2cRegWdtTick0,                 // u32
    I2cRegWdtTick1,
    I2cRegWdtTick2,
    I2cRegWdtTick3,

    I2cRegShtLastMeasurement0,      // u16 T
    I2cRegShtLastMeasurement1,
    I2cRegShtLastMeasurement2,      // u16 RH
    I2cRegShtLastMeasurement3,
    I2cRegShtReadMeasurementLog0,   // u16 T
    I2cRegShtReadMeasurementLog1,
    I2cRegShtReadMeasurementLog2,   // u16 RH
    I2cRegShtReadMeasurementLog3,
    I2cRegShtReadMeasurementLog4,   // u16 WDT tick
    I2cRegShtReadMeasurementLog5,
    I2cRegShtMeasurementLogLength,
} i2c_reg_t;

static uint32_t buf_u32;
static uint16_t buf_u16;
static uint8_t wdt_scratch = 0;

uint8_t i2c_slave_regs_read(uint8_t reg)
{
    const sht_data_t *psht;
    uint8_t v;
    switch (reg) {
    case I2cRegId:
        return 0x5d;
    case I2cRegState:
        return (dev_pwr_state() & 3) | (key_state() << 2) | ((!adc_busy()) << 4);
    case I2cRegBootMode:
        return eeprom_read_byte(&eeprom_data->boot_mode);

    case I2cRegWdtScratch:
        return wdt_scratch;

    case I2cRegShtMeasurementLogLength:
        return sht_measurement_log_length();

    case I2cRegAdcTemp0:
    case I2cRegAdcVbg0:
    case I2cRegEspTimeoutSec0:
    case I2cRegPicoTimeoutSec0:
        switch (reg) {
            case I2cRegAdcTemp0:
                buf_u16 = adc_result(AdcChTemp);
                break;
            case I2cRegAdcVbg0:
                buf_u16 = adc_result(AdcChVcc);
                break;

            case I2cRegEspTimeoutSec0:
                buf_u16 = eeprom_read_word(&eeprom_data->esp_timeout_sec);
                break;
            case I2cRegPicoTimeoutSec0:
                buf_u16 = eeprom_read_word(&eeprom_data->pico_timeout_sec);
                break;
        }
        // fall-through
    case I2cRegAdcTemp1:
    case I2cRegAdcVbg1:
    case I2cRegEspTimeoutSec1:
    case I2cRegPicoTimeoutSec1:
    case I2cRegShtReadMeasurementLog4:
    case I2cRegShtReadMeasurementLog5:
        v = buf_u16;
        buf_u16 >>= 8;
        return v;

    case I2cRegWdtTick0:
    case I2cRegShtLastMeasurement0:
    case I2cRegShtReadMeasurementLog0:
        switch (reg) {
            case I2cRegWdtTick0:
                buf_u32 = wdt_tick();
                break;

            case I2cRegShtLastMeasurement0:
                psht = sht_last_measurement();
                buf_u32 = psht->t | ((uint32_t)psht->rh << 16);
                break;
            case I2cRegShtReadMeasurementLog0:
                psht = sht_read_measurement_log();
                buf_u32 = psht->t | ((uint32_t)psht->rh << 16);
                buf_u16 = psht->tick;
                break;
        }
        // fall-through
    case I2cRegWdtTick1:
    case I2cRegWdtTick2:
    case I2cRegWdtTick3:
    case I2cRegShtLastMeasurement1:
    case I2cRegShtLastMeasurement2:
    case I2cRegShtLastMeasurement3:
    case I2cRegShtReadMeasurementLog1:
    case I2cRegShtReadMeasurementLog2:
    case I2cRegShtReadMeasurementLog3:
        v = buf_u32;
        buf_u32 >>= 8;
        return v;
    }
    return 0xff;
}

void i2c_slave_regs_write(uint8_t reg, uint8_t val)
{
    switch (reg) {
    case I2cRegState:
        dev_pwr_req(DevAux, val & 1);
        dev_pwr_req(DevEsp, val & 2);
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
        buf_u32 = (buf_u32 >> 8) | ((uint32_t)val << 24);
        switch (reg) {
        case I2cRegPicoScheduleSec3:
            dev_schedule_sec(DevAux, buf_u32);
            break;
        case I2cRegEspScheduleSec3:
            dev_schedule_sec(DevEsp, buf_u32);
            break;
        }
        break;

    case I2cRegWdtScratch:
        wdt_scratch = val;
        break;
    }
}
