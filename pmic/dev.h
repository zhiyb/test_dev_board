#pragma once

#include <stdint.h>

// pin SENSORS_EN:  PD3
// pin ESP_EN:      PD4
// pin PICO_EN:     PD7

#define DEV_INIT_PORTA_MASK  0
#define DEV_INIT_PORTB_MASK  0
#define DEV_INIT_PORTC_MASK  0
#define DEV_INIT_PORTD_MASK  (_BV(3) | _BV(4))

#define DEV_INIT_DDRA_MASK   0
#define DEV_INIT_DDRB_MASK   0
#define DEV_INIT_DDRC_MASK   0
#define DEV_INIT_DDRD_MASK   (_BV(3) | _BV(4) | _BV(7))

typedef enum {
    DevPico = 0,
    DevEsp,
    DevSensors,
    NumDevs
} dev_t;

bool dev_pwr_enabled(dev_t dev);
void dev_pwr_en(dev_t dev, bool en);

void dev_wdt_irq(uint32_t tick);
void dev_schedule_sec(dev_t dev, uint32_t sec);
