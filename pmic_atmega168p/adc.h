#pragma once

#include <stdint.h>

typedef enum {
    AdcChTemp,  // Calibration needed
    AdcChVcc,   // Unit: 1/4096 V
} adc_channel_t;

void adc_start(void);
bool adc_busy(void);
uint16_t adc_result(uint8_t ch);
