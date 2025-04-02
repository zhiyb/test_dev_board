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

#if 1
    // SHT40 sensor
    uint8_t addr = 0x44;
    dev_pwr_en(DevSHT, true);
    i2c_master_init();

    // Power-up time 1ms
    timer1_restart_ms(1);
    timer1_wait_sleep();
    i2c_master_init_resync();

#if 0
    // Read serial number
    uint8_t cmd = 0x89;
    if (i2c_master_write(addr, &cmd, 1)) {
        uint8_t buf[6];
        for (uint8_t i = 0; i < 100; i++)
            if (i2c_master_read(addr, &buf[0], 6))
                break;
    }
#endif

    // Measure T & RH with high precision
    uint8_t cmd = 0xfd;
    if (i2c_master_write(addr, &cmd, 1)) {
        timer1_restart_ms(8);
        timer1_wait_sleep();

        uint8_t buf[6];
        for (uint8_t i = 0; i < 200; i++) {
            if (i2c_master_read(addr, &buf[0], 6))
                break;
            timer1_restart_ms(1);
            timer1_wait_sleep();
        }
    }

    i2c_deinit();
#endif


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
