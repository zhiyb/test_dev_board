#include <stdint.h>
#include <avr/io.h>
#include "dev.h"
#include "key.h"
#include "adc.h"
#include "eeprom.h"
#include "sht.h"
#include "wdt.h"

typedef enum : uint8_t {
    I2cRegId = 0,
    I2cRegState,
    I2cRegBootMode,
    I2cRegStoreConfig,

    I2cRegSchdDev,                  // Select a dev for configuration
    I2cRegSchdScheduled,
    I2cRegSchdPeriodic0,            // u16
    I2cRegSchdPeriodic1,
    I2cRegSchdTimeout0,             // u16
    I2cRegSchdTimeout1,
    I2cRegSchdNextTick0,            // u32
    I2cRegSchdNextTick1,
    I2cRegSchdNextTick2,
    I2cRegSchdNextTick3,

    I2cRegWdtScratch,
    I2cRegWdtTick0,                 // u32
    I2cRegWdtTick1,
    I2cRegWdtTick2,
    I2cRegWdtTick3,

    I2cRegAdcTemp0,                 // u16
    I2cRegAdcTemp1,
    I2cRegAdcVcc0,                  // u16
    I2cRegAdcVcc1,

    I2cRegShtLastMeasurement0,      // u16 T
    I2cRegShtLastMeasurement1,
    I2cRegShtLastMeasurement2,      // u16 RH
    I2cRegShtLastMeasurement3,
    I2cRegShtLastMeasurement4,      // u16 WDT tick
    I2cRegShtLastMeasurement5,
    I2cRegShtReadMeasurementLog0,   // u16 T
    I2cRegShtReadMeasurementLog1,
    I2cRegShtReadMeasurementLog2,   // u16 RH
    I2cRegShtReadMeasurementLog3,
    I2cRegShtReadMeasurementLog4,   // u16 WDT tick
    I2cRegShtReadMeasurementLog5,
    I2cRegShtMeasurementLogLength,
} pmic_i2c_reg_t;

static uint32_t buf_u32;
static uint16_t buf_u16;
static uint8_t wdt_scratch = 0;
static dev_t schd_dev = (dev_t)0;

uint8_t i2c_slave_regs_read(uint8_t reg)
{
    const sht_data_t *psht;
    uint8_t v;
    switch ((pmic_i2c_reg_t)reg) {
    case I2cRegId:
        return 0x5d;
    case I2cRegState:
        return (dev_pwr_state() & 3) | (key_state() << 2) | ((!adc_busy()) << 4);
    case I2cRegBootMode:
        return eeprom_cache.boot_mode;

    case I2cRegSchdDev:
        return schd_dev;
    case I2cRegSchdScheduled:
        return dev_get_scheduled(schd_dev);

    case I2cRegWdtScratch:
        return wdt_scratch;

    case I2cRegShtMeasurementLogLength:
        return sht_measurement_log_length();

    case I2cRegSchdPeriodic0:
    case I2cRegSchdTimeout0:
    case I2cRegAdcTemp0:
    case I2cRegAdcVcc0:
        switch ((pmic_i2c_reg_t)reg) {
            case I2cRegSchdPeriodic0:
                buf_u16 = dev_get_periodic_ticks(schd_dev);
                break;
            case I2cRegSchdTimeout0:
                buf_u16 = dev_get_timeout_ticks(schd_dev);
                break;

            case I2cRegAdcTemp0:
                buf_u16 = adc_result(AdcChTemp);
                break;
            case I2cRegAdcVcc0:
                buf_u16 = adc_result(AdcChVcc);
                break;
        }
        // fall-through
    case I2cRegSchdPeriodic1:
    case I2cRegSchdTimeout1:
    case I2cRegAdcTemp1:
    case I2cRegAdcVcc1:
    case I2cRegShtLastMeasurement4:
    case I2cRegShtLastMeasurement5:
    case I2cRegShtReadMeasurementLog4:
    case I2cRegShtReadMeasurementLog5:
        v = buf_u16;
        buf_u16 >>= 8;
        return v;

    case I2cRegSchdNextTick0:
    case I2cRegWdtTick0:
    case I2cRegShtLastMeasurement0:
    case I2cRegShtReadMeasurementLog0:
        switch ((pmic_i2c_reg_t)reg) {
            case I2cRegSchdNextTick0:
                buf_u32 = dev_get_next_tick(schd_dev);
                break;

            case I2cRegWdtTick0:
                buf_u32 = wdt_tick();
                break;

            case I2cRegShtLastMeasurement0:
                psht = sht_last_measurement();
                buf_u32 = psht->t | ((uint32_t)psht->rh << 16);
                buf_u16 = psht->tick;
                break;
            case I2cRegShtReadMeasurementLog0:
                psht = sht_read_measurement_log();
                buf_u32 = psht->t | ((uint32_t)psht->rh << 16);
                buf_u16 = psht->tick;
                break;
        }
        // fall-through
    case I2cRegSchdNextTick1:
    case I2cRegSchdNextTick2:
    case I2cRegSchdNextTick3:
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
    switch ((pmic_i2c_reg_t)reg) {
    case I2cRegState:
        dev_pwr_req(DevAux, val & 1);
        dev_pwr_req(DevEsp, val & 2);
        break;
    case I2cRegBootMode:
        eeprom_cache.boot_mode = val;
        break;
    case I2cRegStoreConfig:
        if (val != 0)
            eeprom_update();
        break;

    case I2cRegSchdDev:
        schd_dev = (dev_t)(val < NumDevs ? val : 0);
        break;
    case I2cRegSchdScheduled:
        dev_set_scheduled(schd_dev, !!val);
        break;

    case I2cRegWdtScratch:
        wdt_scratch = val;
        break;

    case I2cRegSchdPeriodic0:
    case I2cRegSchdPeriodic1:
    case I2cRegSchdTimeout0:
    case I2cRegSchdTimeout1:
        buf_u16 = (buf_u16 >> 8) | ((uint32_t)val << 8);
        switch ((pmic_i2c_reg_t)reg) {
        case I2cRegSchdPeriodic1:
            dev_set_periodic_ticks(schd_dev, buf_u16);
            break;
        case I2cRegSchdTimeout1:
            dev_set_timeout_ticks(schd_dev, buf_u16);
            break;
        }
        break;

    case I2cRegSchdNextTick0:
    case I2cRegSchdNextTick1:
    case I2cRegSchdNextTick2:
    case I2cRegSchdNextTick3:
        buf_u32 = (buf_u32 >> 8) | ((uint32_t)val << 24);
        switch (reg) {
        case I2cRegSchdNextTick3:
            dev_set_next_tick(schd_dev, buf_u32);
            break;
        }
        break;
    }
}
