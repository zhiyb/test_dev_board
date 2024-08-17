#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "led.h"
#include "key.h"
#include "dev.h"
#include "uart.h"
#include "i2c.h"

void init()
{
    MCUCR |= 0x80;			// Disable JTAG
    MCUCR |= 0x80;

    PORTB = LED_INIT_PORTB_MASK | DEV_INIT_PORTB_MASK | KEY_INIT_PORTB_MASK | \
            UART_INIT_PORTB_MASK | I2C_INIT_PORTB_MASK;
    PORTC = LED_INIT_PORTC_MASK | DEV_INIT_PORTC_MASK | KEY_INIT_PORTC_MASK | \
            UART_INIT_PORTC_MASK | I2C_INIT_PORTC_MASK;
    PORTD = LED_INIT_PORTD_MASK | DEV_INIT_PORTD_MASK | KEY_INIT_PORTD_MASK | \
            UART_INIT_PORTD_MASK | I2C_INIT_PORTD_MASK;

    DDRB = LED_INIT_DDRB_MASK | DEV_INIT_DDRB_MASK | KEY_INIT_DDRB_MASK | \
           UART_INIT_DDRB_MASK | I2C_INIT_DDRB_MASK;
    DDRC = LED_INIT_DDRC_MASK | DEV_INIT_DDRC_MASK | KEY_INIT_DDRC_MASK | \
           UART_INIT_DDRC_MASK | I2C_INIT_DDRC_MASK;
    DDRD = LED_INIT_DDRD_MASK | DEV_INIT_DDRD_MASK | KEY_INIT_DDRD_MASK | \
           UART_INIT_DDRD_MASK | I2C_INIT_DDRD_MASK;

    uart0_init();
    i2c_slave_init();
    sei();
}

bool act(bool reset)
{
    static uint16_t led_act = 0;
    if (reset) {
        led_act = 0x0fff;
        return led_act;
    }
    uint16_t ret = led_act;
    if (ret)
        led_act = ret - 1;
    return ret;
}

int main()
{
    init();

    for (;;) {
        uint8_t led = 0;

        static uint8_t prev_key = 0;
        uint8_t key = key_state();
        if (prev_key != key) {
            uint32_t pressed = key & ~prev_key;
            prev_key = key;
            act(true);

            if (pressed & 1)
                dev_pwr_en(dev_pwr_state() ^ 1);
            if (pressed & 2)
                dev_pwr_en(dev_pwr_state() ^ 2);
        }

        // UART activity light
        if (uart0_available()) {
            char c = uart0_read_unblocked();
            act(true);
        }

        if (act(false))
            led |= 0b010;   // Green
        uint8_t pwr = dev_pwr_state();
        if (pwr & 1)
            led |= 0b001;   // Blue
        if (pwr & 2)
            led |= 0b100;   // Red
        led_set(led);
    }
}
