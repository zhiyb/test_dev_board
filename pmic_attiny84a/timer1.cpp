#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "led.h"
#include "key.h"

static const uint32_t timer_period_ms = 10;

void timer1_restart(void)
{
    power_timer1_enable();

    // Non-PWM, CTC mode, clock div-by-1024
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1C = 0;
    // Restart from 0
    TCNT1 = 0;
    // Configure timer period
    OCR1A = timer_period_ms * F_CPU / 1024 / 1000  - 1;

    // Enable interrupt
    TIFR1 = _BV(OCF1A);
    TIMSK1 = _BV(OCIE1A);
    // Configure clock div and start timer
    TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);
}

bool timer1_enabled(void)
{
    return !(PRR & _BV(PRTIM1));
}

ISR(TIM1_COMPA_vect)
{
    // One-shot timer, disable interrupt, clock and power
    TIMSK1 = 0;
    TCCR1B = 0;
    power_timer1_disable();

    // Call handlers
    led_act_off();
    key_irq(key_update());
}
