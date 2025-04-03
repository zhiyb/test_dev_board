#pragma once

#include <stdint.h>

typedef struct {
    uint16_t t, rh;
    uint16_t tick;
} sht_data_t;

void sht_trigger_update(void);

void sht_powered_on(void);
void sht_timer_irq(void);

const sht_data_t *sht_last_measurement(void);
const sht_data_t *sht_read_measurement_log(void);
uint8_t sht_measurement_log_length(void);
