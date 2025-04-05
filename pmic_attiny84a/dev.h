#pragma once

#include <stdint.h>

// pin AUX_EN:  PA1
// pin ESP_EN:  PA2
// pin 3V3_EN:  PA3

#define DEV_INIT_PORTA_MASK  (_BV(1) | _BV(2))
#define DEV_INIT_PORTB_MASK  0

#define DEV_INIT_DDRA_MASK   (_BV(1) | _BV(2) | _BV(3))
#define DEV_INIT_DDRB_MASK   0

typedef enum : uint8_t {
    DevAux = 0,
    DevEsp,
    DevSHT,
    NumDevs,
} dev_t;

void dev_init(void);

void dev_pwr_req(dev_t dev, bool en);
uint8_t dev_pwr_state(void);

bool dev_pwr_req_pending(void);
void dev_pwr_req_proc(void);

void dev_wdt_irq(uint32_t tick);

uint16_t dev_get_timeout_ticks(dev_t dev);
void dev_set_timeout_ticks(dev_t dev, uint16_t ticks);
uint16_t dev_get_periodic_ticks(dev_t dev);
void dev_set_periodic_ticks(dev_t dev, uint16_t ticks);
uint32_t dev_get_next_tick(dev_t dev);
void dev_set_next_tick(dev_t dev, uint32_t tick);
bool dev_get_scheduled(dev_t dev);
void dev_set_scheduled(dev_t dev, bool schd);
