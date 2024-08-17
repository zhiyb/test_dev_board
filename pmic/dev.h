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

typedef enum {
    DevPico = 0,
    DevEsp,
    NumDevs
} dev_t;

bool dev_pwr_enabled(dev_t dev);
void dev_pwr_en(dev_t dev, bool en);

void dev_wdt_irq(uint32_t tick);
void dev_schedule_sec(dev_t dev, uint32_t sec);
