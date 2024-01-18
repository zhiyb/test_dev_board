#include <avr/io.h>
#include "key.h"

uint8_t key_state()
{
    return PINB & ((1 << PB0) | (1 << PB1));
}
