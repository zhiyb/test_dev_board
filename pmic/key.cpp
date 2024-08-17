#include <avr/io.h>
#include <avr/interrupt.h>
#include "key.h"
#include "led.h"
#include "timer.h"

static volatile uint8_t state;

void key_init(void)
{
    state = 0;
    // Set up pin change interrupt
    PCMSK0 = _BV(PCINT1) | _BV(PCINT0);
    PCMSK2 = _BV(PCINT16) | _BV(PCINT17);
    PCICR = _BV(PCIE0) | _BV(PCIE2);
}

uint8_t key_state(void)
{
    return state;
}

void key_update(void)
{
    state = PINB & 0b11;
}

ISR(PCINT0_vect)
{
    led_act_trigger();
    // timer0_restart() is already called by led_act_trigger()
    //timer0_restart();
}

ISR(PCINT2_vect)
{
    led_act_trigger();
}
