#include <avr/io.h>
#include "led.h"

void led_set(uint8_t state)
{
    uint8_t bmask = 0, dmask = 0;
    if (!(state & 0x04))   // RED
        dmask |= (1 << PD6);
    if (!(state & 0x02))   // GREEN
        bmask |= (1 << PB3);
    if (!(state & 0x01))   // BLUE
        dmask |= (1 << PD5);
    PORTD = PORTD & ~((1 << PD5) | (1 << PD6)) | dmask;
    PORTB = PORTB & ~((1 << PB3)) | bmask;
}
