#include <avr/io.h>
#include "dev.h"
#include "led.h"
#include "adc.h"
#include "wdt.h"
#include "i2c.h"
#include "eeprom.h"

// Automatically restart a device after some time if it was timed out
static const uint32_t timeout_reschedule_sec = 60 * 60;

static volatile struct {
    uint32_t schedule_ticks;
    uint32_t start_tick;
    uint32_t enable_tick;
    bool scheduled;
} devs[NumDevs];

static uint8_t enabled = 0;

bool dev_pwr_enabled(dev_t dev)
{
    return !!(enabled & _BV(dev));
}

void dev_pwr_en(dev_t dev, bool en)
{
    uint8_t prev_enabled = enabled;
    if (en)
        enabled |= _BV(dev);
    else
        enabled &= ~_BV(dev);

    // Enable 3v3 and I2C peripheral first if any devices are being enabled
    if (!prev_enabled && en) {
        // Trigger ADC conversions too
        adc_start();
        PORTA |= _BV(3);
        i2c_slave_init();
    }

    switch (dev) {
    case DevAux:
        if (en)
            PORTA &= ~_BV(1);
        else
            PORTA |= _BV(1);
        led_set(LedBlue, en);
        break;
    case DevEsp:
        if (en)
            PORTA &= ~_BV(2);
        else
            PORTA |= _BV(2);
        led_set(LedRed, en);
        break;
    }

    // Check if all devices have been disabled
    if (prev_enabled && !enabled) {
        i2c_slave_deinit();
        PORTA &= ~_BV(3);
    }

    // Record device enabled time
    devs[dev].enable_tick = wdt_tick();
}

void dev_wdt_irq(uint32_t tick)
{
    for (dev_t dev = (dev_t)0; dev < NumDevs; dev = (dev_t)(dev + 1)) {
        if (!devs[dev].scheduled)
            continue;

        if (dev_pwr_enabled(dev)) {
            // Device enabled, check for turn-off timeout
            uint16_t *p = 0;
            if (dev == DevAux)
                p = &eeprom_data->pico_timeout_sec;
            else if (dev == DevEsp)
                p = &eeprom_data->esp_timeout_sec;
            uint16_t timeout_sec = eeprom_read_word(p);
            uint32_t timeout_ticks = wdt_sec_to_ticks(timeout_sec);
            uint32_t delta = tick - devs[dev].enable_tick;
            if (delta >= timeout_ticks) {
                // Timed out, reschedule after some time
                dev_pwr_en(dev, false);
                devs[dev].schedule_ticks = wdt_sec_to_ticks(timeout_reschedule_sec);
                devs[dev].scheduled = true;
            }

        } else {
            // Device not enabled, check for scheduled turn-on time
            uint32_t delta = tick - devs[dev].start_tick;
            if (delta >= devs[dev].schedule_ticks) {
                devs[dev].scheduled = false;
                dev_pwr_en(dev, true);
            }
        }
    }
}

void dev_schedule_sec(dev_t dev, uint32_t sec)
{
    devs[dev].scheduled = false;
    devs[dev].start_tick = wdt_tick();
    devs[dev].schedule_ticks = wdt_sec_to_ticks(sec);
    devs[dev].scheduled = true;
}
