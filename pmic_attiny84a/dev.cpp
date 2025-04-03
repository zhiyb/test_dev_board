#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "dev.h"
#include "led.h"
#include "adc.h"
#include "wdt.h"
#include "i2c.h"
#include "eeprom.h"
#include "sht.h"

// Automatically restart a device after some time if it was timed out
static const uint32_t timeout_reschedule_sec = 60 * 60;

static volatile struct {
    uint32_t schedule_ticks;
    uint32_t start_tick;
    uint32_t enable_tick;
    bool scheduled;
} devs[NumDevs];

static volatile uint8_t power_on_req = 0;
static volatile uint8_t power_off_req = 0;
static uint8_t enabled = 0;

static inline void dev_pwr_enable(dev_t dev)
{
    uint8_t prev_enabled = enabled;
    enabled |= _BV(dev);

    // Enable 3v3 and I2C peripheral first if any devices are being enabled
    if (!prev_enabled) {
        // Trigger ADC conversions too
        adc_start();
        // Enable 3v3
        PORTA |= _BV(3);
        // Initialise I2C interface
        if (dev == DevSHT)
            i2c_master_init();
        else
            i2c_slave_init();
    }

    // Record device enabled time
    devs[dev].enable_tick = wdt_tick();

    switch (dev) {
    case DevAux:
        PORTA &= ~_BV(1);
        led_set(LedBlue, true);
        break;
    case DevEsp:
        PORTA &= ~_BV(2);
        led_set(LedRed, true);
        break;
    case DevSHT:
        sht_powered_on();
        break;
    }
}

static inline void dev_pwr_disable(dev_t dev)
{
    switch (dev) {
    case DevAux:
        PORTA |= _BV(1);
        led_set(LedBlue, false);
        break;
    case DevEsp:
        PORTA |= _BV(2);
        led_set(LedRed, false);
        break;
    }

    enabled &= ~_BV(dev);
    // Check if all devices have been disabled
    if (!enabled) {
        i2c_deinit();
        PORTA &= ~_BV(3);
    }
}

void dev_pwr_req(dev_t dev, bool en)
{
    uint8_t mask = _BV(dev);
    // Use ATOMIC_RESTORESTATE as this function may be called from ISR
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (en) {
            power_on_req |= mask;
            power_off_req &= ~mask;
        } else {
            power_on_req &= ~mask;
            power_off_req |= mask;
        }
    }
    led_act_trigger();
}

bool dev_pwr_req_pending(void)
{
    return power_on_req | power_off_req;
}

void dev_pwr_req_proc(void)
{
    uint8_t on_req, off_req;
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        on_req = power_on_req;
        power_on_req = 0;
        off_req = power_off_req;
        power_off_req = 0;
    }
    on_req &= ~enabled;
    off_req &= enabled;

    // Process power off requests first
    for (uint8_t dev = 0; dev < NumDevs; dev++)
        if (_BV(dev) & off_req)
            dev_pwr_disable((dev_t)dev);

    // Process power on requests
    // SHT sensor and other are exclusive
    if (enabled & _BV(DevSHT)) {
        // SHT sensor already on, cannot power on any other devices
    } else if (enabled & ~_BV(DevSHT)) {
        // Cannot power on SHT
        for (uint8_t dev = 0; dev < DevSHT; dev++)
            if (on_req & _BV(dev))
                dev_pwr_enable((dev_t)dev);
        on_req &= _BV(DevSHT);
    } else if (on_req & _BV(DevSHT)) {
        // SHT sensor takes priority
        dev_pwr_enable(DevSHT);
        on_req &= ~_BV(DevSHT);
    } else {
        for (uint8_t dev = 0; dev < DevSHT; dev++)
            if (on_req & _BV(dev))
                dev_pwr_enable((dev_t)dev);
        on_req = 0;
    }

    // Re-queue pending power on requests
    if (on_req) {
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            on_req &= ~power_off_req;
            power_on_req |= on_req;
        }
    }
}

uint8_t dev_pwr_state(void)
{
    return enabled;
}

void dev_wdt_irq(uint32_t tick)
{
    for (dev_t dev = (dev_t)0; dev < NumDevs; dev = (dev_t)(dev + 1)) {
        if (!devs[dev].scheduled)
            continue;

        if (enabled & _BV(dev)) {
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
                dev_pwr_req(dev, false);
                devs[dev].schedule_ticks = wdt_sec_to_ticks(timeout_reschedule_sec);
                devs[dev].scheduled = true;
            }

        } else {
            // Device not enabled, check for scheduled turn-on time
            uint32_t delta = tick - devs[dev].start_tick;
            if (delta >= devs[dev].schedule_ticks) {
                devs[dev].scheduled = false;
                dev_pwr_req(dev, true);
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
