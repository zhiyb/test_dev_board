#include <avr/io.h>
#include <avr/interrupt.h>
#include "wdt.h"
#include "led.h"
#include "dev.h"

// Pick the min freq so timeout always happens earlier then programmed
static const uint32_t wdt_freq = 110000;
// For power-saving, set WDT period to max, approx. 8s
static const uint32_t wdt_div = 1024ul * 1024ul;

static uint32_t tick = 0;

void wdt_init(void)
{
    // Use WDT as generic timer, enable interrupt
    // Timed configuration sequence
    WDTCSR = _BV(WDE) | _BV(WDCE);
    // Interrupt mode, div 1024k
    WDTCSR = _BV(WDIE) | _BV(WDP3) | _BV(WDP0);
}

uint32_t wdt_tick(void)
{
    return tick;
}

uint32_t wdt_sec_to_ticks(uint32_t sec)
{
    return sec * wdt_freq / wdt_div;
}

ISR(WDT_vect)
{
    tick += 1;
    led_act_trigger();
    dev_wdt_irq(tick);
}
