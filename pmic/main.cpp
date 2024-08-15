#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "led.h"
#include "key.h"
#include "dev.h"
#include "uart.h"

void init()
{
    MCUCR |= 0x80;			// Disable JTAG
    MCUCR |= 0x80;

    PORTB = LED_INIT_PORTB_MASK | DEV_INIT_PORTB_MASK | KEY_INIT_PORTB_MASK | UART_INIT_PORTB_MASK;
    PORTC = LED_INIT_PORTC_MASK | DEV_INIT_PORTC_MASK | KEY_INIT_PORTC_MASK | UART_INIT_PORTC_MASK;
    PORTD = LED_INIT_PORTD_MASK | DEV_INIT_PORTD_MASK | KEY_INIT_PORTD_MASK | UART_INIT_PORTD_MASK;

    DDRB = LED_INIT_DDRB_MASK | DEV_INIT_DDRB_MASK | KEY_INIT_DDRB_MASK | UART_INIT_DDRB_MASK;
    DDRC = LED_INIT_DDRC_MASK | DEV_INIT_DDRC_MASK | KEY_INIT_DDRC_MASK | UART_INIT_DDRC_MASK;
    DDRD = LED_INIT_DDRD_MASK | DEV_INIT_DDRD_MASK | KEY_INIT_DDRD_MASK | UART_INIT_DDRD_MASK;

    uart0_init();
}

int main()
{
    init();

    uint8_t prev_key = 0;
    uint8_t power_esp = 0;
    uint8_t power_pico = 0;
    uint8_t led_act = 0;

    for (;;) {
        uint8_t led = 0;
        uint8_t key = key_state();
        if (prev_key != key) {
            uint32_t pressed = key & ~prev_key;
            prev_key = key;
            led_act = 0xff;

            if (pressed & 1) {
                power_esp = !power_esp;
                dev_esp_en(power_esp);
            }
            if (pressed & 2) {
                power_pico = !power_pico;
                dev_pico_en(power_pico);
            }
        }

        if (uart0_available()) {
            char c = uart0_read_unblocked();
            led_act = 0xff;
        }

        if (led_act) {
            led_act -= 1;
            led |= 0b010;   // Green
        }

        if (power_esp)
            led |= 0b100;   // Red
        if (power_pico)
            led |= 0b001;   // Blue

        led_set(led);
    }
}
