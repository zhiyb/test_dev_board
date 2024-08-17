#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/delay.h>

#include "led.h"
#include "key.h"
#include "dev.h"
#include "i2c.h"
#include "adc.h"
#include "timer.h"
#include "wdt.h"

void init()
{
    // Disable all modules by default
    power_all_disable();
    // Disable JTAG to release GPIOs
    MCUCR |= 0x80;
    MCUCR |= 0x80;

    PORTB = LED_INIT_PORTB_MASK | DEV_INIT_PORTB_MASK | KEY_INIT_PORTB_MASK | \
            I2C_INIT_PORTB_MASK;
    PORTC = LED_INIT_PORTC_MASK | DEV_INIT_PORTC_MASK | KEY_INIT_PORTC_MASK | \
            I2C_INIT_PORTC_MASK;
    PORTD = LED_INIT_PORTD_MASK | DEV_INIT_PORTD_MASK | KEY_INIT_PORTD_MASK | \
            I2C_INIT_PORTD_MASK;

    DDRB = LED_INIT_DDRB_MASK | DEV_INIT_DDRB_MASK | KEY_INIT_DDRB_MASK | \
           I2C_INIT_DDRB_MASK;
    DDRC = LED_INIT_DDRC_MASK | DEV_INIT_DDRC_MASK | KEY_INIT_DDRC_MASK | \
           I2C_INIT_DDRC_MASK;
    DDRD = LED_INIT_DDRD_MASK | DEV_INIT_DDRD_MASK | KEY_INIT_DDRD_MASK | \
           I2C_INIT_DDRD_MASK;

    adc_start();
    led_init();
    i2c_slave_init();
    key_init();
    timer0_init();
    wdt_init();
    sei();
}

int main()
{
    init();

    for (;;) {
        static uint8_t prev_key = 0;
        uint8_t key = key_state();
        if (prev_key != key) {
            uint32_t pressed = key & ~prev_key;
            prev_key = key;
            if (pressed & 1)
                dev_pwr_en(dev_pwr_state() ^ 1);
            if (pressed & 2)
                dev_pwr_en(dev_pwr_state() ^ 2);
        }

        // Select appropriate sleep mode and go to sleep
        cli();
        uint8_t sleep = SLEEP_MODE_PWR_DOWN;
        if (dev_pwr_state() | timer0_enabled())
            sleep = SLEEP_MODE_IDLE;    // Some controllers still powered on
        set_sleep_mode(sleep);
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
    }
}
