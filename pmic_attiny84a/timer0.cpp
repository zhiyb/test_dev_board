#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "led.h"
#include "key.h"

static const uint32_t timer_period_ms = 10;

void timer0_init(void)
{
    power_timer0_enable();

    // Non-PWM, CTC mode
    TCCR0A = _BV(WGM01);
    OCR0A = timer_period_ms * F_CPU / 1024 / 1000  - 1;
    OCR0B = 0;
    // Enable interrupt
    TIMSK0 = _BV(OCIE0A);
}

void timer0_restart(void)
{
    // Restart from 0
    TCNT0 = 0;
    // Clock div 1024, start timer
    TCCR0B = _BV(CS02) | _BV(CS00);
}

bool timer0_enabled(void)
{
    return TCCR0B != 0;
}

ISR(TIMER0_COMPA_vect)
{
    // One-shot timer, disable clock source
    TCCR0B = 0;

    // Call handlers
    led_act_off();
    key_update();
}
