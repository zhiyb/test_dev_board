#include <avr/io.h>

#include "dev.h"

void dev_pico_en(uint8_t en)
{
    if (en)
        PORTD |= (1 << 7);
    else
        PORTD &= ~(1 << 7);

}

void dev_esp_en(uint8_t en)
{
    if (en)
        PORTD &= ~(1 << 4);
    else
        PORTD |= (1 << 4);
}
