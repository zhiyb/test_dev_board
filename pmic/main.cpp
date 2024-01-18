#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "led.h"
#include "key.h"
#include "dev.h"

void init()
{
    MCUCR |= 0x80;			// Disable JTAG
    MCUCR |= 0x80;

    PORTB = LED_INIT_PORTB_MASK | DEV_INIT_PORTB_MASK | KEY_INIT_PORTB_MASK;
    PORTC = LED_INIT_PORTC_MASK | DEV_INIT_PORTC_MASK | KEY_INIT_PORTC_MASK;
    PORTD = LED_INIT_PORTD_MASK | DEV_INIT_PORTD_MASK | KEY_INIT_PORTD_MASK;

    DDRB = LED_INIT_DDRB_MASK | DEV_INIT_DDRB_MASK | KEY_INIT_DDRB_MASK;
    DDRC = LED_INIT_DDRC_MASK | DEV_INIT_DDRC_MASK | KEY_INIT_DDRC_MASK;
    DDRD = LED_INIT_DDRD_MASK | DEV_INIT_DDRD_MASK | KEY_INIT_DDRD_MASK;
}

int main()
{
    init();

    for (;;) {
        uint8_t key = key_state();
        uint8_t led = 0;
        if (key & 1)
            led |= 0b011;
        if (key & 2)
            led |= 0b100;
        led_set(led);
        dev_pico_en(key & 1);
        dev_esp_en(key & 2);
    }
}
