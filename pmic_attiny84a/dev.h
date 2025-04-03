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

void dev_pwr_req(dev_t dev, bool en);
uint8_t dev_pwr_state(void);

bool dev_pwr_req_pending(void);
void dev_pwr_req_proc(void);

void dev_wdt_irq(uint32_t tick);
void dev_schedule_sec(dev_t dev, uint32_t sec);
