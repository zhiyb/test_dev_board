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
    PCMSK0 = _BV(PCINT7);
    GIMSK = _BV(PCIE0);
}

uint8_t key_state(void)
{
    return state;
}

uint8_t key_update(void)
{
    uint8_t s = !!(PINA & _BV(7));
    state = s;
    return s;
}

ISR(PCINT0_vect)
{
    // Timer used for de-bouncing is also be triggered by led_act_trigger()
    led_act_trigger();
}
