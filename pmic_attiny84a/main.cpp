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
#include "eeprom.h"

void init()
{
    // Disable all modules by default
    power_all_disable();

    PORTA = LED_INIT_PORTA_MASK | DEV_INIT_PORTA_MASK | KEY_INIT_PORTA_MASK | I2C_INIT_PORTA_MASK;
    PORTB = LED_INIT_PORTB_MASK | DEV_INIT_PORTB_MASK | KEY_INIT_PORTB_MASK | I2C_INIT_PORTB_MASK;

    DDRA = LED_INIT_DDRA_MASK | DEV_INIT_DDRA_MASK | KEY_INIT_DDRA_MASK | I2C_INIT_DDRA_MASK;
    DDRB = LED_INIT_DDRB_MASK | DEV_INIT_DDRB_MASK | KEY_INIT_DDRB_MASK | I2C_INIT_DDRB_MASK;

    adc_start();
    key_init();
    wdt_init();

    sei();
}

int main()
{
    init();

    uint8_t boot_mode = eeprom_read_byte(&eeprom_data->boot_mode);
    dev_pwr_en(DevAux, boot_mode & 1);
    dev_pwr_en(DevEsp, boot_mode & 2);

    for (;;) {
        // Select appropriate sleep mode and go to sleep
        cli();
        uint8_t sleep = SLEEP_MODE_PWR_DOWN;
        if (dev_pwr_enabled(DevAux) | dev_pwr_enabled(DevEsp) | timer1_enabled())
            sleep = SLEEP_MODE_IDLE;    // Some controllers still powered on
        set_sleep_mode(sleep);
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
    }
}

void key_irq(uint8_t key)
{
    static uint8_t prev_key = 0;
    if (prev_key != key) {
        uint8_t pressed = key & ~prev_key;
        prev_key = key;
        if (pressed & 1)
            dev_pwr_en(DevEsp, !dev_pwr_enabled(DevEsp));
    }
}
