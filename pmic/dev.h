#pragma once

#include <stdint.h>

// pin ESP_EN:  PD4
// pin PICO_EN: PD7

#define DEV_INIT_PORTA_MASK  0
#define DEV_INIT_PORTB_MASK  0
#define DEV_INIT_PORTC_MASK  0
#define DEV_INIT_PORTD_MASK  ((1 << 4))

#define DEV_INIT_DDRA_MASK   0
#define DEV_INIT_DDRB_MASK   0
#define DEV_INIT_DDRC_MASK   0
#define DEV_INIT_DDRD_MASK   ((1 << 4) | (1 << 7))

#define DEV_PICO_MASK   _BV(0)
#define DEV_ESP_MASK    _BV(1)

uint8_t dev_pwr_state(void);
void dev_pwr_en(uint8_t en);
