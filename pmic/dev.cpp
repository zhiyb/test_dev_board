#include <avr/io.h>
#include "dev.h"

uint8_t dev_pwr_state(void)
{
    uint8_t v = 0;
    if (PORTD & _BV(7))     // PICO_EN
        v |= DEV_PICO_MASK;
    if (!(PORTD & _BV(4)))  // ESP_EN
        v |= DEV_ESP_MASK;
    return v;
}

void dev_pwr_en(uint8_t v)
{
    uint8_t portm = _BV(7) | _BV(4);
    uint8_t portv = 0;
    if (v & DEV_PICO_MASK)
        portv |= _BV(7);
    if (!(v & DEV_ESP_MASK))
        portv |= _BV(4);
    PORTD = (PORTD & ~portm) | portv;
}
