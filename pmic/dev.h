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

void dev_pico_en(uint8_t en);
void dev_esp_en(uint8_t en);
