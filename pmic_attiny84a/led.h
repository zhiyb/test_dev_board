#pragma once

#include <stdint.h>

// pin LED_R: PB0
// pin LED_G: PB2
// pin LED_B: PB1

#define LED_INIT_PORTA_MASK  0
#define LED_INIT_PORTB_MASK  (_BV(0) | _BV(1) | _BV(2))

#define LED_INIT_DDRA_MASK   0
#define LED_INIT_DDRB_MASK   (_BV(0) | _BV(1) | _BV(2))

typedef enum {LedRed, LedGreen, LedBlue} led_t;

void led_set(led_t led, bool on);
void led_act_trigger(void);
void led_act_off(void);
