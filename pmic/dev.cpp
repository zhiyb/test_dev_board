#include <avr/io.h>
#include "dev.h"
#include "led.h"
#include "adc.h"
#include "wdt.h"

static uint32_t esp_start_tick = 0;

uint8_t dev_pwr_state(void)
{
    uint8_t v = 0;
    if (PORTD & _BV(7))     // PICO_EN
        v |= DEV_PICO_MASK;
    if (!(PORTD & _BV(4)))  // ESP_EN
        v |= DEV_ESP_MASK;
    return v;
}

void dev_pwr_en(uint8_t v)
{
    uint8_t portm = _BV(7) | _BV(4);
    uint8_t portv = 0;
    if (v & DEV_PICO_MASK)
        portv |= _BV(7);
    if (!(v & DEV_ESP_MASK))
        portv |= _BV(4);
    PORTD = (PORTD & ~portm) | portv;

    // Update LEDs
    led_set(LedRed, v & DEV_ESP_MASK);
    led_set(LedBlue, v & DEV_PICO_MASK);
    // Trigger ADC if enabling any controller
    if (v & (DEV_PICO_MASK | DEV_ESP_MASK))
        adc_start();
    // Update periodic timer
    if (v & DEV_ESP_MASK)
        esp_start_tick = wdt_tick();
}

void dev_wdt_irq(uint32_t tick)
{
    // Enable ESP every hour
    const uint32_t esp_period = wdt_sec_to_ticks(60 * 60);

    if (tick - esp_start_tick >= esp_period) {
        esp_start_tick = tick;

        uint8_t state = dev_pwr_state();
        if (!(state & DEV_ESP_MASK))
            dev_pwr_en(state | DEV_ESP_MASK);
    }
}
