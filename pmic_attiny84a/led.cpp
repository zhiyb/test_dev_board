#include <avr/io.h>
#include "led.h"
#include "timer.h"

uint8_t led_state = 0;

void led_set(led_t led, bool on)
{
    // Use one-bit writes for safe atomic RMW
    switch (led) {
    case LedRed:
        if (on)
            PORTB &= ~_BV(PB0);
        else
            PORTB |= _BV(PB0);
        break;
    case LedGreen:
        if (on)
            PORTB &= ~_BV(PB2);
        else
            PORTB |= _BV(PB2);
        break;
    case LedBlue:
        if (on)
            PORTB &= ~_BV(PB1);
        else
            PORTB |= _BV(PB1);
        break;
    }
}

void led_act_trigger(void)
{
    led_set(LedGreen, true);
    timer1_restart();
}

void led_act_off(void)
{
    led_set(LedGreen, false);
}
