#pragma once

#include <stdint.h>

// pin LED_R: PD6
// pin LED_G: PB3
// pin LED_B: PD5

#define LED_INIT_PORTA_MASK  0
#define LED_INIT_PORTB_MASK  (1 << 3)
#define LED_INIT_PORTC_MASK  0
#define LED_INIT_PORTD_MASK  ((1 << 5) | (1 << 6))

#define LED_INIT_DDRA_MASK   0
#define LED_INIT_DDRB_MASK   (1 << 3)
#define LED_INIT_DDRC_MASK   0
#define LED_INIT_DDRD_MASK   ((1 << 5) | (1 << 6))

void led_set(uint8_t state);
