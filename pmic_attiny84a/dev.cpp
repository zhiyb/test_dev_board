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

static volatile struct {
    uint32_t pwr_tick;
    uint32_t next_tick;
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

    // Record when device power changed state
    devs[dev].pwr_tick = wdt_tick();

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

uint16_t dev_get_timeout_ticks(dev_t dev)
{
    uint16_t *p = 0;
    if (dev == DevAux)
        p = &eeprom_data->aux_timeout_ticks;
    else if (dev == DevEsp)
        p = &eeprom_data->esp_timeout_ticks;
    else if (dev == DevSHT)
        p = &eeprom_data->sht_timeout_ticks;
    return p ? eeprom_read_word(p) : 0;
}

void dev_set_timeout_ticks(dev_t dev, uint16_t ticks)
{
    uint16_t *p = 0;
    if (dev == DevAux)
        p = &eeprom_data->aux_timeout_ticks;
    else if (dev == DevEsp)
        p = &eeprom_data->esp_timeout_ticks;
    else if (dev == DevSHT)
        p = &eeprom_data->sht_timeout_ticks;
    if (p)
        eeprom_write_word(p, ticks);
}

uint16_t dev_get_periodic_ticks(dev_t dev)
{
    uint16_t *p = 0;
    if (dev == DevAux)
        p = &eeprom_data->aux_periodic_ticks;
    else if (dev == DevEsp)
        p = &eeprom_data->esp_periodic_ticks;
    else if (dev == DevSHT)
        p = &eeprom_data->sht_periodic_ticks;
    return p ? eeprom_read_word(p) : 0;
}

void dev_set_periodic_ticks(dev_t dev, uint16_t ticks)
{
    uint16_t *p = 0;
    if (dev == DevAux)
        p = &eeprom_data->aux_periodic_ticks;
    else if (dev == DevEsp)
        p = &eeprom_data->esp_periodic_ticks;
    else if (dev == DevSHT)
        p = &eeprom_data->sht_periodic_ticks;
    if (p)
        eeprom_write_word(p, ticks);
}

uint32_t dev_get_next_tick(dev_t dev)
{
    return devs[dev].next_tick;
}

void dev_set_next_tick(dev_t dev, uint32_t tick)
{
    devs[dev].next_tick = tick;
    devs[dev].scheduled = true;
}

bool dev_get_scheduled(dev_t dev)
{
    return devs[dev].scheduled;
}

void dev_set_scheduled(dev_t dev, bool schd)
{
    devs[dev].scheduled = schd;
}

void dev_init(void)
{
    uint32_t tick = wdt_tick();

    // Check scheduling info in EEPROM config
    uint8_t boot_mode = eeprom_read_byte(&eeprom_data->boot_mode);
    for (uint8_t dev = 0; dev < NumDevs; dev++) {
        if (boot_mode & _BV(dev))
            dev_pwr_req((dev_t)dev, true);

        // Check if device is scheduled for turning on
        uint16_t ticks = dev_get_periodic_ticks((dev_t)dev);
        devs[dev].next_tick = tick + ticks;
        devs[dev].scheduled = ticks != 0;
    }
}

void dev_wdt_irq(uint32_t tick)
{
    for (uint8_t dev = 0; dev < NumDevs; dev++) {
        if (enabled & _BV(dev)) {
            // Device enabled, check for turn-off timeout
            uint16_t ticks = dev_get_timeout_ticks((dev_t)dev);
            if (ticks != 0) {
                uint32_t delta = tick - devs[dev].pwr_tick;
                if (delta >= ticks) {
                    // Timed out, turn off
                    dev_pwr_req((dev_t)dev, false);
                }
            }

        } else if (devs[dev].scheduled) {
            // Device not enabled, but scheduled for turning on
            int32_t delta = tick - devs[dev].next_tick;
            if (delta >= 0) {
                // Turn-on tick reached
                dev_pwr_req((dev_t)dev, true);
                // Check for next periodic turn-on schedule
                uint16_t ticks = dev_get_periodic_ticks((dev_t)dev);
                devs[dev].next_tick = tick + ticks;
                devs[dev].scheduled = ticks != 0;
            }
        }
    }
}
