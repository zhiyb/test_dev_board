#include <avr/io.h>
#include "led.h"
#include "timer.h"

uint8_t led_state = 0;

void led_init(void)
{
    led_set(LedRed, false);
    led_set(LedGreen, false);
    led_set(LedBlue, false);
}

void led_set(led_t led, bool on)
{
    // Use one-bit writes for safe atomic RMW
    switch (led) {
    case LedRed:
        if (on)
            PORTD &= ~_BV(PD6);
        else
            PORTD |= _BV(PD6);
        break;
    case LedGreen:
        if (on)
            PORTB &= ~_BV(PB3);
        else
            PORTB |= _BV(PB3);
        break;
    case LedBlue:
        if (on)
            PORTD &= ~_BV(PD5);
        else
            PORTD |= _BV(PD5);
        break;
    }
}

void led_act_trigger(void)
{
    led_set(LedGreen, true);
    timer0_restart();
}

void led_act_off(void)
{
    led_set(LedGreen, false);
}
