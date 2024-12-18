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

uint32_t wdt_sec_to_ticks(uint32_t secs)
{
    // Avoid u32 overflow
    static const uint32_t blk_max_secs = (1ull << 32) / wdt_freq;
    static const uint32_t blk_ticks = blk_max_secs * wdt_freq / wdt_div;
    static const uint32_t blk_secs = blk_ticks * wdt_div / wdt_freq;
    uint32_t ticks = 0;
    while (secs >= blk_secs) {
        ticks += blk_ticks;
        secs -= blk_secs;
    }
    ticks += secs * wdt_freq / wdt_div;
    return ticks;
}

ISR(WATCHDOG_vect)
{
    tick += 1;
    dev_wdt_irq(tick);
}
