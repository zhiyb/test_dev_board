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
#include "sht.h"

#define STACK_MAGIC 0x5a
uint8_t stack_magic __attribute__ ((section (".noinit")));

void init()
{
    // Disable all modules by default
    power_all_disable();

    PORTA = LED_INIT_PORTA_MASK | DEV_INIT_PORTA_MASK | KEY_INIT_PORTA_MASK | I2C_INIT_PORTA_MASK;
    PORTB = LED_INIT_PORTB_MASK | DEV_INIT_PORTB_MASK | KEY_INIT_PORTB_MASK | I2C_INIT_PORTB_MASK;

    DDRA = LED_INIT_DDRA_MASK | DEV_INIT_DDRA_MASK | KEY_INIT_DDRA_MASK | I2C_INIT_DDRA_MASK;
    DDRB = LED_INIT_DDRB_MASK | DEV_INIT_DDRB_MASK | KEY_INIT_DDRB_MASK | I2C_INIT_DDRB_MASK;

    stack_magic = STACK_MAGIC;

    adc_start();
    key_init();
    wdt_init();

    sei();
}

int main()
{
    init();
    sht_trigger_update();

    uint8_t boot_mode = eeprom_read_byte(&eeprom_data->boot_mode);
    if (boot_mode & 1)
        dev_pwr_req(DevAux, true);
    if (boot_mode & 2)
        dev_pwr_req(DevEsp, true);

    for (;;) {
        if (stack_magic != STACK_MAGIC) {
            // Stack overflow!
            cli();
            led_set(LedBlue, false);
            for (;;) {
                led_set(LedRed, true);
                led_set(LedGreen, true);
                _delay_ms(50);
                led_set(LedRed, false);
                led_set(LedGreen, false);
                _delay_ms(100);
            }
        }

        cli();
        if (dev_pwr_req_pending()) {
            // Process pending power requests with interrupts enabled
            sei();
            dev_pwr_req_proc();
        } else {
            // Select appropriate sleep mode and go to sleep
            uint8_t sleep = SLEEP_MODE_PWR_DOWN;
            if (i2c_enabled() | timer1_enabled())
                sleep = SLEEP_MODE_IDLE;    // Clock required by peripherals
            set_sleep_mode(sleep);
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
        }
    }
}

void key_irq(uint8_t key)
{
    static uint8_t prev_key = 0;
    if (prev_key != key) {
        uint8_t pressed = key & ~prev_key;
        prev_key = key;
        if (pressed & 1)
            dev_pwr_req(DevEsp, !(dev_pwr_state() & _BV(DevEsp)));
    }
}
