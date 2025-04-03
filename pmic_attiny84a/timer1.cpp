#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "led.h"
#include "key.h"
#include "sht.h"

static volatile bool debouncing_pending = false;
static volatile bool delay_pending = false;

void timer1_restart_ms(uint8_t ms)
{
    power_timer1_enable();

    // Non-PWM, CTC mode, clock div-by-1024
    // reset value: TCCR1A = 0;
    TCCR1B = 0;
    // reset value: TCCR1C = 0;
    // Restart from 0
    TCNT1 = 0;
    // Configure timer period
    uint16_t cnt = F_CPU / 1000 / 64;
    cnt = (cnt * ms + (1024 / 64 - 1)) / (1024 / 64);
    OCR1A = cnt - 1;

    // Enable interrupt
    TIFR1 = _BV(OCF1A);
    TIMSK1 = _BV(OCIE1A);
    // Configure clock div and start timer
    delay_pending = true;
    TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);
}

void timer1_restart_debouncing(void)
{
    debouncing_pending = true;
    if (timer1_enabled())
        return;

    // Start timer
    static const uint8_t ms = 10;
    power_timer1_enable();

    // Non-PWM, CTC mode, clock div-by-1024
    // reset value: TCCR1A = 0;
    TCCR1B = 0;
    // reset value: TCCR1C = 0;
    // Restart from 0
    TCNT1 = 0;
    // Configure timer period
    uint16_t cnt = F_CPU / 1000 / 64;
    cnt = (cnt * ms + (1024 / 64 - 1)) / (1024 / 64);
    OCR1A = cnt - 1;

    // Enable interrupt
    TIFR1 = _BV(OCF1A);
    TIMSK1 = _BV(OCIE1A);
    // Configure clock div and start timer
    TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);
}

bool timer1_enabled(void)
{
    // Check is power_timer1 enabled
    return !(PRR & _BV(PRTIM1));
}

ISR(TIM1_COMPA_vect)
{
    // One-shot timer, disable interrupt, clock and power
    TCCR1B = 0;
    TIMSK1 = 0;
    power_timer1_disable();

    // Call handlers
    if (delay_pending) {
        delay_pending = false;
        sht_timer_irq();

        // Check if need to re-arm timer for debouncing
        if (debouncing_pending)
            timer1_restart_debouncing();

    } else if (debouncing_pending) {
        // Timer was triggered for debouncing
        debouncing_pending = false;
        led_act_off();
        key_irq(key_update());
    }
}
